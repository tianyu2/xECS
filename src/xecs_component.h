namespace xecs::component
{
    //
    // TYPE DEFINITIONS
    //
    namespace type
    {
        using guid = xcore::guid::unit<64, struct component_type_tag>;

        enum class id : std::uint8_t
        {
            DATA
        ,   TAG
        ,   SHARE
        };

        struct data
        {
            constexpr static auto   max_size_v          = xecs::settings::virtual_page_size_v;
            constexpr static auto   id_v                = id::DATA;

            guid                    m_Guid              {};
            const char*             m_pName             {"Unnamed data component"};
        };

        struct tag
        {
            constexpr static auto   max_size_v          = 1;
            constexpr static auto   id_v                = id::TAG;

            guid                    m_Guid              {};
            const char*             m_pName             { "Unnamed tag component" };
        };

        struct share
        {
            constexpr static auto   max_size_v          = xecs::settings::virtual_page_size_v;
            constexpr static auto   id_v                = id::SHARE;

            struct key
            {
                std::uint64_t       m_Value;
            };
            using compute_key_fn = key(const std::byte*) noexcept;

            guid                    m_Guid              {};
            const char*             m_pName             { "Unnamed tag component" };
            bool                    m_bGlobalScoped     { true };
            bool                    m_bKeyCanFilter     { false };
            compute_key_fn*         m_ComputeKeyFn      { nullptr };
        };

        namespace details
        {
            template< typename T_COMPONENT >
            struct is_valid
            {
                template<auto U>     struct Check;
                template<typename>   static std::false_type Test(...);
                template<typename C> static auto Test(Check<&C::typedef_v>*) -> std::conditional_t
                                     <
                                        ( std::is_same_v< const data, decltype(C::typedef_v) > && sizeof(T_COMPONENT) <= data::max_size_v )
                                     || ( std::is_same_v< const tag,  decltype(C::typedef_v) > && sizeof(T_COMPONENT) <= tag::max_size_v  )
                                     , std::true_type
                                     , std::false_type
                                     >;
                constexpr static auto value = decltype(Test<T_COMPONENT>(nullptr))::value;
            };
        }
        template< typename T_COMPONENT >
        constexpr bool is_valid_v = details::is_valid<std::decay_t<T_COMPONENT>>::value;

        //
        // TYPE INFO
        //
        struct info final
        {
            constexpr static auto invalid_bit_id_v = 0xffff;

            using construct_fn      = void(std::byte*) noexcept;
            using destruct_fn       = void(std::byte*) noexcept;
            using move_fn           = void(std::byte* Dst, std::byte* Src ) noexcept;
            using compute_key_fn    = share::compute_key_fn;

            type::guid              m_Guid;             // Unique Identifier for the component type
            mutable std::uint16_t   m_BitID;            // Which bit was allocated for this type at run time
            std::uint16_t           m_Size;             // Size of the component in bytes
            type::id                m_TypeID;           // Simple enumeration that tells what type of component is this
            bool                    m_bGlobalScoped:1   // If the component is a share, it indicates if it should be factor to a globally scope or to an archetype scope
            ,                       m_bKeyCanFilter:1;  // If the component is a share, it indicates if the query can filter by its key
            construct_fn*           m_pConstructFn;     // Constructor function pointer if required
            destruct_fn*            m_pDestructFn;      // Destructor function pointer if required
            move_fn*                m_pMoveFn;          // Move function pointer if required
            compute_key_fn*         m_pComputeKeyFn;    // Computes the key from a share component
            const char*             m_pName;            // Friendly Human readable string name for the component type
        };

        namespace details
        {
            template< typename T >
            consteval info CreateInfo(void) noexcept;

            template< typename T >
            static constexpr auto info_v = CreateInfo<T>();
        }
        template< typename T_COMPONENT >
        requires( type::is_valid_v<xcore::types::decay_full_t<T_COMPONENT>> )
        constexpr auto& info_v = details::info_v<xcore::types::decay_full_t<T_COMPONENT>>;
    } 

    //
    // ENTITY
    //
    union entity final
    {
        constexpr static auto typedef_v = xecs::component::type::data
        {
            .m_pName = "Entity"
        };

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

        std::uint64_t       m_Value{0xffffffffffffffffu};
        struct
        {
            std::uint32_t   m_GlobalIndex;      // Index of the entity in the global pool in the game_mgr
            validation      m_Validation;       // Determine the state and give an additional unique element
        };

        constexpr bool isZombie         ( void )                const noexcept { return m_Validation.m_bZombie; }
        constexpr bool operator ==      ( const entity& V )     const noexcept { return m_Value == V.m_Value;   }
        constexpr bool isValid          ( void )                const noexcept { return m_Value != 0xffffffffffffffffu; }
    };
    static_assert(sizeof(entity) == sizeof(std::uint64_t));

    //
    // MGR
    //
    struct mgr final
    {
        template
        < typename T_COMPONENT
        > requires
        ( xecs::component::type::is_valid_v<T_COMPONENT>
        )
        void RegisterComponent          ( void
                                        ) noexcept;

        inline static int m_UniqueID = 0;
    };
}
