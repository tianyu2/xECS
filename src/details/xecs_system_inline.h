namespace xecs::system
{
    namespace details
    {
        template< typename T_USER_SYSTEM >
        requires( std::derived_from< T_USER_SYSTEM, xecs::system::instance > )
        struct compleated final : T_USER_SYSTEM
        {
            T_USER_SYSTEM::events m_Events;

            __inline
            compleated( xecs::game_mgr::instance& GameMgr, const xecs::system::type::info& TypeInfo ) noexcept
            : T_USER_SYSTEM{ { GameMgr, TypeInfo } }
            {}
            compleated( void ) noexcept = delete;

            __inline
            void Run( void ) noexcept
            {
                //
                // Call the user OnUpdate or process directly
                //
                if constexpr ( T_USER_SYSTEM::typedef_v.id_v == xecs::system::type::id::UPDATE )
                {
                    XCORE_PERF_ZONE_SCOPED_N(xecs::system::type::info_v<T_USER_SYSTEM>.m_pName)
                    if constexpr (&T_USER_SYSTEM::OnUpdate != &instance::OnUpdate)
                    {
                        T_USER_SYSTEM::OnUpdate();
                    }
                    else
                    {
                        const auto& Q = type::info_v<T_USER_SYSTEM>.m_Query;
                        T_USER_SYSTEM::m_GameMgr.Foreach
                        (
                            T_USER_SYSTEM::m_GameMgr.Search(Q)
                        ,   *this
                        );
                    }
                }

                //
                // Ask the game to update all the pending structural changes
                //
                T_USER_SYSTEM::m_GameMgr.m_ArchetypeMgr.UpdateStructuralChanges();

                //
                // Check to see if the user would like to be notified 
                //
                if constexpr ( &T_USER_SYSTEM::OnPostStructuralChanges != &instance::OnPostStructuralChanges )
                {
                    T_USER_SYSTEM::OnPostStructuralChanges();
                }
            }

            void Notify( xecs::component::entity& Entity ) noexcept
            {
                if constexpr (T_USER_SYSTEM::typedef_v.is_notifier_v )
                {
                    XCORE_PERF_ZONE_SCOPED_N(xecs::system::type::info_v<T_USER_SYSTEM>.m_pName)
                    if constexpr (&T_USER_SYSTEM::OnNotify != &instance::OnNotify)
                    {
                        T_USER_SYSTEM::OnNotify(Entity);
                    }
                    else
                    {
                        using function_args = xcore::function::traits<compleated<T_USER_SYSTEM>>::args_tuple;
                        auto& Entry         = T_USER_SYSTEM::m_GameMgr.m_ComponentMgr.getEntityDetails(Entity);
                        {
                            auto CachedArray = xecs::archetype::details::GetDataComponentPointerArray
                            ( *Entry.m_pPool
                            , Entry.m_PoolIndex
                            , xcore::types::null_tuple_v<function_args> 
                            );
                            xecs::archetype::details::CallFunction( *this, CachedArray );
                        }
                    }
                }
            }
        };
    }

    //-------------------------------------------------------------------------------------------

    namespace type::details
    {
        template< typename T_SYSTEM >
        consteval type::info CreateInfo( void ) noexcept
        {
            return type::info
            {
                .m_Guid                 = T_SYSTEM::typedef_v.m_Guid.isValid()
                                             ? T_SYSTEM::typedef_v.m_Guid
                                             : type::guid{ __FUNCSIG__ }
            ,   .m_NotifierRegistration = (T_SYSTEM::typedef_v.id_v == type::id::UPDATE)
                                           ? nullptr
                                           : []( xecs::archetype::instance&     Archetype
                                            , xecs::system::instance&           System 
                                            ) noexcept
                                            {
                                                using real_system = xecs::system::details::compleated<T_SYSTEM>;
                                                if constexpr ( T_SYSTEM::typedef_v.is_notifier_v == false )
                                                {
                                                    // NOTHING TO DO...
                                                }
                                                else if constexpr (T_SYSTEM::typedef_v.id_v == type::id::NOTIFY_CREATE )
                                                {
                                                    Archetype.m_Events.m_OnEntityCreated.Register<&real_system::Notify>(static_cast<real_system&>(System));
                                                }
                                                else if constexpr (T_SYSTEM::typedef_v.id_v == type::id::NOTIFY_DESTROY)
                                                {
                                                    Archetype.m_Events.m_OnEntityDestroyed.Register<&real_system::Notify>(static_cast<real_system&>(System));
                                                }
                                                else if constexpr (T_SYSTEM::typedef_v.id_v == type::id::NOTIFY_MOVE_IN)
                                                {
                                                    Archetype.m_Events.m_OnEntityMovedIn.Register<&real_system::Notify>(static_cast<real_system&>(System));
                                                }
                                                else if constexpr (T_SYSTEM::typedef_v.id_v == type::id::NOTIFY_MOVE_OUT)
                                                {
                                                    Archetype.m_Events.m_OnEntityMovedOut.Register<&real_system::Notify>(static_cast<real_system&>(System));
                                                }
                                                else if constexpr (T_SYSTEM::typedef_v.id_v == type::id::POOL_FAMILY_CREATE)
                                                {
                                                    Archetype.m_Events.m_OnPoolFamilyCreated.Register<&real_system::OnPoolFamily>(static_cast<real_system&>(System));
                                                }
                                                else if constexpr (T_SYSTEM::typedef_v.id_v == type::id::POOL_FAMILY_DESTROY)
                                                {
                                                    Archetype.m_Events.m_OnPoolFamilyDestroy.Register<&real_system::OnPoolFamily>(static_cast<real_system&>(System));
                                                }
                                                else
                                                {
                                                    static_assert( xcore::types::always_false_v<T_SYSTEM>, "Case is not supported for right now");
                                                }
                                            }
            ,   .m_pName                = T_SYSTEM::typedef_v.m_pName
            ,   .m_ID                   = T_SYSTEM::typedef_v.id_v
            };
        }
    }

    //-------------------------------------------------------------------------------------------
    constexpr
    instance::instance( xecs::game_mgr::instance& G, const type::info& I ) noexcept
    : m_GameMgr( G )
    , m_TypeInfo( I )
    { }

    //-------------------------------------------------------------------------------------------
    template< typename T_EVENT, typename T_CLASS, typename...T_ARGS>
    requires( std::derived_from<T_CLASS, xecs::system::instance>
                && (false == std::is_same_v<typename T_CLASS::events, xecs::system::overrides::events>)
                && !!(xcore::types::tuple_t2i_v<T_EVENT, typename T_CLASS::events > +1) 
                )
    void instance::SendEventFrom(T_CLASS* pThis, T_ARGS&&... Args) noexcept
    {
        std::get<T_EVENT>
        ( static_cast< details::compleated<T_CLASS>* >( pThis )
          ->m_Events
        )
        .NotifyAll( std::forward<T_ARGS&&>(Args) ... );
    }

    //-------------------------------------------------------------------------------------------
    template
    < typename T_SYSTEM
    > requires( std::derived_from< T_SYSTEM, xecs::system::instance> )
    T_SYSTEM& mgr::RegisterSystem( xecs::game_mgr::instance& GameMgr ) noexcept
    {
        using real_system = details::compleated<T_SYSTEM>;
        using typedef_t   = std::decay_t<decltype(T_SYSTEM::typedef_v)>;

        //
        // TODO: Validate the crap out of each system type
        //


        //
        // Register System
        //
        auto& System = *static_cast<real_system*>([&]
        {
            if constexpr( real_system::typedef_v.is_notifier_v )
            {
                m_NotifierSystems.push_back(std::make_unique< real_system >(GameMgr, type::info_v<T_SYSTEM>));
                return m_NotifierSystems.back().get();
            }
            else
            {
                m_UpdaterSystems.push_back(std::make_unique< real_system >(GameMgr, type::info_v<T_SYSTEM>));
                return m_UpdaterSystems.back().get();
            }
        }());

        m_SystemMaps.emplace( std::pair<type::guid, xecs::system::instance*>
            { type::info_v<T_SYSTEM>.m_Guid, static_cast<instance*>(&System) });

        //
        // Call the OnCreate if the user overwrote that
        //
        if constexpr ( &T_SYSTEM::OnCreate != &xecs::system::overrides::OnCreate )
        {
            System.OnCreate();
        }

        //
        // For all systems that are not type event we create the query and update the info
        // this is like caching it...
        // 
        if constexpr (real_system::typedef_v.id_v != type::id::GLOBAL_EVENT )
        {
            xecs::query::instance Q;
            Q.AddQueryFromTuple(xcore::types::null_tuple_v< T_SYSTEM::query >);
            if constexpr ( xcore::function::is_callable_v<T_SYSTEM>)
            {
                Q.AddQueryFromFunction<T_SYSTEM>();
            }
            type::info_v<T_SYSTEM>.m_Query = Q;
        }

        //
        // For the Update systems hook the run function
        //
        if constexpr(real_system::typedef_v.id_v == type::id::UPDATE )
        {
            m_Events.m_OnUpdate.Register<&real_system::Run>(System);
        }

        //
        // If it is a global event delegate then we need to hook it up with the actual event
        // NOTE: that this requires all the relevant events to be register before our system delegate
        if constexpr (real_system::typedef_v.id_v == type::id::GLOBAL_EVENT )
        {
            GameMgr.m_EventMgr.getEvent< typedef_t::event_t >()
                .Register<&T_SYSTEM::OnEvent>( GameMgr.getSystem< T_SYSTEM >() );
        }

        //
        // If we are dealing with a system event type
        //
        if constexpr (real_system::typedef_v.id_v == type::id::SYSTEM_EVENT)
        {
            static_assert( xcore::types::tuple_t2i_v<typedef_t::event_t, typedef_t::system_t::events > + 1 );

            std::get<typedef_t::event_t>
            ( 
                reinterpret_cast< details::compleated<typedef_t::system_t>* >
                ( find<typedef_t::system_t>()
                )->m_Events 
            )
            .Register<&T_SYSTEM::OnEvent>(System);
        }

        //
        // General messages
        //
        if constexpr ( &real_system::OnGameStart != &xecs::system::overrides::OnGameStart )
        {
            m_Events.m_OnGameStart.Register<&real_system::OnGameStart>(System);
        }
        if constexpr (&real_system::OnGameEnd != &xecs::system::overrides::OnGameEnd)
        {
            m_Events.m_OnGameEnd.Register<&real_system::OnGameEnd>(System);
        }
        if constexpr (&real_system::OnFrameStart != &xecs::system::overrides::OnFrameStart)
        {
            m_Events.m_OnFrameStart.Register<&real_system::OnFrameStart>(System);
        }
        if constexpr (&real_system::OnFrameEnd != &xecs::system::overrides::OnFrameEnd)
        {
            m_Events.m_OnFrameEnd.Register<&real_system::OnFrameEnd>(System);
        }

        return System;
    }

    //---------------------------------------------------------------------------

    void mgr::Run( void ) noexcept
    {
        m_Events.m_OnFrameStart.NotifyAll();
        m_Events.m_OnUpdate.NotifyAll();
        m_Events.m_OnFrameEnd.NotifyAll();
    }

    //---------------------------------------------------------------------------
    template< typename T_SYSTEM >
    T_SYSTEM* mgr::find( void ) noexcept
    {
        auto I = m_SystemMaps.find( xecs::system::type::info_v<T_SYSTEM>.m_Guid );
        if(I == m_SystemMaps.end() ) return nullptr;
        return static_cast<T_SYSTEM*>(I->second);
    }

    //---------------------------------------------------------------------------

    void mgr::OnNewArchetype( xecs::archetype::instance& Archetype ) noexcept
    {
        for( auto& E : m_NotifierSystems )
        {
            auto& Entry = *E;
            if( Entry.m_TypeInfo.m_Query.Compare(Archetype.m_ComponentBits) )
            {
                Entry.m_TypeInfo.m_NotifierRegistration( Archetype, Entry );
            }
        }
    }

}