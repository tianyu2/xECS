namespace xecs::system
{
    namespace type
    {
        using guid = xcore::guid::unit<64, struct system_tag>;

        enum class id : std::uint8_t
        {
            UPDATE
        ,   NOTIFY_CREATE
        ,   NOTIFY_DESTROY
        ,   NOTIFY_MODIFIED
        ,   NOTIFY_MOVE_IN
        ,   NOTIFY_MOVE_OUT
        ,   NOTIFY_COMPONENT_CHANGE
        ,   NOTIFY_COMPONENT_ADDED
        ,   NOTIFY_COMPONENT_REMOVE
        };

        struct update
        {
            static constexpr auto   id_v    = id::UPDATE;
            const char*             m_pName = "Unnamed Update System";
            guid                    m_Guid  {};
        };

        struct notify_create
        {
            static constexpr auto   id_v    = id::NOTIFY_CREATE;
            const char*             m_pName = "Unnamed Notified Create Entity System";
            guid                    m_Guid  {};
        };

        struct notify_destroy
        {
            static constexpr auto   id_v    = id::NOTIFY_DESTROY;
            const char*             m_pName = "Unnamed Notified Destroy Entity System";
            guid                    m_Guid  {};
        };

        struct notify_moved_in
        {
            static constexpr auto   id_v    = id::NOTIFY_MOVE_IN;
            const char*             m_pName = "Unnamed Notified Move In Entity System";
            guid                    m_Guid  {};
        };

        struct notify_moved_out
        {
            static constexpr auto   id_v    = id::NOTIFY_MOVE_OUT;
            const char*             m_pName = "Unnamed Notified Move Out System";
            guid                    m_Guid  {};
        };

        struct notify_component_change
        {
            static constexpr auto        id_v             = id::NOTIFY_COMPONENT_CHANGE;
            const char*                  m_pName          = "Unnamed Component Change System";
            guid                         m_Guid          {};
            const xecs::component::info* m_pComponentInfo{};
        };

        struct info
        {
            type::guid                              m_Guid;
            const char*                             m_pName;
            id                                      m_ID;
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
        constexpr static auto   typedef_v   = type::update{};

        void                    OnGameStart     ( void ) noexcept {}
        void                    OnFrameStart    ( void ) noexcept {}
        void                    OnUpdate        ( void ) noexcept {}
        void                    OnFrameEnd      ( void ) noexcept {}
        void                    OnGameEnd       ( void ) noexcept {}
        void                    OnNotified      ( xecs::component::entity& Entity ) noexcept {}
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