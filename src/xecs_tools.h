namespace xecs::tools
{
    using empty_lambda = decltype([]() {});

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
        __inline 
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
        > __inline
        void        AddFromComponents   ( void 
                                        ) noexcept;

        std::array<std::uint64_t, ((xecs::settings::max_component_types_v-1)/64)+1> m_Bits{};
    };
}
