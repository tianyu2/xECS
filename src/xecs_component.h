namespace xecs::component
{
    //
    // INFO
    //
    struct info final
    {
        constexpr static auto invalid_id_v = 0xffff;
        using guid = xcore::guid::unit<64, struct component_type_tag>;

        using construct_fn  = void(std::byte*) noexcept;
        using destruct_fn   = void(std::byte*) noexcept;
        using move_fn       = void(std::byte* Dst, std::byte* Src ) noexcept;

        mutable std::uint16_t   m_UID;
        std::uint32_t           m_Size;
        guid                    m_Guid;
        construct_fn*           m_pConstructFn;
        destruct_fn*            m_pDestructFn;
        move_fn*                m_pMoveFn;
    };

    namespace details
    {
        template< typename T >
        consteval info CreateInfo(void) noexcept;

        template< typename T >
        static constexpr auto info_v = CreateInfo<T>();
    }
    template< typename T >
    constexpr auto& info_v = details::info_v<std::decay_t<T>>;

    //
    // ENTITY
    //
    union entity final
    {
        union validation final
        {
            std::uint32_t       m_Value;
            struct
            {
                std::uint32_t   m_Generation:31     // Which generation to avoid collisions
                ,               m_bZombie:1;        // If the entity is mark as a zombie
            };

            constexpr bool operator == (const validation& V) const noexcept { return m_Value == V.m_Value; }
        };
        static_assert( sizeof(validation) == sizeof(std::uint32_t) );

        std::uint64_t       m_Value;
        struct
        {
            std::uint32_t   m_GlobalIndex;      // Index of the entity in the global pool in the game_mgr
            validation      m_Validation;       // Determine the state and give an additional unique element
        };

        constexpr bool isZombie         ( void )                const noexcept { return m_Validation.m_bZombie; }
        constexpr bool operator ==      ( const entity& V )     const noexcept { return m_Value == V.m_Value;   }
    };
    static_assert(sizeof(entity) == sizeof(std::uint64_t));

    //
    // MGR
    //
    struct mgr final
    {
        template
        < typename T_COMPONENT
        >
        void RegisterComponent          ( void
                                        ) noexcept;

        inline static int m_UniqueID = 0;
    };
}
