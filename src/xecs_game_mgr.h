namespace xecs::game_mgr
{
    //---------------------------------------------------------------------------

    struct entity_info final
    {
        xecs::archetype::instance*      m_pArchetype    {};
        int                             m_PoolIndex     {-1};
        component::entity::validation   m_Validation    {};
    };

    //---------------------------------------------------------------------------

    struct instance final
    {
                                            instance                ( const instance& 
                                                                    ) = delete;
        inline                              instance                ( void
                                                                    ) noexcept;

        template
        < typename...T_SYSTEMS
        > requires
        ( std::derived_from< T_SYSTEMS, xecs::system::instance> && ...
        )
        void                                RegisterSystems         ( void
                                                                    ) noexcept;
        template
        < typename...T_COMPONENTS
        > 
        void                                RegisterComponents      ( void
                                                                    ) noexcept;
        inline
        xecs::component::entity             AllocNewEntity          ( int                        PoolIndex
                                                                    , xecs::archetype::instance& Archetype 
                                                                    ) noexcept;
        inline
        const entity_info&                  getEntityDetails        ( xecs::component::entity Entity 
                                                                    ) const noexcept;
        inline
        void                                DeleteEntity            ( xecs::component::entity& Entity 
                                                                    ) noexcept;
        template
        < typename T_FUNCTION = xecs::tools::empty_lambda
        > requires
        ( xcore::function::is_callable_v<T_FUNCTION>
        ) [[nodiscard]] xecs::component::entity
                                            AddOrRemoveComponents   ( xecs::component::entity                       Entity
                                                                    , std::span<const xecs::component::info* const> Add
                                                                    , std::span<const xecs::component::info* const> Sub
                                                                    , T_FUNCTION&&                                  Function = xecs::tools::empty_lambda{}
                                                                    ) noexcept;
        template
        <   typename T_TUPLE_ADD
        ,   typename T_TUPLE_SUBTRACT   = std::tuple<>
        ,   typename T_FUNCTION         = xecs::tools::empty_lambda
        > requires
        ( xcore::function::is_callable_v<T_FUNCTION>
        && xcore::types::is_specialized_v<std::tuple, T_TUPLE_ADD>
        && xcore::types::is_specialized_v<std::tuple, T_TUPLE_SUBTRACT>
        ) [[nodiscard]] xecs::component::entity
                                            AddOrRemoveComponents   ( xecs::component::entity   Entity
                                                                    , T_FUNCTION&&              Function = xecs::tools::empty_lambda{}
                                                                    ) noexcept;
        inline
        void                                DeleteGlobalEntity      ( std::uint32_t              GlobalIndex
                                                                    , xecs::component::entity&   SwappedEntity 
                                                                    ) noexcept;
        inline
        void                                DeleteGlobalEntity      ( std::uint32_t GlobalIndex
                                                                    ) noexcept;
        inline
        void                                MovedGlobalEntity       ( std::uint32_t             PoolIndex
                                                                    , xecs::component::entity&  SwappedEntity
                                                                    ) noexcept;
        template
        < typename T_FUNCTION = xecs::tools::empty_lambda
        > requires
        ( xcore::function::is_callable_v<T_FUNCTION>
        ) __inline
        bool                                findEntity              ( xecs::component::entity Entity
                                                                    , T_FUNCTION&&            Function = xecs::tools::empty_lambda{}
                                                                    ) const noexcept;
        inline
        std::vector<archetype::instance*>   Search                  ( std::span<const component::info* const> Types 
                                                                    ) const noexcept;

        template
        < typename... T_COMPONENTS
        >
        std::vector<archetype::instance*>   Search                  ( void 
                                                                    ) const noexcept;
        template
        < typename... T_COMPONENTS
        >
        std::vector<archetype::instance*>   Search                  ( const xecs::query::instance& Query 
                                                                    ) const noexcept;
        inline
        archetype::instance&                getOrCreateArchetype    ( std::span<const component::info* const> Types 
                                                                    ) noexcept;

        template
        < typename... T_COMPONENTS
        >
        archetype::instance&                getOrCreateArchetype    ( void 
                                                                    ) noexcept;
        template
        <   typename T_FUNCTION
        > requires
        (   xcore::function::is_callable_v<T_FUNCTION> 
        &&  std::is_same_v< bool, typename xcore::function::traits<T_FUNCTION>::return_type >
        ) __inline
        void                                Foreach                 ( const std::vector<xecs::archetype::instance*>& List
                                                                    , T_FUNCTION&&                                   Function 
                                                                    ) const noexcept;
        template
        < typename T_FUNCTION
        > requires
        ( xcore::function::is_callable_v<T_FUNCTION>
        && std::is_same_v< void, typename xcore::function::traits<T_FUNCTION>::return_type >
        ) __inline
        void                                Foreach                 ( const std::vector<xecs::archetype::instance*>& List
                                                                    , T_FUNCTION&&                                   Function 
                                                                    ) const noexcept;
        inline
        void                                Run                     ( void 
                                                                    ) noexcept;

        xecs::system::mgr                                   m_SystemMgr         {};
        xecs::component::mgr                                m_ComponentMgr      {};
        std::vector<std::unique_ptr<archetype::instance>>   m_lArchetype        {};
        std::vector<tools::bits>                            m_lArchetypeBits    {};
        std::unique_ptr<entity_info[]>                      m_lEntities         = std::make_unique<entity_info[]>(xecs::settings::max_entities_v);
        int                                                 m_EmptyHead         = 0;
    };
}