namespace xecs::query
{
    namespace details
    {
        template< typename  T_FUNCTION >
        data_pack<T_FUNCTION, true>::data_pack() noexcept
        : m_UpdatedComponentsBits { [&] <typename...T>(std::tuple<T...>*) constexpr noexcept
                                    {
                                        xecs::tools::bits Bits;
                                        (([&]<typename J>(std::tuple<J>*) constexpr noexcept
                                        {
                                            if constexpr ( false == std::is_const_v<J> )
                                            {
                                                Bits.AddFromComponents< xcore::types::decay_full_t<J> >();
                                            }
                                        }(xcore::types::make_null_tuple_v<T>)), ...);
                                        return Bits;
                                    }(xcore::types::null_tuple_v<data_pack<T_FUNCTION, true>::share_tuple_unfilter>) }
        {   
        }
    }

    //---------------------------------------------------------------------

    template< typename T_FUNCTION >
    void iterator<T_FUNCTION>::ForeachArchetype( const xecs::archetype::instance& Archetype ) noexcept
    {
        if constexpr( parent_t::has_shares_v )
        {
            // Setup the share bits only
            parent_t::m_ArchetypeShareBits.setupAnd( Archetype.getComponentBits(), xecs::component::mgr::s_ShareBits );

            // Cache a map that goes from the function share args to the family share
            [&]<typename...T_COMPONENTS>(std::tuple<T_COMPONENTS...>*) constexpr noexcept
            {
                (([&]<typename J>(std::tuple<J>*) constexpr noexcept
                {
                    if constexpr ( std::is_pointer_v<J>)
                    {
                        parent_t::m_RemapIndices[xcore::types::tuple_t2i_v<J, parent_t::share_tuple_unfilter>] 
                            = (parent_t::m_ArchetypeShareBits.getBit(xecs::component::type::info_v<J>.m_BitID) )
                                ? parent_t::m_ArchetypeShareBits.getIndexOfComponent(xecs::component::type::info_v<J>.m_BitID)
                                : -1;
                    }
                    else
                    {
                        assert(parent_t::m_ArchetypeShareBits.getBit(xecs::component::type::info_v<J>.m_BitID));
                        parent_t::m_RemapIndices[xcore::types::tuple_t2i_v<J, parent_t::share_tuple_unfilter>]
                            = parent_t::m_ArchetypeShareBits.getIndexOfComponent(xecs::component::type::info_v<J>.m_BitID);
                    }
                }(xcore::types::make_null_tuple_v<T_COMPONENTS>)), ... );
            }( xcore::types::null_tuple_v<parent_t::share_tuple_unfilter> );
        }
    }

    //---------------------------------------------------------------------

