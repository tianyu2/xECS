namespace xecs::system
{
    namespace details
    {
        template< typename T_USER_SYSTEM >
        requires( std::derived_from< T_USER_SYSTEM, xecs::system::instance > )
        struct compleated final : T_USER_SYSTEM
        {
            __inline
            compleated(xecs::game_mgr::instance& GameMgr) noexcept
            : T_USER_SYSTEM{ GameMgr }
            {}
            compleated( void ) noexcept = delete;

            __inline
            void Run( void ) noexcept
            {
                XCORE_PERF_ZONE_SCOPED_N( xecs::system::type::info_v<T_USER_SYSTEM>.m_pName )
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
                .m_CallRun = [](xecs::system::instance& System) noexcept
                {
                    static_cast<xecs::system::details::compleated<T_SYSTEM>&>(System).Run();
                }
            ,   .m_Guid = T_SYSTEM::typedef_v.m_Guid.m_Value
                              ? T_SYSTEM::typedef_v.m_Guid
                              : type::guid{ __FUNCSIG__ }
            ,   .m_pName = T_SYSTEM::typedef_v.m_pName
            };
        }
    }

    //-------------------------------------------------------------------------------------------

    instance::instance(xecs::game_mgr::instance& G) noexcept
    : m_GameMgr(G)
    {}

    //-------------------------------------------------------------------------------------------
    template
    < typename T_SYSTEM
    > requires( std::derived_from< T_SYSTEM, xecs::system::instance> )
    T_SYSTEM& mgr::RegisterSystem( xecs::game_mgr::instance& GameMgr ) noexcept
    {
        //
        // Register System
        //
        m_Systems.push_back
        (
            {
                std::make_unique< details::compleated<T_SYSTEM> >(GameMgr)
            ,   &type::info_v<T_SYSTEM>
            }
        );

        auto& System = *static_cast<T_SYSTEM*>(std::get<std::unique_ptr<xecs::system::instance>>(m_Systems.back()).get());

        m_SystemMaps.emplace( std::pair<type::guid, xecs::system::instance*>
        { type::info_v<T_SYSTEM>.m_Guid, static_cast<instance*>(&System) } 
        );

        //
        // Connect all the delegates
        //
        if constexpr ( &T_SYSTEM::OnGameStart != &xecs::system::overrides::OnGameStart )
        {
            m_Events.m_OnGameStart.Register<&T_SYSTEM::OnGameStart>(System);
        }
        if constexpr (&T_SYSTEM::OnGameEnd != &xecs::system::overrides::OnGameEnd)
        {
            m_Events.m_OnGameEnd.Register<&T_SYSTEM::OnGameEnd>(System);
        }
        if constexpr (&T_SYSTEM::OnFrameStart != &xecs::system::overrides::OnFrameStart)
        {
            m_Events.m_OnFrameStart.Register<&T_SYSTEM::OnFrameStart>(System);
        }
        if constexpr (&T_SYSTEM::OnFrameEnd != &xecs::system::overrides::OnFrameEnd)
        {
            m_Events.m_OnFrameEnd.Register<&T_SYSTEM::OnFrameEnd>(System);
        }

        return System;
    }

    //---------------------------------------------------------------------------

    void mgr::Run( void ) noexcept
    {
        m_Events.m_OnFrameStart.NotifyAll();

        for( const auto& S : m_Systems )
            std::get<const type::info*>(S)
                ->m_CallRun( *std::get<std::unique_ptr<xecs::system::instance>>(S).get() );

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

}