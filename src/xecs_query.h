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
}
