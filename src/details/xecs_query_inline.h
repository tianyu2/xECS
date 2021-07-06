namespace xecs::query
{
    //---------------------------------------------------------------------------

    bool instance::Compare(const tools::bits& ArchetypeBits) const noexcept
    {
        std::uint64_t c = 0;
        std::uint64_t a = 0;

        for (int i = 0, end = static_cast<int>(ArchetypeBits.m_Bits.size()); i != end; ++i)
        {
            if (m_NoneOf.m_Bits[i] & ArchetypeBits.m_Bits[i])
                return false;

            if ((m_Must.m_Bits[i] & ArchetypeBits.m_Bits[i]) != m_Must.m_Bits[i])
                return false;

            c |= (m_OneOf.m_Bits[i] & ArchetypeBits.m_Bits[i]);
            a |= m_OneOf.m_Bits[i];
        }

        // a | c
        // ------
        // 0   0 | 1 true 
        // 0   1 | x error
        // 1   0 | 0 false
        // 1   1 | 1 true 
        assert(!(!a && c));
        return !a || c;
    }

    //---------------------------------------------------------------------------

    template
    < typename T_FUNCTION
    > 
    void instance::AddQueryFromFunction( void ) noexcept
    {
        using func_traits = xcore::function::traits<T_FUNCTION>;
        static_assert( func_traits::arg_count_v > 0 );

        [&] < typename...T_COMPONENTS >(std::tuple<T_COMPONENTS...>*) constexpr noexcept
        {
            ( [&]<typename T_C>(std::tuple<T_C>*)
            {
                if constexpr (std::is_pointer_v<T_C>)
                {
                    m_OneOf.AddFromComponents<T_C>();
                }
                else if constexpr (std::is_reference_v<T_C>)
                {
                    m_Must.AddFromComponents<T_C>();
                }
                else
                {
                    static_assert(xcore::types::always_false_v<T_C>);
                }
            }(xcore::types::make_null_tuple_v<T_COMPONENTS>)
            , ...
            );

        }( xcore::types::null_tuple_v<func_traits::args_tuple> );
    }

    //---------------------------------------------------------------------------

    template
    < typename T_FUNCTION
    > requires
    ( xcore::function::is_callable_v<T_FUNCTION>
    )
    void instance::AddQueryFromFunction(T_FUNCTION&&) noexcept
    {
        AddQueryFromFunction<T_FUNCTION>();
    }

    //---------------------------------------------------------------------------

    template
    < typename... T_QUERIES
    > 
    void instance::AddQueryFromTuple(std::tuple<T_QUERIES...>*) noexcept
    {
        ( [&]<template<typename...> class T_QTYPE, typename... T_COMPONENTS>(T_QTYPE<T_COMPONENTS...>*) constexpr noexcept
        {
            using t = T_QTYPE<T_COMPONENTS...>;
            if constexpr (std::is_same_v< t, xecs::query::must<T_COMPONENTS...>>)
            {
                m_Must.AddFromComponents<T_COMPONENTS...>();
            }
            else if constexpr (std::is_same_v< t, xecs::query::one_of<T_COMPONENTS...>>)
            {
                m_OneOf.AddFromComponents<T_COMPONENTS...>();
            }
            else if constexpr (std::is_same_v< t, xecs::query::none_of<T_COMPONENTS...>>)
            {
                m_NoneOf.AddFromComponents<T_COMPONENTS...>();
            }
            else
            {
                static_assert(xcore::types::always_false_v<t>);
            }
        }( xcore::types::null_tuple_v<T_QUERIES> )
        , ...
        );
    }

    /*
    //---------------------------------------------------------------------------
    template
    < typename... T_SHARE_COMPONENTS
    > requires
    ( xecs::tools::assert_all_components_are_share_types_v<T_SHARE_COMPONENTS>
    )
    instance& instance::AddFilter( T_SHARE_COMPONENTS&&... ShareComponents ) noexcept
    {
        ((AddFilter(xecs::component::type::info_v<T_SHARE_COMPONENTS>
                  , xecs::component::type::info_v<T_SHARE_COMPONENTS>.m_pComputeKeyFn(reinterpret_cast<std::byte*>(&ShareComponents)))
         ), ... );
        return *this;
    }

    //---------------------------------------------------------------------------
    inline
    instance& instance::AddFilter ( const xecs::component::type::info& ComponentTypeInfo
                                  , xecs::component::type::share::key  PartialKey
                                  ) noexcept
    {
        assert(m_nShareFilters < max_share_component_to_filter_v);

        m_Must.setBit(ComponentTypeInfo.m_BitID);
        m_ShareFilterTypes[m_nShareFilters]         = &ComponentTypeInfo;
        m_ShareFilterPartialKeys[m_nShareFilters]   = PartialKey;
        m_nShareFilters++;
    }
    */
}
    
