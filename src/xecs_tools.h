namespace xecs::tools
{
    using empty_lambda = decltype([]() {});

    //------------------------------------------------------------------------------

    template
    < typename T_CALLABLE
    , typename T_RETURN_TYPE
    > requires
    ( xcore::function::is_callable_v<T_CALLABLE> 
    )
    constexpr auto function_return_v = std::is_same_v<typename xcore::function::traits<T_CALLABLE>::return_type, T_RETURN_TYPE>;

    //------------------------------------------------------------------------------

    template
    < typename T_CALLABLE
    > constexpr
    auto function_args_have_no_share_or_tag_components_v = []<typename...T_ARGS>(std::tuple<T_ARGS...>*) constexpr noexcept
    {
        static_assert( xcore::function::is_callable_v<T_CALLABLE> );
        static_assert( false == xcore::types::tuple_has_duplicates_v< xcore::function::traits<T_CALLABLE>::args_tuple > );
        static_assert( ((xecs::component::type::info_v<T_ARGS>.m_TypeID != xecs::component::type::id::SHARE
                       && xecs::component::type::info_v<T_ARGS>.m_TypeID != xecs::component::type::id::TAG) && ...));
        return true;
    }( xcore::types::null_tuple_v< typename xcore::function::template traits<T_CALLABLE>::args_tuple> );
    
    //------------------------------------------------------------------------------

    template
    < typename T_CALLABLE
    > constexpr
    auto function_args_have_only_non_const_references_v = []<typename...T_ARGS>( std::tuple<T_ARGS...>*) constexpr noexcept
    {
        static_assert( xcore::function::is_callable_v<T_CALLABLE> );
        static_assert( false == xcore::types::tuple_has_duplicates_v< xcore::function::traits<T_CALLABLE>::args_tuple >);
        static_assert( ((std::is_same_v< xcore::types::decay_full_t<T_ARGS>&, T_ARGS>) && ...) );
        return true;
    }( xcore::types::null_tuple_v< xcore::function::traits<T_CALLABLE>::args_tuple> );

    //------------------------------------------------------------------------------

    template
    < typename... T_SHARE_COMPONENTS
    > constexpr
    auto all_components_are_share_types_v = []() constexpr noexcept
    {
        static_assert( false == xcore::types::tuple_has_duplicates_v< std::tuple<T_SHARE_COMPONENTS...> > );
        static_assert( ((xecs::component::type::info_v<T_SHARE_COMPONENTS>.m_TypeID == xecs::component::type::id::SHARE) && ...) );
        return true;
    }();
        

    //------------------------------------------------------------------------------

    namespace details
    {
        template< typename T>
        struct as_tuple
        {
            using type = std::tuple<T>;
        };

        template< typename...T >
        struct as_tuple<std::tuple<T...>>
        {
            using type = std::tuple<T...>;
        };
    }

    template< typename... T >
    using united_tuple = xcore::types::tuple_cat_t< typename details::as_tuple<T>::type ... >;

    struct bits final
    {
        constexpr __inline
        void        setBit              ( int Bit 
                                        ) noexcept;

        constexpr __inline
        bool        getBit              ( int Bit
                                        ) const noexcept;

        constexpr
        bool        Superset            ( const bits& B 
                                        ) const noexcept;

        constexpr
        bool        Subset              ( const bits& B 
                                        ) const noexcept;

        constexpr
        bool        Equals              ( const bits& B 
                                        ) const noexcept;

        constexpr
        void        clearBit            ( int Bit
                                        ) noexcept;

        template
        < typename... T_COMPONENTS
        > constexpr __inline
        void        AddFromComponents   ( void 
                                        ) noexcept;

        std::array<std::uint64_t, ((xecs::settings::max_component_types_v-1)/64)+1> m_Bits{};
    };

    //-------------------------------------------------------------------------------------------------
    constexpr
    bool HaveAllComponents(const bits& Bits, std::span<const xecs::component::type::info* const > Span ) noexcept
    {
        for( auto& e : Span )
        {
            if( Bits.getBit( e->m_BitID ) == false ) return false;
        }
        return true;
    }

    //-------------------------------------------------------------------------------------------------
    template< typename... T_COMPONENTS >
    constexpr
    bool HaveAllComponents( const bits& Bits, std::tuple<T_COMPONENTS...>* ) noexcept
    {
        return ((Bits.getBit(xecs::component::type::info_v<T_COMPONENTS>.m_BitID ) && ... ));
    }
}
