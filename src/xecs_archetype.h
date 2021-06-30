namespace xecs::archetype
{
    using guid = xcore::guid::unit<64, struct archetype_tag>;

    template< typename...T_TUPLES_OF_COMPONENTS_OR_COMPONENTS >
    constexpr static auto guid_v = []<typename...T>(std::tuple<T...>*) consteval
    {
        static_assert( ((xecs::component::type::is_valid_v<T>) && ... ) );
        return guid{ ((xecs::component::type::info_v<T>.m_Guid.m_Value) + ...) };
    }( xcore::types::null_tuple_v< xecs::tools::united_tuple<T_TUPLES_OF_COMPONENTS_OR_COMPONENTS...> > );

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
            std::array<xecs::event::instance<xecs::component::entity&>, xecs::settings::max_components_per_entity_v> m_OnComponentUpdated;
        };

                                instance                ( const instance& 
                                                        ) = delete;
        inline                  instance                ( xecs::archetype::mgr& Mgr 
                                                        ) noexcept;
        
        inline
        void                    Initialize              ( std::span<const xecs::component::type::info* const> Infos
                                                        , const tools::bits&                                  Bits 
                                                        ) noexcept;

        template
        < typename...T_SHARE_COMPONENTS
        > requires
        ( xecs::tools::all_components_are_share_types_v<T_SHARE_COMPONENTS...>
        ) __inline
        xecs::pool::family&     getOrCreatePoolFamily   ( T_SHARE_COMPONENTS&&... Components
                                                        ) noexcept;
        inline
        xecs::pool::family&     getOrCreatePoolFamily   ( std::span< const xecs::component::type::info* const>  TypeInfos
                                                        , std::span< std::byte* >                               MoveData
                                                        ) noexcept;

        template
        < typename T_CALLBACK = xecs::tools::empty_lambda
        > requires
        ( xecs::tools::function_return_v<T_CALLBACK, void>
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
        ( xecs::tools::function_return_v<T_CALLBACK, void>
            && xecs::tools::function_args_have_no_share_or_tag_components_v<T_CALLBACK>
            && xecs::tools::function_args_have_only_non_const_references_v<T_CALLBACK>
        ) __inline
        xecs::component::entity CreateEntity            ( T_CALLBACK&& Function
                                                        ) noexcept;
        template
        < typename T_CALLBACK = xecs::tools::empty_lambda
        > requires
        ( xecs::tools::function_return_v<T_CALLBACK, void>
            && xecs::tools::function_args_have_no_share_or_tag_components_v<T_CALLBACK>
            && xecs::tools::function_args_have_only_non_const_references_v<T_CALLBACK>
        ) __inline
        xecs::component::entity CreateEntity            ( xecs::pool::family&   PoolFamily
                                                        , T_CALLBACK&&          Function = xecs::tools::empty_lambda{}
                                                        ) noexcept;
        template
        < typename T_CALLBACK = xecs::tools::empty_lambda
        > requires
        ( xecs::tools::function_return_v<T_CALLBACK, void>
            && xecs::tools::function_args_have_no_share_or_tag_components_v<T_CALLBACK>
            && xecs::tools::function_args_have_only_non_const_references_v<T_CALLBACK>
        ) __inline
        void                    CreateEntities          ( int           Count
                                                        , T_CALLBACK&&  Function = xecs::tools::empty_lambda{}
                                                        ) noexcept;
        template
        < typename T_CALLBACK = xecs::tools::empty_lambda
        > requires
        ( xecs::tools::function_return_v<T_CALLBACK, void>
            && xecs::tools::function_args_have_no_share_or_tag_components_v<T_CALLBACK>
            && xecs::tools::function_args_have_only_non_const_references_v<T_CALLBACK>
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
        ( xecs::tools::function_return_v<T_FUNCTION, void>
            && xecs::tools::function_args_have_no_share_or_tag_components_v<T_FUNCTION>
            && xecs::tools::function_args_have_only_non_const_references_v<T_FUNCTION>
        ) [[nodiscard]] xecs::component::entity
                                MoveInEntity            ( xecs::component::entity&  Entity
                                                        , xecs::pool::family&       PoolFamily
                                                        , T_FUNCTION&&              Function = xecs::tools::empty_lambda{} 
                                                        ) noexcept;
        template
        < typename T_FUNCTION = xecs::tools::empty_lambda
        > requires
        ( xecs::tools::function_return_v<T_FUNCTION, void>
            && xecs::tools::function_args_have_no_share_or_tag_components_v<T_FUNCTION>
            && xecs::tools::function_args_have_only_non_const_references_v<T_FUNCTION>
        ) [[nodiscard]] xecs::component::entity
                                MoveInEntity            ( xecs::component::entity&  Entity
                                                        , T_FUNCTION&&              Function = xecs::tools::empty_lambda{} 
                                                        ) noexcept;

        using info_array             = std::array<const xecs::component::type::info*,           xecs::settings::max_components_per_entity_v >;
        using share_archetypes_array = std::array<std::shared_ptr<xecs::archetype::instance>,   xecs::settings::max_components_per_entity_v >;

        xecs::archetype::mgr&               m_Mgr;
        guid                                m_Guid                      {};
        xecs::tools::bits                   m_ComponentBits             {};
        std::uint8_t                        m_nDataComponents           {};
        std::uint8_t                        m_nShareComponents          {};
        events                              m_Events                    {};
        pool::family                        m_DefaultPoolFamily         {};
        info_array                          m_InfoData                  {}; // rename to InfoArray
        share_archetypes_array              m_ShareArchetypesArray      {};
    };

    //
    // ARCHETYPE MGR
    //
    struct mgr
    {
        struct events
        {
            xecs::event::instance<xecs::archetype::instance&>         m_OnNewArchetype;
        };

        inline                                  mgr                         ( xecs::game_mgr::instance& GameMgr 
                                                                            ) noexcept;
        inline
        std::shared_ptr<archetype::instance>    getOrCreateArchetype        ( std::span<const component::type::info* const> Types
                                                                            ) noexcept;



        // Pool family is all the share components of a certain type with a certain value plus the archetype guid
        using pool_family_map               = std::unordered_map<xecs::pool::family::guid,          xecs::pool::family*         >;
        using archetype_map                 = std::unordered_map<xecs::archetype::guid,             xecs::archetype::instance*  >;
        using share_component_entity_map    = std::unordered_map<xecs::component::type::share::key, xecs::component::entity     >;

        xecs::game_mgr::instance&                           m_GameMgr;
        events                                              m_Events                    {};
        share_component_entity_map                          m_ShareComponentEntityMap   {};
        archetype_map                                       m_ArchetypeMap              {};
        std::vector<std::shared_ptr<archetype::instance>>   m_lArchetype                {};
        std::vector<tools::bits>                            m_lArchetypeBits            {};
        pool_family_map                                     m_PoolFamily                {};
    };
}
