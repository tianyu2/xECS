namespace xecs::component
{
    namespace type::details
    {
        template< typename T_COMPONENT >
        consteval
        type::info CreateInfo( void ) noexcept
        {
            static_assert( xecs::component::type::is_valid_v<T_COMPONENT> );
            return type::info
            {
                .m_Guid             = std::is_same_v<xecs::component::entity, T_COMPONENT>
                                        ? type::guid{ nullptr }
                                        : T_COMPONENT::typedef_v.m_Guid.m_Value
                                        ? T_COMPONENT::typedef_v.m_Guid
                                        : type::guid{ __FUNCSIG__ }
            ,   .m_BitID            = info::invalid_bit_id_v
            ,   .m_Size             = xcore::types::static_cast_safe<std::uint16_t>(sizeof(T_COMPONENT))
            ,   .m_TypeID           = T_COMPONENT::typedef_v.id_v
            ,   .m_bGlobalScoped    = []{ if constexpr (T_COMPONENT::typedef_v.id_v == type::id::SHARE) return T_COMPONENT::typedef_v.m_bGlobalScoped; else return false; }()
            ,   .m_bBuildShareFilter    = []{ if constexpr (T_COMPONENT::typedef_v.id_v == type::id::SHARE) return T_COMPONENT::typedef_v.m_bBuildFilter; else return false; }()
            ,   .m_bExclusiveTag    = []{ if constexpr (T_COMPONENT::typedef_v.id_v == type::id::TAG)   return T_COMPONENT::typedef_v.exclusive_v;     else return false; }()
            ,   .m_pConstructFn     = std::is_trivially_constructible_v<T_COMPONENT>
                                        ? nullptr
                                        : []( std::byte* p ) noexcept
                                        {
                                            new(p) T_COMPONENT;
                                        }
            ,   .m_pDestructFn      = std::is_trivially_destructible_v<T_COMPONENT>
                                        ? nullptr
                                        : []( std::byte* p ) noexcept
                                        {
                                            std::destroy_at(reinterpret_cast<T_COMPONENT*>(p));
                                        }
            ,   .m_pMoveFn          = std::is_trivially_move_assignable_v<T_COMPONENT>
                                        ? nullptr
                                        : []( std::byte* p1, std::byte* p2 ) noexcept
                                        {
                                            *reinterpret_cast<T_COMPONENT*>(p1) = std::move(*reinterpret_cast<T_COMPONENT*>(p2));
                                        }
            ,   .m_pCopyFn          = std::is_trivially_copy_assignable_v<T_COMPONENT>
                                        ? nullptr
                                        : []( std::byte* p1, const std::byte* p2 ) noexcept
                                        {
                                            *reinterpret_cast<T_COMPONENT*>(p1) = *reinterpret_cast<const T_COMPONENT*>(p2);
                                        }
            ,   .m_pComputeKeyFn    = []() consteval noexcept ->type::share::compute_key_fn*
                                      {
                                            if constexpr (T_COMPONENT::typedef_v.id_v != type::id::SHARE) return nullptr;
                                            else if constexpr(T_COMPONENT::typedef_v.m_ComputeKeyFn)
                                            {
                                                return [](const std::byte* p) constexpr noexcept -> type::share::key
                                                {
                                                    constexpr auto Guid = std::is_same_v<xecs::component::entity, T_COMPONENT>
                                                        ? type::guid{ nullptr }
                                                        : T_COMPONENT::typedef_v.m_Guid.m_Value
                                                        ? T_COMPONENT::typedef_v.m_Guid
                                                        : type::guid{ __FUNCSIG__ };

                                                    return { T_COMPONENT::typedef_v.m_ComputeKeyFn(p).m_Value
                                                                + Guid.m_Value };
                                                };
                                            }
                                            else return [](const std::byte* p) constexpr noexcept -> type::share::key
                                            {
                                                constexpr auto Guid = type::guid{ __FUNCSIG__ };
                                                return { xcore::crc<64>{}.FromBytes( {p,sizeof(T_COMPONENT)}, Guid.m_Value ).m_Value };
                                            };
                                      }()
            ,   .m_pName            = T_COMPONENT::typedef_v.m_pName
                                        ? T_COMPONENT::typedef_v.m_pName
                                        : __FUNCSIG__
            };
        }

        //-------------------------------------------------------------------------------------

        constexpr
        type::share::key ComputeShareKey( xecs::archetype::guid Guid, const type::info& TypeInfo, const std::byte* pData = nullptr ) noexcept
        {
            if(pData)
            {
                auto Key = TypeInfo.m_pComputeKeyFn(pData);
                if (TypeInfo.m_bGlobalScoped) return Key;
                return { Guid.m_Value + Key.m_Value };

            }
            else
            {
                if (TypeInfo.m_bGlobalScoped) return TypeInfo.m_DefaultShareKey;
                return { Guid.m_Value + TypeInfo.m_DefaultShareKey.m_Value };
            }
        }

        //
        // Sort component types
        //

        //------------------------------------------------------------------------------------------------------
        constexpr __inline
        bool CompareTypeInfos( const xecs::component::type::info* pA, const xecs::component::type::info* pB ) noexcept
        {
            if ((int)pA->m_TypeID == (int)pB->m_TypeID)
            {
                return pA->m_Guid < pB->m_Guid;
            }
            return (int)pA->m_TypeID < (int)pB->m_TypeID;
        }

        //------------------------------------------------------------------------------------------------------
        template< typename T_A, typename T_B >
        struct smaller_component
        {
            constexpr static bool value = CompareTypeInfos( &xecs::component::type::info_v<T_A>, &xecs::component::type::info_v<T_B> );
        };

        template< typename T_TUPLE >
        requires(xcore::types::is_specialized_v<std::tuple, T_TUPLE>)
        using sort_tuple_t = xcore::types::tuple_sort_t<details::smaller_component, T_TUPLE>;

        template< typename T_TUPLE >
        static constexpr auto sorted_info_array_v = []<typename...T>(std::tuple<T...>*) constexpr
        {
            if constexpr (sizeof...(T) == 0 )   return std::span<const xecs::component::type::info*>{};
            else                                return std::array{ &component::type::info_v<T>... };
        }(xcore::types::null_tuple_v< sort_tuple_t<T_TUPLE> >);

        template< typename...T_TUPLES_OR_COMPONENTS >
        using combined_t = xcore::types::tuple_cat_t
        <   std::conditional_t
            <
                xcore::types::is_specialized_v<std::tuple, T_TUPLES_OR_COMPONENTS>
            ,   T_TUPLES_OR_COMPONENTS
            ,   std::tuple<T_TUPLES_OR_COMPONENTS>
            > ...
        >;

        template< typename T_TUPLE >
        using share_only_tuple_t = std::invoke_result_t
        <
            decltype
            (   []<typename...T>(std::tuple<T...>*) ->
                xcore::types::tuple_cat_t
                < std::conditional_t
                    < xecs::component::type::info_v<T>.m_TypeID == xecs::component::type::id::SHARE
                    , std::tuple<T>
                    , std::tuple<>
                    >
                ...
                > {}
            )
        , T_TUPLE*
        >;

        template< typename T_TUPLE >
        using data_only_tuple_t = std::invoke_result_t
        <   decltype
            (   []<typename...T>(std::tuple<T...>*) ->
                xcore::types::tuple_cat_t
                < std::conditional_t
                    < xecs::component::type::info_v<T>.m_TypeID == xecs::component::type::id::DATA
                    , std::tuple<T>
                    , std::tuple<>
                    >
                ...
                > {}
            )
        ,   T_TUPLE*
        >;
    }
}
