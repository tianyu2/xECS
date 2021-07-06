namespace xecs::query
{
    template< typename... T_COMPONENTS >
    struct must final {};

    template< typename... T_COMPONENTS >
    struct one_of final {};

    template< typename... T_COMPONENTS >
    struct none_of final {};

    struct instance final
    {
        tools::bits     m_Must;
        tools::bits     m_OneOf;
        tools::bits     m_NoneOf;

        inline 
        bool                    Compare                 ( const tools::bits& ArchetypeBits
                                                        ) const noexcept;
        template
        < typename T_FUNCTION
        > 
        void                    AddQueryFromFunction    ( void
                                                        ) noexcept;
        template
        < typename T_FUNCTION
        > requires
        ( xcore::function::is_callable_v<T_FUNCTION> 
        )
        void                    AddQueryFromFunction    ( T_FUNCTION&& 
                                                        ) noexcept;
        template
        < typename... T_QUERIES
        >
        void                    AddQueryFromTuple       ( std::tuple<T_QUERIES...>*
                                                        ) noexcept;
    };

    struct share_filter
    {
        template
        < typename T_SHARE_COMPONENT
        > requires
        ( xecs::tools::assert_all_components_are_share_types_v<T_SHARE_COMPONENT>
        ) constexpr
        share_filter(T_SHARE_COMPONENT&& ShareComponent ) noexcept
        : m_ComponentInfo{ xecs::component::type::info_v<T_SHARE_COMPONENT>}
        , m_ShareFilterPartialKeys{ xecs::component::type::info_v<T_SHARE_COMPONENT>.m_pComputeKeyFn(reinterpret_cast<std::byte*>(&ShareComponent)) }
        {}

        const xecs::component::type::info&  m_ComponentInfo;
        xecs::component::type::share::key   m_ShareFilterPartialKeys;
    };
}
