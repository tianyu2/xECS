namespace xecs::query
{
    namespace details
    {
        //------------------------------------------------------------------------------------

        template
        < typename  T_FUNCTION
        , bool      T_HAS_SHARES_V = xecs::tools::function_has_share_component_args_v<T_FUNCTION>
        >
        struct data_pack;

        //------------------------------------------------------------------------------------

        template< typename T_FUNCTION>
        struct data_pack< T_FUNCTION, false >
        {
            static_assert(xcore::function::is_callable_v<T_FUNCTION>);

            using                   func                = xcore::function::traits<T_FUNCTION>;
            static constexpr bool   has_shares_v        = false;
            using                   data_tuple_unfilter = typename func::args_tuple;
            template< typename T >
            using                   universal_t         = xcore::types::decay_full_t<T>*;
            using                   data_tuple          = std::invoke_result_t
            < decltype
                (   []<typename...T>(std::tuple<T...>*) -> xcore::types::tuple_cat_t< std::tuple<universal_t<T>> ... >{}
                )
            , decltype(xcore::types::null_tuple_v<typename func::args_tuple>)
            >;
            
            data_tuple  m_DataTuple;
        };

        //------------------------------------------------------------------------------------

        template< typename T_FUNCTION>
        struct data_pack< T_FUNCTION, true >
        {
            static_assert(xcore::function::is_callable_v<T_FUNCTION>);

            using func = xcore::function::traits<T_FUNCTION>;

            static constexpr bool has_shares_v = true;

            using share_tuple_unfilter = std::invoke_result_t
            < decltype
                (   []<typename...T>(std::tuple<T...>*)->xcore::types::tuple_cat_t
                    <
                        std::conditional_t
                        < xecs::component::type::info_v<T>.m_TypeID == xecs::component::type::id::SHARE
                        , std::tuple<T>
                        , std::tuple<>
                        > ...
                    >{}
                )
            , decltype(xcore::types::null_tuple_v<typename func::args_tuple>)
            >;

            using share_tuple_full_decay = xcore::types::tuple_decay_full_t<share_tuple_unfilter>;
            
            using share_tuple_changables_unfilter = std::invoke_result_t
            < decltype
                (   []<typename...T>(std::tuple<T...>*) -> xcore::types::tuple_cat_t
                    <
                        std::conditional_t
                        < std::is_const_v<T>
                        , std::tuple<>
                        , std::tuple<T>
                        > ...
                    >{}
                )
            , decltype(xcore::types::null_tuple_v<share_tuple_unfilter>)
            >;

            static constexpr auto share_count_v             = std::tuple_size_v<share_tuple_unfilter>;
            static constexpr auto nonconst_share_count_v    = std::tuple_size_v<share_tuple_changables_unfilter>;
            static constexpr auto const_share_count_v       = share_count_v - nonconst_share_count_v;
            static constexpr auto data_count_v              = func::arg_count_v - share_count_v;

            static_assert(share_count_v);


            using data_tuple_unfilter = std::invoke_result_t
            < decltype
                (   []<typename...T>(std::tuple<T...>*) -> xcore::types::tuple_cat_t
                    <
                        std::conditional_t
                        < xecs::component::type::info_v<T>.m_TypeID == xecs::component::type::id::DATA
                        , std::tuple<T>
                        , std::tuple<>
                        > ...
                    >{}
                )
            , decltype(xcore::types::null_tuple_v<typename func::args_tuple>)
            >;

            template< typename T >
            using universal_t = std::conditional_t
            <   xecs::component::type::info_v<T>.m_TypeID == xecs::component::type::id::DATA
            ,   xcore::types::decay_full_t<T>*
            ,   std::conditional_t
                <   std::is_reference_v<T>
                ,   std::conditional_t
                    <   std::is_const_v<T>
                    ,   std::remove_reference_t<T>*
                    ,   std::remove_reference_t<T>
                    >
                ,   std::conditional_t
                    <   std::is_const_v<T>
                    ,   T
                    ,   std::remove_pointer_t<T>
                    >
                >
            >;

            using data_tuple = std::invoke_result_t
            < decltype
                (   []<typename...T>(std::tuple<T...>*) -> xcore::types::tuple_cat_t< std::tuple<universal_t<T>> ... >{}
                )
            , decltype(xcore::types::null_tuple_v<typename func::args_tuple>)
            >;

            using share_tuple = std::invoke_result_t
            < decltype
                (   []<typename...T>(std::tuple<T...>*) -> xcore::types::tuple_cat_t
                    <
                        std::conditional_t
                        < xecs::component::type::info_v<T>.m_TypeID == xecs::component::type::id::SHARE
                        , std::tuple<universal_t<T>>
                        , std::tuple<>
                        > ...
                    >{}
                )
            , decltype(xcore::types::null_tuple_v<typename func::args_tuple>)
            >;

            using share_tuple_pointers =std::invoke_result_t
            < decltype
                (   []<typename...T>(std::tuple<T...>*) -> xcore::types::tuple_cat_t
                    <
                        std::conditional_t
                        < xecs::component::type::info_v<T>.m_TypeID == xecs::component::type::id::SHARE && (false == std::is_pointer_v<universal_t<T>>)
                        , std::tuple<xcore::types::decay_full_t<T>*>
                        , std::tuple<>
                        > ...
                    >{}
                )
            , decltype(xcore::types::null_tuple_v<typename func::args_tuple>)
            >;
            static_assert(nonconst_share_count_v == std::tuple_size_v<share_tuple_pointers>);


            __inline data_pack() noexcept;

            const xecs::tools::bits                                                 m_UpdatedComponentsBits;
            xecs::tools::bits                                                       m_ArchetypeShareBits;
            std::array<std::uint8_t,                        share_count_v>          m_RemapIndices;
            std::array<xecs::component::type::share::key,   share_count_v>          m_CacheShareKeys;
            share_tuple_pointers                                                    m_CacheSharePointers;
            share_tuple                                                             m_UniversalTuple;
            data_tuple                                                              m_DataTuple;
            std::uint64_t                                                           m_KeyCheckSum;
            int                                                                     m_EntityIndex;
        };
    }

    //------------------------------------------------------------------------------------

    template< typename T_FUNCTION >
    struct iterator : details::data_pack<T_FUNCTION>
    {
        using parent_t  = details::data_pack<T_FUNCTION>;
        using func_t    = xcore::function::traits<T_FUNCTION>;
        using ret_t     = func_t::return_type;

        __inline
        void        ForeachArchetype                ( const xecs::archetype::instance& Archetype 
                                                    ) noexcept;
        __inline
        void        ForeachFamilyPool               ( const xecs::component::mgr&   ComponentMgr
                                                    , const xecs::pool::family&     Family 
                                                    ) noexcept;
        __inline
        void        ForeachPool                     ( const xecs::tools::bits&      ArchetypeBits
                                                    , xecs::pool::instance&         Pool 
                                                    ) noexcept;
        __inline
        ret_t       CallUserFunction                ( xecs::archetype::instance&    Archetype
                                                    , xecs::pool::family&           Family
                                                    , xecs::pool::instance&         Pool
                                                    , T_FUNCTION&&                  Function 
                                                    ) noexcept;
    };
}