    template< typename T_FUNCTION >
    void iterator<T_FUNCTION>::ForeachFamilyPool(const xecs::component::mgr& ComponentMgr, const xecs::pool::family& Family) noexcept
    {
        if constexpr (parent_t::has_shares_v)
        {
            //
            // Update pointers and keys in case something has change
            //
            [&] <typename...T>(std::tuple<T...>*) constexpr noexcept
            {
                (([&]<typename J>(std::tuple<J>*) constexpr noexcept
                {
                    const auto      Index               = xcore::types::tuple_t2i_v<T, parent_t::share_tuple_unfilter>;
                    const auto      RemapedIndex        = parent_t::m_RemapIndices[Index];
                    const auto&     FamilyShareDetails  = Family.m_ShareDetails[RemapedIndex];

                    if( parent_t::m_CacheShareKeys[Index] != FamilyShareDetails.m_Key )
                    {
                        using       j_uni           = parent_t::template universal_t<J>;
                        using       j_ptr           = xcore::types::decay_full_t<J>*;

                        auto&       EntityDetails   = ComponentMgr.getEntityDetails(FamilyShareDetails.m_Entity);
                        const auto  ComponentIndex  = parent_t::m_RemapIndices[Index];//EntityDetails.m_pPool->findIndexComponentFromInfo(*Family.m_ShareInfos[RemapedIndex]);

                        if constexpr( std::is_pointer_v<J> )
                        {
                            if(-1 != ComponentIndex)
                            {
                                if constexpr ( std::is_pointer_v<j_uni> )
                                {
                                    std::get<j_uni>(parent_t::m_UniversalTuple) = &reinterpret_cast<j_uni>(EntityDetails.m_pPool->m_pComponent[ComponentIndex])[EntityDetails.m_PoolIndex.m_Value];
                                }
                                else
                                {
                                    std::get<j_ptr>(parent_t::m_CacheSharePointers) = &reinterpret_cast<j_ptr>(EntityDetails.m_pPool->m_pComponent[ComponentIndex])[EntityDetails.m_PoolIndex.m_Value];
                                    std::get<j_uni>(parent_t::m_UniversalTuple)     = *std::get<j_ptr>(parent_t::m_CacheSharePointers);
                                }

                                parent_t::m_CacheShareKeys[Index] = Family.m_ShareDetails[RemapedIndex];
                            }
                        }
                        else
                        {
                            assert(-1 != ComponentIndex);
                            if constexpr (std::is_pointer_v<j_uni>)
                            {
                                std::get<j_uni>(parent_t::m_UniversalTuple) = &reinterpret_cast<j_uni>(EntityDetails.m_pPool->m_pComponent[ComponentIndex])[EntityDetails.m_PoolIndex.m_Value];
                            }
                            else
                            {
                                std::get<j_ptr>(parent_t::m_CacheSharePointers) = &reinterpret_cast<j_ptr>(EntityDetails.m_pPool->m_pComponent[ComponentIndex])[EntityDetails.m_PoolIndex.m_Value];
                                std::get<j_uni>(parent_t::m_UniversalTuple)     = *std::get<j_ptr>(parent_t::m_CacheSharePointers);
                            }

                            parent_t::m_CacheShareKeys[Index] = Family.m_ShareDetails[RemapedIndex].m_Key;
                        }
                    }
                }(xcore::types::make_null_tuple_v<T>), ...));
            }(xcore::types::null_tuple_v<parent_t::share_tuple_unfilter>);

            //
            // Compute the key Sum Guid (We will use it to detect if there are changes made in the shares)
            //
            parent_t::m_KeyCheckSum = [&]<typename... T>(std::tuple<T...>*) constexpr noexcept
            {
                return (([&]<typename J>(std::tuple<J>*) constexpr noexcept
                {
                    if constexpr( std::is_const_v<J> )
                    {
                        return 0;
                    }
                    else 
                    {
                        const int Index = xcore::types::tuple_t2i_v< J, parent_t::share_tuple_unfilter >;
                        if constexpr (std::is_pointer_v<J>) if( -1 == parent_t::m_RemapIndices[Index] ) return 0;
                        return parent_t::m_CacheShareKeys[Index].m_Value;
                    }
                }(xcore::types::make_null_tuple_v<T>)) + ...);

            }( xcore::types::null_tuple_v<parent_t::share_tuple_unfilter> );
        }
    }

    //---------------------------------------------------------------------

    template< typename T_FUNCTION >
    void iterator<T_FUNCTION>::ForeachPool( const xecs::tools::bits& ArchetypeBits, xecs::pool::instance& Pool ) noexcept
    {
        //
        // Set the data tuple for each pool
        // Handles references and pointers const and non-const
        //
        if constexpr (parent_t::has_shares_v)
        {
            parent_t::m_EntityIndex = 0;
        }

        //
        // Handle regular components, it runs in both modes shares+data/data only
        //
        {
            [&]<typename...T>(std::tuple<T...>*) constexpr noexcept
            {
                (([&]<typename J>(std::tuple<J>*)
                {
                    using t = parent_t::template universal_t<J>;
                    if constexpr (std::is_pointer_v<J>)
                    {
                        if( ArchetypeBits.getBit(xecs::component::type::info_v<t>.m_BitID) )
                        {
                            const auto I = ArchetypeBits.getIndexOfComponent(xecs::component::type::info_v<t>.m_BitID);
                            std::get<t>(parent_t::m_DataTuple) = reinterpret_cast<t>(Pool.m_pComponent[I]);
                        }
                        else
                        {
                            std::get<t>(parent_t::m_DataTuple) = nullptr;
                        }
                    }
                    else
                    {
                        assert(ArchetypeBits.getBit(xecs::component::type::info_v<t>.m_BitID));
                        const auto I = ArchetypeBits.getIndexOfComponent(xecs::component::type::info_v<t>.m_BitID);
                        std::get<t>(parent_t::m_DataTuple) = reinterpret_cast<t>(Pool.m_pComponent[I]);
                    }
                }( reinterpret_cast<std::tuple<T>*>(nullptr) )), ... );
            }( xcore::types::null_tuple_v<parent_t::data_tuple_unfilter> );
        }
    }

