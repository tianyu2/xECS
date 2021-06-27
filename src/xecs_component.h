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
                friend auto operator <=> ( const key&, const key& ) = default;
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
        constexpr bool is_valid_v = details::is_valid<xcore::types::decay_full_t<T_COMPONENT>>::value;

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
            type::share::key        m_DefaultShareKey;  // Default value for this share component
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
        constexpr static auto invalid_entity_v  = 0xffffffffffffffffu;
        constexpr static auto typedef_v         = xecs::component::type::data
        {
            .m_pName = "Entity"
        };

        // Validation allows us to know if a particular entity Unique ID is still valid
        // Farther more it allow us to know if an entity has been deleted but not removed yet (Zombie)
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

        // The actual global record of the entity is contain in this structure
        // Which it means that for every entity in the game there is one of these
        // Note that this record gets reclycle so knowing of an all entity ID is
        // still valid can be done thought the m_Validation 
        struct global_info final
        {
            xecs::archetype::instance*      m_pArchetype    {};
            xecs::pool::instance*           m_pPool         {};
            xecs::pool::index               m_PoolIndex     {-1};
            validation                      m_Validation    {};
        };

        // This is the actual entity component data members. It boils down to an
        // index to the global record as shown above and a validation.
        std::uint64_t       m_Value{ invalid_entity_v };
        struct
        {
            std::uint32_t   m_GlobalIndex;      // Index of the entity in the global pool in the game_mgr
            validation      m_Validation;       // Determine the state and give an additional unique element
        };

        constexpr bool isZombie         ( void )                const noexcept { return m_Validation.m_bZombie; }
        constexpr bool operator ==      ( const entity& V )     const noexcept { return m_Value == V.m_Value;   }
        constexpr bool isValid          ( void )                const noexcept { return m_Value != invalid_entity_v; }
    };
    static_assert(sizeof(entity) == sizeof(std::uint64_t));

    //
    // Reference Count Data Component
    //
    struct ref_count
    {
        constexpr static auto typedef_v = xecs::component::type::data
        {
            .m_pName = "Reference Count"
        };

        int m_Value{1};
    };

    //
    // MGR
    //
    struct mgr final
    {
        inline
                                            mgr                     ( void 
                                                                    ) noexcept;
        template
        < typename T_COMPONENT
        > requires
        ( xecs::component::type::is_valid_v<T_COMPONENT>
        )
        void                                RegisterComponent       ( void
                                                                    ) noexcept;
        inline
        const entity::global_info&          getEntityDetails        ( xecs::component::entity Entity 
                                                                    ) const noexcept;
        inline
        void                                DeleteGlobalEntity      ( std::uint32_t              GlobalIndex
                                                                    , xecs::component::entity&   SwappedEntity 
                                                                    ) noexcept;
        inline
        void                                DeleteGlobalEntity      ( std::uint32_t GlobalIndex
                                                                    ) noexcept;
        inline
        void                                MovedGlobalEntity       ( xecs::pool::index         PoolIndex
                                                                    , xecs::component::entity&  SwappedEntity
                                                                    ) noexcept;
        inline 
        entity                              AllocNewEntity          ( pool::index                   PoolIndex
                                                                    , xecs::archetype::instance&    Archetype
                                                                    , xecs::pool::instance&         Pool 
                                                                    ) noexcept;


        inline static int                                   m_UniqueID  = 0;
        std::unique_ptr<entity::global_info[]>              m_lEntities = std::make_unique<entity::global_info[]>(xecs::settings::max_entities_v);
        int                                                 m_EmptyHead = 0;
    };
}

template<>
struct std::hash< xecs::component::type::share::key >
{
    auto operator()(const typename xecs::component::type::share::key obj) const { return hash<std::uint64_t>()(obj.m_Value); }
};

