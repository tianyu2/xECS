namespace xecs::component
{
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
        inline
        void                                LockComponentTypes      ( void 
                                                                    ) noexcept;

        using bits_to_info_array = std::array<const xecs::component::type::info*, xecs::settings::max_component_types_v>;

        std::unique_ptr<entity::global_info[]>              m_lEntities         = std::make_unique<entity::global_info[]>(xecs::settings::max_entities_v);
        int                                                 m_EmptyHead         = 0;

        inline static xecs::tools::bits                     s_ShareBits         {};
        inline static xecs::tools::bits                     s_DataBits          {};
        inline static xecs::tools::bits                     s_TagsBits          {};
        inline static xecs::tools::bits                     s_ExclusiveTagsBits {};
        inline static int                                   s_UniqueID          = 0;
        inline static bits_to_info_array                    s_BitsToInfo        {};
        inline static int                                   s_nTypes            = 0;
        inline static bool                                  s_isLocked          = false;
    };
}