    //---------------------------------------------------------------------

    template< typename T_FUNCTION >
    xcore::function::traits<T_FUNCTION>::return_type
    iterator<T_FUNCTION>::CallUserFunction( xecs::archetype::instance& Archetype, xecs::pool::family& Family, xecs::pool::instance& Pool, T_FUNCTION&& Function) noexcept
    {
        if constexpr ( parent_t::has_shares_v )
        {
            auto& GameMgr = Archetype.m_Mgr.m_GameMgr;

            //
            // Call the user function
            //
            [&] <typename...T>(std::tuple<T...>*) constexpr noexcept
            {
                Function
                (   [&]<typename J>(std::tuple<J>*) constexpr noexcept -> J
                    {
                        using univ = parent_t::template universal_t<J>;

                        if constexpr (xecs::component::type::info_v<J>.m_TypeID == xecs::component::type::id::SHARE)
                        {
                            if constexpr (std::is_pointer_v<J>)
                            {
                                if constexpr (std::is_pointer_v<univ>) return std::get<univ>(parent_t::m_UniversalTuple);
                                else                                   return &std::get<univ>(parent_t::m_UniversalTuple);
                            }
                            else
                            {
                                if constexpr (std::is_pointer_v<univ>) return *std::get<univ>(parent_t::m_UniversalTuple);
                                else                                   return std::get<univ>(parent_t::m_UniversalTuple);
                            }
                        }
                        else
                        {
                            auto& MyP = std::get<parent_t::universal_t<J>>(parent_t::m_DataTuple);

                            if constexpr (std::is_pointer_v<J>) if (MyP == nullptr) return reinterpret_cast<J>(nullptr);

                            auto p = MyP;                   // Back up the pointer
                            MyP++;                         // Get ready for the next entity

                            if constexpr (std::is_pointer_v<J>) return reinterpret_cast<J>(p);
                            else                                return reinterpret_cast<J>(*p);
                        }
                    }( xcore::types::make_null_tuple_v<T> )
                    ...
                );

            }(xcore::types::null_tuple_v<typename func_t::args_tuple>);

            //
            // Did the user change any of the share components?
            // If so... we need to move this entity into another family
            //
            [&]<typename...T>(std::tuple<T...>*) constexpr noexcept
            {
                //
                // Compute the new Sum Guid which will allow us to detect if something has change
                //
                std::array<xecs::component::type::share::key, parent_t::nonconst_share_count_v >   UpdatedKeyArray;
                std::uint64_t                                                                      NewKeySumGuid    {};
                int                                                                                nNewEnties       {0};

                (([&]<typename J>(std::tuple<J>*) constexpr noexcept
                {
                    if constexpr ( false == std::is_const_v<J> )
                    {
                        const auto Index = xcore::types::tuple_t2i_v<J, parent_t::share_tuple_unfilter>;
                        if constexpr (std::is_pointer_v<J>) if(-1 == parent_t::m_RemapIndices[Index]) return;

                        UpdatedKeyArray[nNewEnties] = xecs::component::type::details::ComputeShareKey
                        ( Archetype.getGuid()
                        , xecs::component::type::info_v<J>
                        , reinterpret_cast<const std::byte*>(&std::get<parent_t::universal_t<J>>(parent_t::m_UniversalTuple))
                        );
                        NewKeySumGuid += UpdatedKeyArray[nNewEnties++].m_Value;
                    }
                }( xcore::types::make_null_tuple_v<T> )), ... );

                //
                // Check if we need to change family
                //
                if( NewKeySumGuid != parent_t::m_KeyCheckSum)
                {
                    std::array<std::byte*, parent_t::nonconst_share_count_v >   PointersToShares;

                    // Collect the rest of the data
                    int nCompactify = 0;
                    (([&]<typename J>(std::tuple<J>*) constexpr noexcept
                    {
                        if constexpr (false == std::is_const_v<J>)
                        {
                            const auto Index = xcore::types::tuple_t2i_v<J, parent_t::share_tuple_unfilter>;
                            if constexpr (std::is_pointer_v<J>) if (-1 == parent_t::m_RemapIndices[Index]) return;
                            PointersToShares[nCompactify] = reinterpret_cast<std::byte*>(&std::get<parent_t::universal_t<J>>(parent_t::m_UniversalTuple));
                            nCompactify++;
                        }
                    }(xcore::types::make_null_tuple_v<T>)), ...);
                    assert( nCompactify == nNewEnties );
                    assert( nCompactify == parent_t::m_UpdatedComponentsBits.CountComponents() );

                    //
                    // Get the new family and move the entity there
                    //
                    Archetype.getOrCreatePoolFamilyFromSameArchetype
                    ( Family
                    , parent_t::m_UpdatedComponentsBits
                    , PointersToShares
                    , UpdatedKeyArray
                    )
                    .MoveIn
                    ( GameMgr
                    , Family
                    , Pool
                    , { parent_t::m_EntityIndex }
                    );

                    //
                    // Copy the memory for only share components that can change
                    //
                    (([&]<typename J>(std::tuple<J>*) constexpr noexcept
                    {
                        if constexpr (!std::is_const_v<J>)
                        {
                            const auto Index = xcore::types::tuple_t2i_v<J, parent_t::share_tuple_unfilter>;
                            if constexpr (std::is_pointer_v<J>) if (-1 == parent_t::m_RemapIndices[Index]) return;
                            std::get<parent_t::universal_t<J>>(parent_t::m_UniversalTuple) = *std::get<xcore::types::decay_full_t<J>*>(parent_t::m_CacheSharePointers);
                        }
                    }(xcore::types::make_null_tuple_v<T>)), ...);
                }

            }(xcore::types::null_tuple_v<parent_t::share_tuple_unfilter>);

            //
            // Increase the entity index count
            //
            parent_t::m_EntityIndex++;
        }
        else
        {
            //
            // Call the user function
            //
            if constexpr (std::is_same_v<bool, ret_t>)
            {
                return [&] <typename... T>(std::tuple<T...>*) constexpr noexcept
                {
                    return Function
                    ( [&]<typename J>(std::tuple<J>*) constexpr noexcept -> J
                        {
                            auto& MyP = std::get<parent_t::data_universal_t<J>>(parent_t::m_DataTuple);

                            if constexpr (std::is_pointer_v<J>) if (MyP == nullptr) return reinterpret_cast<J>(nullptr);

                            auto p = MyP;                   // Back up the pointer
                            MyP ++;                         // Get ready for the next entity

                            if constexpr (std::is_pointer_v<J>) return reinterpret_cast<J>(p);
                            else                                return reinterpret_cast<J>(*p);
                        }(xcore::types::make_null_tuple_v<T>)
                        ...
                    );
                }(xcore::types::null_tuple_v<typename func_t::args_tuple>);
            }
            else
            {
                [&] <typename... T>(std::tuple<T...>*) constexpr noexcept
                {
                    Function
                    ( [&]<typename J>(std::tuple<J>*) constexpr noexcept -> J
                        {
                            auto& MyP = std::get<parent_t::universal_t<J>>(parent_t::m_DataTuple);

                            if constexpr (std::is_pointer_v<J>) if (MyP == nullptr) return reinterpret_cast<J>(nullptr);

                            auto p = MyP;                   // Back up the pointer
                            MyP ++;                         // Get ready for the next entity

                            if constexpr (std::is_pointer_v<J>) return reinterpret_cast<J>(p);
                            else                                return reinterpret_cast<J>(*p);
                        }(xcore::types::make_null_tuple_v<T>)
                        ...
                    );
                }(xcore::types::null_tuple_v<typename func_t::args_tuple>);
            }
        }
    }
}