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
                if constexpr (std::is_same_v<std::decay_t<decltype(T_USER_SYSTEM::typedef_v)>, xecs::system::type::simple>)
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
                .m_Guid = T_SYSTEM::typedef_v.m_Guid.m_Value
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
        using real_system = details::compleated<T_SYSTEM>;

        //
        // Register System
        //
        m_Systems.push_back
        (
            {
                std::make_unique< real_system >(GameMgr)
            ,   &type::info_v<T_SYSTEM>
            }
        );
        auto& System = *static_cast<real_system*>(std::get<std::unique_ptr<xecs::system::instance>>(m_Systems.back()).get());

        m_SystemMaps.emplace( std::pair<type::guid, xecs::system::instance*>
        { type::info_v<T_SYSTEM>.m_Guid, static_cast<instance*>(&System) } 
        );

        //
        // Connect all the delegates
        //
        if constexpr( std::is_same_v<std::decay_t<decltype(real_system::typedef_v)>, xecs::system::type::simple> )
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

}