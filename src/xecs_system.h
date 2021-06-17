namespace xecs::system
{
    //-----------------------------------------------------------------
    // ECS USER DEFINITIONS
    //-----------------------------------------------------------------
    struct overrides
    {
        using                   query       = std::tuple<>;
        constexpr static auto   name_v      = "unnamed system";

        void                    OnUpdate    ( void ) noexcept {}
    };

    struct instance : overrides
    {
        using entity = xecs::component::entity;

                    instance            ( const instance&
                                        ) noexcept = delete;

        inline      instance            ( xecs::game_mgr::instance& GameMgr 
                                        ) noexcept;

        xecs::game_mgr::instance& m_GameMgr;
    };

    //-----------------------------------------------------------------
    // ECS SYSTEM DEFINITIONS
    //-----------------------------------------------------------------
    struct mgr final
    {
        struct info
        {
            using call_run = void( xecs::system::instance&);

            std::unique_ptr<xecs::system::instance> m_System;
            call_run*                               m_CallRun;
            const char*                             m_pName;
        };

                                mgr                 ( const mgr&
                                                    ) noexcept = delete;
                                mgr                 ( void
                                                    ) noexcept = default;
        template
        < typename T_SYSTEM
        > requires
        ( std::derived_from< T_SYSTEM, xecs::system::instance>
        )
        T_SYSTEM&               RegisterSystem      ( xecs::game_mgr::instance& GameMgr 
                                                    ) noexcept;
        inline 
        void                    Run                 ( void 
                                                    ) noexcept;

        std::vector< info >  m_Systems;
    };
}