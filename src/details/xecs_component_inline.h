namespace xecs::component
{
    namespace details
    {
        template< typename T_COMPONENT >
        consteval info CreateInfo(void) noexcept
        {
            static_assert( xecs::component::type::is_valid_v<T_COMPONENT> );
            return info
            {
                .m_Guid         = std::is_same_v<xecs::component::entity,T_COMPONENT>
                                    ? type::guid{ nullptr }
                                    : T_COMPONENT::typedef_v.m_Guid.m_Value
                                        ? T_COMPONENT::typedef_v.m_Guid
                                        : type::guid{ __FUNCSIG__ }
            ,   .m_BitID        = info::invalid_id_v
            ,   .m_TypeID       = T_COMPONENT::typedef_v.id_v
            ,   .m_Size         = static_cast<std::uint32_t>(sizeof(T_COMPONENT))
            ,   .m_pConstructFn = std::is_trivially_constructible_v<T_COMPONENT>
                                    ? nullptr
                                    : []( std::byte* p ) noexcept
                                    {
                                        new(p) T_COMPONENT;
                                    }
            ,   .m_pDestructFn  = std::is_trivially_destructible_v<T_COMPONENT>
                                    ? nullptr
                                    : []( std::byte* p ) noexcept
                                    {
                                        std::destroy_at(reinterpret_cast<T_COMPONENT*>(p));
                                    }
            ,   .m_pMoveFn      = std::is_trivially_move_assignable_v<T_COMPONENT>
                                    ? nullptr
                                    : []( std::byte* p1, std::byte* p2 ) noexcept
                                    {
                                        *reinterpret_cast<T_COMPONENT*>(p1) = std::move(*reinterpret_cast<T_COMPONENT*>(p2));
                                    }
            ,   .m_pName        = T_COMPONENT::typedef_v.m_pName
                                    ? T_COMPONENT::typedef_v.m_pName
                                    : __FUNCSIG__
            };
        }

        //
        // Sort component types
        //
        template< typename T_A, typename T_B >
        struct smaller_component
        {
            constexpr static bool value = xecs::component::info_v<T_A>.m_Guid < xecs::component::info_v<T_B>.m_Guid;
        };

        template< typename T_TUPLE >
        requires(xcore::types::is_specialized_v<std::tuple, T_TUPLE>)
        using sort_tuple_t = xcore::types::tuple_sort_t<details::smaller_component, T_TUPLE>;

        template< typename T_TUPLE >
        static constexpr auto sorted_info_array_v = []<typename...T>(std::tuple<T...>*) constexpr
        {
            return std::array{ &component::info_v<xecs::component::entity>, &component::info_v<T>... };
        }(xcore::types::null_tuple_v< sort_tuple_t<T_TUPLE> > );

        template< typename...T_TUPLES_OR_COMPONENTS >
        using combined_t = xcore::types::tuple_cat_t
        <   std::conditional_t
            <
                xcore::types::is_specialized_v<std::tuple, T_TUPLES_OR_COMPONENTS>
            ,   T_TUPLES_OR_COMPONENTS
            ,   std::tuple<T_TUPLES_OR_COMPONENTS>
            > ...
        >;
    }

    //------------------------------------------------------------------------------

    template< typename T_COMPONENT >
    requires (xecs::component::type::is_valid_v<T_COMPONENT>)
    void mgr::RegisterComponent(void) noexcept
    {
        if (component::info_v<T_COMPONENT>.m_BitID == info::invalid_id_v)
            component::info_v<T_COMPONENT>.m_BitID = m_UniqueID++;
    }
}
