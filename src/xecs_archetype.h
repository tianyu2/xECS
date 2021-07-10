namespace xecs::archetype
{
    using guid = xcore::guid::unit<64, struct archetype_tag>;

    inline
    guid ComputeGuidFromInfos( std::span<const xecs::component::type::info* const> Infos ) noexcept
    {
        xecs::tools::bits Bits{};
        for( const auto& e : Infos ) Bits.setBit( e->m_BitID );
        return guid{ Bits.GenerateUniqueID() };
    }

    //
    // ARCHETYPE INSTANCE
    //
    struct instance final
    {
        struct events
        {
            xecs::event::instance<xecs::component::entity&>      m_OnEntityCreated;
            xecs::event::instance<xecs::component::entity&>      m_OnEntityDestroyed;
            xecs::event::instance<xecs::component::entity&>      m_OnEntityMovedIn;
            xecs::event::instance<xecs::component::entity&>      m_OnEntityMovedOut;
            xecs::event::instance<instance&, pool::family&>      m_OnPoolFamilyCreated;
            xecs::event::instance<instance&, pool::family&>      m_OnPoolFamilyDestroy;
            std::array<xecs::event::instance<xecs::component::entity&>, xecs::settings::max_components_per_entity_v> m_OnComponentUpdated;
        };

                                instance                ( const instance& 
                                                        ) = delete;
        inline                  instance                ( xecs::archetype::mgr& Mgr 
                                                        ) noexcept;
        
        inline
        void                    Initialize              ( archetype::guid                                     Guid
                                                        , const tools::bits&                                  AllComponentsBits
                                                        ) noexcept;
        template< typename T >
        T&                      getShareComponent       ( xecs::pool::family& Family 
                                                        ) noexcept;
        template
        < typename...T_SHARE_COMPONENTS
        > requires
        ( ((xecs::component::type::is_valid_v<T_SHARE_COMPONENTS>) && ...)
            && xecs::tools::all_components_are_share_types_v<T_SHARE_COMPONENTS...>
        ) __inline
        xecs::pool::family&     getOrCreatePoolFamily   ( T_SHARE_COMPONENTS&&... Components
                                                        ) noexcept;
        inline
        xecs::pool::family&     getOrCreatePoolFamily   ( std::span< const xecs::component::type::info* const>  TypeInfos
                                                        , std::span< std::byte* >                               MoveData
                                                        ) noexcept;
        inline
        xecs::pool::family&     getOrCreatePoolFamilyFromSameArchetype
                                                        ( xecs::pool::family&                                   FromFamily
                                                        , std::span<const int>                                  IndexRemaps
                                                        , std::span< const xecs::component::type::info* const>  TypeInfos
                                                        , std::span< std::byte* >                               MoveData
                                                        , std::span< const xecs::component::entity >            EntitySpan
                                                        , std::span< const xecs::component::type::share::key >  Keys
                                                        ) noexcept;
        inline
        xecs::pool::family&     CreateNewPoolFamily     ( xecs::pool::family::guid                          Guid
                                                        , std::span<xecs::component::entity>                ShareEntityList
                                                        , std::span<xecs::component::type::share::key>      ShareKeyList
                                                        ) noexcept;
        inline
        xecs::pool::family&     getOrCreatePoolFamilyFromDifferentArchetype
                                                        ( xecs::component::entity        Entity
                                                        ) noexcept;
        template
        < typename T_CALLBACK = xecs::tools::empty_lambda
        > requires
        ( xecs::tools::assert_function_return_v<T_CALLBACK, void>
        ) 
        void                    CreateEntity            ( xecs::pool::family&                                   PoolFamily
                                                        , int                                                   Count
                                                        , T_CALLBACK&&                                          Function = xecs::tools::empty_lambda{}
                                                        ) noexcept;
        __inline
        xecs::component::entity CreateEntity            ( xecs::pool::family&                                   PoolFamily
                                                        , std::span< const xecs::component::type::info* const>  Infos
                                                        , std::span< std::byte* >                               MoveData
                                                        ) noexcept;
        __inline
        xecs::component::entity CreateEntity            ( std::span< const xecs::component::type::info* const>  Infos
                                                        , std::span< std::byte* >                               MoveData
                                                        ) noexcept;
        template
        < typename T_CALLBACK
        > requires
        ( xecs::tools::assert_function_return_v<T_CALLBACK, void>
            && xecs::tools::assert_function_args_have_no_share_or_tag_components_v<T_CALLBACK>
            && xecs::tools::assert_function_args_have_only_non_const_references_v<T_CALLBACK>
        ) __inline
        xecs::component::entity CreateEntity            ( T_CALLBACK&& Function
                                                        ) noexcept;
        template
        < typename T_CALLBACK = xecs::tools::empty_lambda
        > requires
        ( xecs::tools::assert_function_return_v<T_CALLBACK, void>
            && xecs::tools::assert_function_args_have_no_share_or_tag_components_v<T_CALLBACK>
            && xecs::tools::assert_function_args_have_only_non_const_references_v<T_CALLBACK>
        ) __inline
        xecs::component::entity CreateEntity            ( xecs::pool::family&   PoolFamily
                                                        , T_CALLBACK&&          Function = xecs::tools::empty_lambda{}
                                                        ) noexcept;
        template
        < typename T_CALLBACK = xecs::tools::empty_lambda
        > requires
        ( false == xecs::tools::function_has_share_component_args_v<T_CALLBACK>
            && xecs::tools::assert_function_return_v<T_CALLBACK, void>
            && xecs::tools::assert_standard_setter_function_v<T_CALLBACK>
        ) __inline
        void                    CreateEntities          ( int           Count
                                                        , T_CALLBACK&&  Function = xecs::tools::empty_lambda{}
                                                        ) noexcept;

        template
        < typename T_CALLBACK = xecs::tools::empty_lambda
        > requires
        ( true == xecs::tools::function_has_share_component_args_v<T_CALLBACK>
            && xecs::tools::assert_function_return_v<T_CALLBACK, void>
            && xecs::tools::assert_standard_setter_function_v<T_CALLBACK>
        ) __inline
        void                    CreateEntities          ( int           Count
                                                        , T_CALLBACK&&  Function = xecs::tools::empty_lambda{}
                                                        ) noexcept;


        template
        < typename T_CALLBACK = xecs::tools::empty_lambda
        > requires
        ( xecs::tools::assert_function_return_v<T_CALLBACK, void>
            && xecs::tools::assert_function_args_have_no_share_or_tag_components_v<T_CALLBACK>
            && xecs::tools::assert_function_args_have_only_non_const_references_v<T_CALLBACK>
        ) __inline
        void                    CreateEntities          ( int                   Count
                                                        , xecs::pool::family    PoolFamily
                                                        , T_CALLBACK&&          Function = xecs::tools::empty_lambda{}
                                                        ) noexcept;
        inline
        void                    DestroyEntity           ( xecs::component::entity& Entity
                                                        ) noexcept;
        template
        < typename T_FUNCTION = xecs::tools::empty_lambda
        > requires
        ( xecs::tools::assert_function_return_v<T_FUNCTION, void>
            && xecs::tools::assert_function_args_have_no_share_or_tag_components_v<T_FUNCTION>
            && xecs::tools::assert_function_args_have_only_non_const_references_v<T_FUNCTION>
        ) [[nodiscard]] xecs::component::entity
                                MoveInEntity            ( xecs::component::entity&  Entity
                                                        , xecs::pool::family&       PoolFamily
                                                        , T_FUNCTION&&              Function = xecs::tools::empty_lambda{} 
                                                        ) noexcept;
        template
        < typename T_FUNCTION = xecs::tools::empty_lambda
        > requires
        ( xecs::tools::assert_function_return_v<T_FUNCTION, void>
            && xecs::tools::assert_function_args_have_no_share_or_tag_components_v<T_FUNCTION>
            && xecs::tools::assert_function_args_have_only_non_const_references_v<T_FUNCTION>
        ) [[nodiscard]] xecs::component::entity
                                MoveInEntity            ( xecs::component::entity&  Entity
                                                        , T_FUNCTION&&              Function = xecs::tools::empty_lambda{} 
                                                        ) noexcept;

        __inline
        void                    UpdateStructuralChanges ( void
                                                        ) noexcept {}

        __inline
        void                    UpdateStructuralChanges ( pool::family& poolFamily
                                                        ) noexcept;

        using share_archetypes_array = std::array<std::shared_ptr<xecs::archetype::instance>,   xecs::settings::max_components_per_entity_v >;

        xecs::archetype::mgr&               m_Mgr;
        guid                                m_Guid                      {};
        xecs::tools::bits                   m_ComponentBits             {};
        xecs::tools::bits                   m_ExclusiveTagsBits         {};
        std::uint8_t                        m_nDataComponents           {};
        std::uint8_t                        m_nShareComponents          {};
        events                              m_Events                    {};
        std::unique_ptr<pool::family>       m_FamilyHead                {}; // Please note that the Default family pool will be in this list so we can't let the unique pointer free it
        pool::family                        m_DefaultPoolFamily         {};
        xecs::component::entity::info_array m_InfoData                  {}; // rename to InfoArray
        instance*                           m_pPendingStructuralChanges {};
        share_archetypes_array              m_ShareArchetypesArray      {};
    };
}
