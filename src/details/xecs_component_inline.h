namespace xecs::component
{
    namespace details
    {
        template< typename T >
        consteval info CreateInfo(void) noexcept
        {
            return info
            {
                .m_UID          = info::invalid_id_v
            ,   .m_Size         = static_cast<std::uint32_t>(sizeof(T))
            ,   .m_Guid         = std::is_same_v<xecs::component::entity,T> ? nullptr : info::guid{ __FUNCSIG__ }
            ,   .m_pConstructFn = std::is_trivially_constructible_v<T>      ? nullptr
                                                                            : []( std::byte* p ) noexcept
                                                                            {
                                                                                new(p) T;
                                                                            }
            ,   .m_pDestructFn  = std::is_trivially_destructible_v<T>       ? nullptr
                                                                            : []( std::byte* p ) noexcept
                                                                            {
                                                                                std::destroy_at(reinterpret_cast<T*>(p));   
                                                                            }
            ,   .m_pMoveFn      = std::is_trivially_move_assignable_v<T>    ? nullptr
                                                                            : []( std::byte* p1, std::byte* p2 ) noexcept
                                                                            {
                                                                                *reinterpret_cast<T*>(p1) = std::move(*reinterpret_cast<T*>(p2));
                                                                            }
            };
        }

        //
        // Sort component types
        //
        template< typename T_A, typename T_B >
        struct smaller_component
        {
            constexpr static bool value = info_v<T_A>.m_Guid < info_v<T_B>.m_Guid;
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
    void mgr::RegisterComponent(void) noexcept
    {
        if (component::info_v<T_COMPONENT>.m_UID == info::invalid_id_v)
            component::info_v<T_COMPONENT>.m_UID = m_UniqueID++;
    }
}
