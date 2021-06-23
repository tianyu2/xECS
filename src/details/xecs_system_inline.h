namespace xecs::system
{
    namespace details
    {
        template< typename T_USER_SYSTEM >
        requires( std::derived_from< T_USER_SYSTEM, xecs::system::instance > )
        struct compleated final : T_USER_SYSTEM
        {
            __inline
            compleated( xecs::game_mgr::instance& GameMgr, const xecs::system::type::info& TypeInfo ) noexcept
            : T_USER_SYSTEM{ { GameMgr, TypeInfo } }
            {}
            compleated( void ) noexcept = delete;

            __inline
            void Run( void ) noexcept
            {
                if constexpr ( T_USER_SYSTEM::typedef_v.id_v == xecs::system::type::id::UPDATE )
                {
                    XCORE_PERF_ZONE_SCOPED_N(xecs::system::type::info_v<T_USER_SYSTEM>.m_pName)
                    if constexpr (&T_USER_SYSTEM::OnUpdate != &instance::OnUpdate)
                    {
                        T_USER_SYSTEM::OnUpdate();
                    }
                    else
                    {
                        xecs::query::instance Query;
                        Query.AddQueryFromTuple(xcore::types::null_tuple_v< T_USER_SYSTEM::query >);
                        Query.AddQueryFromFunction(*this);
                        T_USER_SYSTEM::m_GameMgr.Foreach(T_USER_SYSTEM::m_GameMgr.Search(Query), *this);
                    }
                }
            }

            void Notify( xecs::component::entity& Entity ) noexcept
            {
                XCORE_PERF_ZONE_SCOPED_N(xecs::system::type::info_v<T_USER_SYSTEM>.m_pName)
                if constexpr (T_USER_SYSTEM::typedef_v.is_notifier_v )
                {
                    XCORE_PERF_ZONE_SCOPED_N(xecs::system::type::info_v<T_USER_SYSTEM>.m_pName)
                    if constexpr (&T_USER_SYSTEM::OnNotify != &instance::OnNotify)
                    {
                        T_USER_SYSTEM::OnNotify(Entity);
                    }
                    else
                    {
                        // TODO: Optimize this
                        T_USER_SYSTEM::m_GameMgr.findEntity( Entity, *this );
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
                .m_Guid                 = T_SYSTEM::typedef_v.m_Guid.m_Value
                                             ? T_SYSTEM::typedef_v.m_Guid
                                             : type::guid{ __FUNCSIG__ }
            ,   .m_pName                = T_SYSTEM::typedef_v.m_pName
            ,   .m_ID                   = T_SYSTEM::typedef_v.id_v
            ,   .m_NotifierRegistration = (T_SYSTEM::typedef_v.id_v == type::id::UPDATE)
                                           ? nullptr
                                           : []( xecs::archetype::instance&     Archetype
                                            , xecs::system::instance&           System 
                                            ) noexcept
                                            {
                                               using real_system = xecs::system::details::compleated<T_SYSTEM>;
                                                if constexpr (T_SYSTEM::typedef_v.id_v == type::id::UPDATE )
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
                                                else 
                                                {
                                                    static_assert( xcore::types::always_false_v<T_SYSTEM>, "Case is not supported for right now");
                                                }
                                            }
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
    template
    < typename T_SYSTEM
    > requires( std::derived_from< T_SYSTEM, xecs::system::instance> )
    T_SYSTEM& mgr::RegisterSystem( xecs::game_mgr::instance& GameMgr ) noexcept
    {
        using real_system = details::compleated<T_SYSTEM>;

        //
        // Register System
        //
        auto& System = *static_cast<real_system*>([&]
        {
            if constexpr( std::is_same_v<std::decay_t<decltype(real_system::typedef_v)>, xecs::system::type::update> )
            {
                m_UpdaterSystems.push_back(std::make_unique< real_system >(GameMgr, type::info_v<T_SYSTEM>));
                return m_UpdaterSystems.back().get();
            }
            else
            {
                m_NotifierSystems.push_back(std::make_unique< real_system >(GameMgr, type::info_v<T_SYSTEM>));
                return m_NotifierSystems.back().get();
            }
        }());

        m_SystemMaps.emplace( std::pair<type::guid, xecs::system::instance*>
            { type::info_v<T_SYSTEM>.m_Guid, static_cast<instance*>(&System) } 
            );

        if constexpr ( T_SYSTEM::typedef_v.is_notifier_v )
        {
            xecs::query::instance Q;
            Q.AddQueryFromTuple(xcore::types::null_tuple_v< T_SYSTEM::query >);
            Q.AddQueryFromFunction<T_SYSTEM()>();
            type::info_v<T_SYSTEM>.m_NotifierQuery = Q;
        }

        //
        // Connect all the delegates
        //
        if constexpr( type::info_v<T_SYSTEM>.m_ID == type::id::UPDATE )
        {
            m_Events.m_OnUpdate.Register<&real_system::Run>(System);
        }
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
            if( Entry.m_TypeInfo.m_NotifierQuery.Compare(Archetype.m_ComponentBits) )
            {
                Entry.m_TypeInfo.m_NotifierRegistration( Archetype, Entry );
            }
        }
    }

}