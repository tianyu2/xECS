namespace xecs::system
{
    namespace type
    {
        using guid = xcore::guid::unit<64, struct system_tag>;
        
        struct simple
        {
            const char*  m_pName = "Unnamed System";
            guid         m_Guid{};
        };

        struct info
        {
            using call_run = void( xecs::system::instance&);

            type::guid                              m_Guid;
            const char*                             m_pName;
        };

        namespace details
        {
            template< typename T >
            consteval type::info CreateInfo(void) noexcept;

            template< typename T >
            static constexpr auto info_v = CreateInfo<T>();
        }

        template< typename T_SYSTEM >
        constexpr static auto& info_v = details::info_v<T_SYSTEM>;
    }

    //-----------------------------------------------------------------
    // ECS USER DEFINITIONS
    //-----------------------------------------------------------------
    struct overrides
    {
        using                   query       = std::tuple<>;
        constexpr static auto   typedef_v   = type::simple{};

        void                    OnUpdate        ( void ) noexcept {}
        void                    OnGameStart     ( void ) noexcept {}
        void                    OnGameEnd       ( void ) noexcept {}
        void                    OnFrameStart    ( void ) noexcept {}
        void                    OnFrameEnd      ( void ) noexcept {}
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
        struct events
        {
            xecs::event::instance<>     m_OnGameStart;
            xecs::event::instance<>     m_OnGameEnd;
            xecs::event::instance<>     m_OnUpdate;
            xecs::event::instance<>     m_OnFrameStart;
            xecs::event::instance<>     m_OnFrameEnd;
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
        template< typename T_SYSTEM >
        T_SYSTEM*               find                ( void
                                                    ) noexcept;

        using system_list = std::vector< std::tuple<std::unique_ptr<xecs::system::instance>, const type::info* > >;

        std::unordered_map<type::guid, xecs::system::instance*> m_SystemMaps;
        system_list                                             m_Systems;
        events                                                  m_Events;
    };
}