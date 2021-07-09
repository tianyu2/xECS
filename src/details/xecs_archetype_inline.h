namespace xecs::archetype
{
    namespace details
    {
        //-------------------------------------------------------------------------------------------------

        template< typename T_FUNCTION >
        constexpr auto function_arguments_r_refs_v = []<typename...T>(std::tuple<T...>*) constexpr
        {
            return (std::is_reference_v<T> && ...);
        }( xcore::types::null_tuple_v<xcore::function::traits<T_FUNCTION>::args_tuple> );

        //-------------------------------------------------------------------------------------------------

        template< typename... T_FUNCTION_ARGS >
        requires( ((std::is_reference_v<T_FUNCTION_ARGS>) && ... ) )
        constexpr __inline
        auto GetDataComponentPointerArray
        ( const xecs::pool::instance&             Pool
        , const pool::index                       StartingPoolIndex
        , std::tuple<T_FUNCTION_ARGS... >* 
        ) noexcept
        {
            assert( ((Pool.findIndexComponentFromInfo(xecs::component::type::info_v<T_FUNCTION_ARGS>) >= 0 ) && ... ) );

            using function_args = std::tuple< T_FUNCTION_ARGS... >;
            using sorted_tuple  = xecs::component::type::details::sort_tuple_t< function_args >;

            std::array< std::byte*, sizeof...(T_FUNCTION_ARGS) > CachePointers;
            [&]<typename... T_SORTED_COMPONENT >( std::tuple<T_SORTED_COMPONENT...>* ) constexpr noexcept
            {
                int Sequence = 0;
                ((CachePointers[xcore::types::tuple_t2i_v< T_SORTED_COMPONENT, function_args >] = 
                  &Pool.m_pComponent[Pool.findIndexComponentFromInfoInSequence(xecs::component::type::info_v<T_SORTED_COMPONENT>, Sequence)]
                        [sizeof(std::remove_reference_t<T_SORTED_COMPONENT>) * StartingPoolIndex.m_Value ])
                , ... );

            }( xcore::types::null_tuple_v<sorted_tuple>);

            return CachePointers;
        }

        //-------------------------------------------------------------------------------------------------

        template< typename... T_FUNCTION_ARGS >
        requires( ((std::is_pointer_v<T_FUNCTION_ARGS>) || ...) )
        constexpr __inline
        auto GetDataComponentPointerArray
        ( const xecs::pool::instance&             Pool
        , const pool::index                       StartingPoolIndex
        , std::tuple<T_FUNCTION_ARGS... >* 
        ) noexcept
        {
            static_assert(((std::is_reference_v<T_FUNCTION_ARGS> || std::is_pointer_v<T_FUNCTION_ARGS>) && ...));

            using function_args = std::tuple< T_FUNCTION_ARGS... >;
            using sorted_tuple  = xecs::component::type::details::sort_tuple_t< function_args >;

            std::array< std::byte*, sizeof...(T_FUNCTION_ARGS) > CachePointers;
            [&]<typename... T_SORTED_COMPONENT >( std::tuple<T_SORTED_COMPONENT...>* ) constexpr noexcept
            {
                int Sequence = 0;
                ((CachePointers[xcore::types::tuple_t2i_v< T_SORTED_COMPONENT, function_args >] = [&]<typename T>(std::tuple<T>*) constexpr noexcept 
                {
                    const auto I = Pool.findIndexComponentFromInfoInSequence(xecs::component::type::info_v<T>, Sequence);
                    if constexpr (std::is_pointer_v<T>)
                        return (I < 0) ? nullptr : &Pool.m_pComponent[I][sizeof(std::decay_t<T>) * StartingPoolIndex.m_Value];
                    else
                        return &Pool.m_pComponent[I][sizeof(std::decay_t<T>) * StartingPoolIndex.m_Value];
                }(xcore::types::make_null_tuple_v<T_SORTED_COMPONENT>))
                , ... );

            }( xcore::types::null_tuple_v<sorted_tuple>);

            return CachePointers;
        }

        //-------------------------------------------------------------------------------------------------
        template< typename T_FUNCTION, typename T_ARRAY >
        requires( function_arguments_r_refs_v<T_FUNCTION>
                  && std::is_same_v< void, typename xcore::function::traits<T_FUNCTION>::return_type >
                )
        constexpr __inline
        void CallFunction( T_FUNCTION&& Function, T_ARRAY& CachePointers) noexcept
        {
            using func_traits = xcore::function::traits<T_FUNCTION>;
            [&] <typename...T>(std::tuple<T...>*) constexpr noexcept
            {
                static_assert(((std::is_reference_v<T>) && ...));
                Function(reinterpret_cast<T>(*CachePointers[xcore::types::tuple_t2i_v<T, func_traits::args_tuple>])
                    ...);
                ((CachePointers[xcore::types::tuple_t2i_v<T, func_traits::args_tuple>] += sizeof(std::remove_reference_t<T>)), ...);
            }(xcore::types::null_tuple_v<func_traits::args_tuple>);
        }

        //-------------------------------------------------------------------------------------------------
        template< typename T_FUNCTION, typename T_ARRAY >
        requires( function_arguments_r_refs_v<T_FUNCTION>
                  && std::is_same_v< bool, typename xcore::function::traits<T_FUNCTION>::return_type >
                )
        constexpr __inline
        bool CallFunction( T_FUNCTION&& Function, T_ARRAY& CachePointers) noexcept
        {
            using func_traits = xcore::function::traits<T_FUNCTION>;
            return [&] <typename...T>(std::tuple<T...>*) constexpr noexcept
            {
                static_assert(((std::is_reference_v<T>) && ...));
                if( Function(reinterpret_cast<T>(*CachePointers[xcore::types::tuple_t2i_v<T, func_traits::args_tuple>])
                    ...) ) return true;
                ((CachePointers[xcore::types::tuple_t2i_v<T, func_traits::args_tuple>] += sizeof(std::remove_reference_t<T>)), ...);
                return false;
            }(xcore::types::null_tuple_v<func_traits::args_tuple>);
        }

        //-------------------------------------------------------------------------------------------------
        template< typename T_FUNCTION, typename T_ARRAY >
        requires( xcore::function::is_callable_v<T_FUNCTION>
                  && false == function_arguments_r_refs_v<T_FUNCTION>
                  && std::is_same_v< void, typename xcore::function::traits<T_FUNCTION>::return_type >
                )
        constexpr __inline
        void CallFunction( T_FUNCTION&& Function, T_ARRAY& CachePointers) noexcept
        {
            using func_traits = xcore::function::traits<T_FUNCTION>;
            [&] <typename... T_COMPONENTS>(std::tuple<T_COMPONENTS...>*) constexpr noexcept
            {
                Function([&]<typename T>(std::tuple<T>*) constexpr noexcept -> T
                {
                    auto& MyP = CachePointers[xcore::types::tuple_t2i_v<T, typename func_traits::args_tuple>];

                    if constexpr (std::is_pointer_v<T>) if (MyP == nullptr) return reinterpret_cast<T>(nullptr);

                    auto p = MyP;                   // Back up the pointer
                    MyP += sizeof(std::decay_t<T>); // Get ready for the next entity

                    if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(p);
                    else                                return reinterpret_cast<T>(*p);
                }(xcore::types::make_null_tuple_v<T_COMPONENTS>)
                ...);
            }(xcore::types::null_tuple_v<func_traits::args_tuple>);
        }

        //-------------------------------------------------------------------------------------------------
        template< typename T_FUNCTION, typename T_ARRAY >
        requires
        ( xcore::function::is_callable_v<T_FUNCTION>
          && false == function_arguments_r_refs_v<T_FUNCTION>
          && std::is_same_v< bool, typename xcore::function::traits<T_FUNCTION>::return_type >
        )
        constexpr __inline
    bool CallFunction( T_FUNCTION&& Function, T_ARRAY& CachePointers) noexcept
        {
            using func_traits = xcore::function::traits<T_FUNCTION>;
            return [&] <typename... T_COMPONENTS>(std::tuple<T_COMPONENTS...>*) constexpr noexcept
            {
                return Function([&]<typename T>(std::tuple<T>*) constexpr noexcept -> T
                {
                    auto& MyP = CachePointers[xcore::types::tuple_t2i_v<T, typename func_traits::args_tuple>];

                    if constexpr (std::is_pointer_v<T>) if (MyP == nullptr) return reinterpret_cast<T>(nullptr);

                    auto p = MyP;                   // Back up the pointer
                    MyP += sizeof(std::decay_t<T>); // Get ready for the next entity

                    if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(p);
                    else                                return reinterpret_cast<T>(*p);
                }(xcore::types::make_null_tuple_v<T_COMPONENTS>)
                ...);
            }(xcore::types::null_tuple_v<func_traits::args_tuple>);
        }

        //-------------------------------------------------------------------------------------------------
        inline
        int
    ComputeIndexRemaps
        ( std::span<int>                                            To2From
        , std::span<const xecs::component::type::info*>             FinalInfos
        , const std::span<const xecs::component::type::info* const> To
        , const std::span<const xecs::component::type::info* const> From
        ) noexcept
        {
            assert( To2From.size() == To.size() );
            int FinalCount = 0;
            for( std::size_t iFrom = 0, iTo = 0; ; )
            {
                if( From[iFrom]->m_Guid.m_Value == To[iTo]->m_Guid.m_Value )
                {
                    FinalInfos[FinalCount] = From[iFrom];
                    To2From[FinalCount++]  = static_cast<int>(iFrom);

                    iFrom++;
                    if (iFrom == From.size()) break;

                    iTo++;
                    if( iTo == To.size() ) break;
                }
                else if( xecs::component::type::details::CompareTypeInfos( To[iTo], From[iFrom] ) )
                {
                    iFrom++;
                    if (iFrom == From.size()) break;
                }
                else
                {
                    iTo++;
                    if (iTo == To2From.size()) break;
                }
            }
            return FinalCount;
        }

        //-------------------------------------------------------------------------------------------------
        inline
        void CollectSharePointersFromFamily
        ( std::span<std::byte*>                                     ToPointers
        , const std::span<int const >                               To2From
        , const std::span<const xecs::component::type::info* const> ToInfos
        , const xecs::pool::family&                                 FromFamily
        , const xecs::component::mgr&                               ComponentMgr
        ) noexcept
        {
            for (int i = 0; i < ToInfos.size(); i++ )
            {
                const auto  iFrom    = To2From[i];
                const auto& FromInfo = *FromFamily.m_ShareInfos[iFrom];

                // Get the data from...
                auto& ShareEntityDetails = ComponentMgr.getEntityDetails( FromFamily.m_ShareDetails[iFrom].m_Entity );
                auto  FromIndex          = ShareEntityDetails.m_pPool->findIndexComponentFromInfo(FromInfo);
                ToPointers[i]            = &ShareEntityDetails.m_pPool->m_pComponent[FromIndex][ShareEntityDetails.m_PoolIndex.m_Value * static_cast<int>(FromInfo.m_Size)];
            }
        }
    }

    //--------------------------------------------------------------------------------------------

    instance::instance
    ( xecs::archetype::mgr& Mgr
    ) noexcept
    : m_Mgr{ Mgr }
    {}

    //--------------------------------------------------------------------------------------------

    void instance::Initialize
    ( archetype::guid                                       Guid
    , std::span<const xecs::component::type::info* const>   Infos
    , const tools::bits&                                    Bits
    ) noexcept
    {
        // Deep copy the infos just in case the user gave us data driven infos
        // Also we will extract all the tag components (since no data can be store there)
        bool AreTheySorted = true;
        m_nShareComponents = 0;
        int nInfos         = 0;
        for( int i=0; i<Infos.size(); ++i )
        {
            // Make sure all the components are accounted for
            assert(Bits.getBit(Infos[i]->m_BitID));

            // Extract flags out (if any)
            if( Infos[i]->m_TypeID == xecs::component::type::id::TAG )
            {
                continue;
            }

            m_InfoData[nInfos] = Infos[i];
            if( nInfos && AreTheySorted )
            {
                AreTheySorted = xecs::component::type::details::CompareTypeInfos(m_InfoData[nInfos - 1], m_InfoData[nInfos]);
            }

            if( m_InfoData[nInfos]->m_TypeID == xecs::component::type::id::SHARE ) ++m_nShareComponents;

            // Count up our new set
            nInfos++;
        }

        // Short Infos, entry 0 should be the entity
        if( false == AreTheySorted )
        {
            std::sort
            ( m_InfoData.begin()
            , m_InfoData.begin() + (nInfos - 1)
            , xecs::component::type::details::CompareTypeInfos
            );
        }

        //
        // Lets run a sanity check
        //
#ifdef _DEBUG
        {
            // First component should the the entity
            assert(m_InfoData[0] == &xecs::component::type::info_v<xecs::component::entity>);

            // Entity bit should be turn on
            assert(Bits.getBit(xecs::component::type::info_v<xecs::component::entity>.m_BitID));

            // Check all other components
            for (int i = 1; i < nInfos; ++i)
            {
                // There should be no duplication of components
                assert(m_InfoData[i - 1] != m_InfoData[i]);

                // Check that the bits match
                assert( Bits.getBit(m_InfoData[i]->m_BitID));
            }
        }
#endif

        //
        // We initialize the remaining vars
        //

        // Is the user telling us not ignore the shares?
        if( Bits.getBit(xecs::component::type::info_v<xecs::component::share_as_data_exclusive_tag>.m_BitID) ) m_nShareComponents = 0;

        // Setup the last few bits
        m_ComponentBits     = Bits;
        m_ExclusiveTagsBits.setupAnd( Bits, xecs::component::mgr::m_ExclusiveTagsBits );
        m_nDataComponents = xcore::types::static_cast_safe<std::uint8_t>(nInfos) - m_nShareComponents;
        m_Guid            = Guid;

        //
        // We can initialize our default pool if not shares...
        //
        /*
        if( m_nShareComponents == 0 )
        {
            m_DefaultPoolFamily2.Initialize( pool::family::guid{42ull}, {}, {}, {}, { m_InfoData.data(), static_cast<std::size_t>(nInfos) } );
            m_Mgr.AddToStructutalPendingList(&m_DefaultPoolFamily2);
        }
        */

        if( m_nShareComponents )
        {
            //
            // Make sure that all the pools for our share components are cached
            //
            for (int i = 0; i < m_nShareComponents; ++i)
            {
                m_ShareArchetypesArray[i] = m_Mgr.getOrCreateArchetype
                ( std::array
                    { &xecs::component::type::info_v<xecs::component::entity>
                    , &xecs::component::type::info_v<xecs::component::ref_count>
                    , &xecs::component::type::info_v<xecs::component::share_as_data_exclusive_tag>
                    , m_InfoData[ m_nDataComponents + i ]
                    }
                );
            }
        }
    }

    //--------------------------------------------------------------------------------------------

    xecs::pool::family& instance::getOrCreatePoolFamily
    ( std::span< const xecs::component::type::info* const>  TypeInfos
    , std::span< std::byte* >                               MoveData
    ) noexcept
    {
        assert(TypeInfos.size() == MoveData.size());
        if (m_nShareComponents == 0)
        {
            if(m_DefaultPoolFamily2.m_Guid.isValid()) return m_DefaultPoolFamily2;

            assert(TypeInfos.size() == 0);

            // For these types of archetypes we only have one family
            m_DefaultPoolFamily2.Initialize(pool::family::guid{ 42ull }, *this, {}, {}, {}, { m_InfoData.data(), static_cast<std::size_t>(m_nDataComponents+m_nShareComponents) });
            m_Mgr.AddToStructuralPendingList( m_DefaultPoolFamily2 );

            return m_DefaultPoolFamily2;
        }

        //
        // Lets do a quick sanity check
        //
#ifdef _DEBUG
        for( auto& e : TypeInfos )
        {
            // The user only can give the share components to find the family
            assert(e->m_TypeID == xecs::component::type::id::SHARE );

            // Make sure that the components that the user gave us are with in this archetype infos
            bool bFound = false;
            for( auto s : std::span{ &m_InfoData[m_nDataComponents], (std::size_t)m_nShareComponents } )
            {
                if( s == e ) 
                {
                    bFound = true;
                    break;
                }
            }
            assert(bFound);

            // Make sure that there are not duplicates
            for( auto i = 1 + static_cast<std::size_t>( &e - TypeInfos.data()); i< TypeInfos.size(); ++i )
            {
                assert( e->m_Guid != TypeInfos[i]->m_Guid );
            }
        }

        // Make sure all the move data is there
        for( auto e : MoveData )
            assert(e);
#endif

        //
        // Fast Path... They give me all the data so just execute 
        //
        xecs::pool::family::guid g{};
        if( TypeInfos.size() == m_nShareComponents )
        {
            auto FamilyGuid = xecs::pool::family::ComputeGuid
            (   m_Guid
                , TypeInfos
                , MoveData
            );
            g = FamilyGuid;
            if (auto It = m_Mgr.m_PoolFamily.find(FamilyGuid); It != m_Mgr.m_PoolFamily.end())
                return *It->second;
        }

        //
        // Lets compute all the requires Keys
        //
        std::array<xecs::component::type::share::key,   xecs::settings::max_share_components_per_entity_v> AllKeys;
        std::array<std::byte*,                          xecs::settings::max_share_components_per_entity_v> DataInOrder;
        std::array<xecs::component::entity,             xecs::settings::max_share_components_per_entity_v> ShareComponentEntityRefs;

        xecs::pool::family::guid FamilyGuid{ m_Guid.m_Value };
        for( int i=0; i< m_nShareComponents; i++ )
        {
            auto pInfo = m_InfoData[ m_nDataComponents + i ];
            int  Index = -1;
            for( int j=0; j< TypeInfos.size(); j++ )
            {
                if(TypeInfos[j] == pInfo )
                {
                    Index = j;
                    break;
                }
            }

            // Set the data in the right place
            if( Index == -1 ) DataInOrder[i] = nullptr;
            else              DataInOrder[i] = MoveData[Index];

            // Compute the key for this share component
            AllKeys[i]     = xecs::component::type::details::ComputeShareKey(m_Guid, *pInfo, DataInOrder[i]);
            FamilyGuid.m_Value += AllKeys[i].m_Value;
        }
        assert(g.isValid() == false || g == FamilyGuid );

        if (auto It = m_Mgr.m_PoolFamily.find(FamilyGuid); It != m_Mgr.m_PoolFamily.end())
            return *It->second;

        //
        // Make sure all the share components exists and the references are set to the correct amount
        //
        for (int i = 0; i < m_nShareComponents; i++)
        {
            auto pInfo = m_InfoData[m_nDataComponents + i];

            //
            // Does this share component exists?
            //
            if( auto It = m_Mgr.m_ShareComponentEntityMap.find(AllKeys[i]); It == m_Mgr.m_ShareComponentEntityMap.end() )
            {
                xecs::component::entity Entity;
                if(DataInOrder[i])
                {
                    Entity = m_ShareArchetypesArray[i]->CreateEntity
                    ( { &pInfo, 1u }
                    , { &DataInOrder[i], 1u }
                    );
                }
                else 
                {
                    Entity = m_ShareArchetypesArray[i]->CreateEntity({},{});
                }

                m_Mgr.m_ShareComponentEntityMap.emplace( AllKeys[i], Entity );
                ShareComponentEntityRefs[i] = Entity;
            }
            else
            {
                ShareComponentEntityRefs[i] = It->second;
                m_Mgr.m_GameMgr.getEntity(ShareComponentEntityRefs[i], [](xecs::component::ref_count& RefCount )
                {
                    RefCount.m_Value++;
                });
            }
        }

        //
        // Create new Pool Family
        //
        return CreateNewPoolFamily
        ( FamilyGuid
        , std::span{ ShareComponentEntityRefs.data(),       static_cast<std::size_t>(m_nShareComponents) }
        , std::span{ AllKeys.data(),                        static_cast<std::size_t>(m_nShareComponents) }
        , std::span{ m_InfoData.data() + m_nDataComponents, static_cast<std::size_t>(m_nShareComponents) }
        , std::span{ m_InfoData.data(),                     static_cast<std::size_t>(m_nDataComponents)  }
        );
    }

    //--------------------------------------------------------------------------------------------
    template< typename T >
    T& instance::getShareComponent( xecs::pool::family& Family ) noexcept
    {
        assert( m_nShareComponents == Family.m_ShareInfos.size() );
        for( int i=0, end = m_nShareComponents; i!=end; ++i )
        {
            if( Family.m_ShareInfos[i] == &xecs::component::type::info_v<T> )
            {
                const auto& ShareDetails = Family.m_ShareDetails[i];
                const auto& GlobalEntity = m_Mgr.m_GameMgr.m_ComponentMgr.getEntityDetails(ShareDetails.m_Entity);
                return GlobalEntity.m_pPool->getComponent<T>(GlobalEntity.m_PoolIndex);
            }
        }
        assert(false);
        return getShareComponent<T>(Family); //m_DefaultPoolFamily.m_DefaultPool.getComponent<T>({0});
    }

    //--------------------------------------------------------------------------------------------

    xecs::pool::family& 
instance::getOrCreatePoolFamilyFromSameArchetype
    ( xecs::pool::family&                                   FromFamily
    , std::span< const int >                                IndexRemaps
    , std::span< const xecs::component::type::info* const>  TypeInfos
    , std::span< std::byte* >                               MoveData
    , std::span< const xecs::component::entity >            EntitySpan
    , std::span< const xecs::component::type::share::key >  Keys
    ) noexcept
    {
        assert(TypeInfos.size() == MoveData.size());
        assert(TypeInfos.size() == EntitySpan.size());
        assert(TypeInfos.size() == Keys.size());

        //
        // Lets compute the New Family GUID
        //
        xecs::pool::family::guid NewFamilyGuid{ m_Guid.m_Value };
        for( int i=0, j=0, end = static_cast<int>(m_nShareComponents); i != end; i++ )
        {
            NewFamilyGuid.m_Value += (TypeInfos[j] == FromFamily.m_ShareInfos[i])
            ? Keys[j++].m_Value
            : FromFamily.m_ShareDetails[i].m_Key.m_Value;
        }

        //
        // Try to find the new family
        //
        if (auto It = m_Mgr.m_PoolFamily.find(NewFamilyGuid); It != m_Mgr.m_PoolFamily.end())
            return *It->second;

        //
        // Make sure all the shares Entities are created
        //
        std::array< xecs::component::entity,            xecs::settings::max_share_components_per_entity_v > FinalShareEntities;
        std::array< xecs::component::type::share::key,  xecs::settings::max_share_components_per_entity_v > FinalShareKeys;

        // Copy all the share entity keys... some of them may get replace later
        for (int i = 0, end = static_cast<int>(Keys.size()); i != end; i++)
        {
            FinalShareEntities[i] = FromFamily.m_ShareDetails[i].m_Entity;
            FinalShareKeys[i]     = FromFamily.m_ShareDetails[i].m_Key;
        }
        
        // Create shares and make sure we set all the right entities into the final array
        for( int i=0, end = static_cast<int>(Keys.size()); i != end; i++ )
        {
            if( Keys[i] != FromFamily.m_ShareDetails[ IndexRemaps[i] ].m_Key )
            {
                // Update the key
                FinalShareKeys[ IndexRemaps[i] ] = Keys[i];

                //
                // Does this share component exists?
                //
                if (auto It = m_Mgr.m_ShareComponentEntityMap.find(Keys[i]); It == m_Mgr.m_ShareComponentEntityMap.end())
                {
                    xecs::component::entity Entity = m_ShareArchetypesArray[ IndexRemaps[i] ]->CreateEntity
                    (  { &TypeInfos[i], 1u }
                     , { &MoveData[i],  1u }
                    );

                    m_Mgr.m_ShareComponentEntityMap.emplace(Keys[i], Entity);
                    FinalShareEntities[ IndexRemaps[i] ] = Entity;
                }
                else
                {
                    m_Mgr.m_GameMgr.findEntity( FinalShareEntities[ IndexRemaps[i] ], [](xecs::component::ref_count& RefCount)
                    {
                        RefCount.m_Value++;
                    });
                }
            }
        }

        //
        // Create new Pool Family
        //
        return CreateNewPoolFamily
        ( NewFamilyGuid
        , std::span{ FinalShareEntities.data(),             static_cast<std::size_t>(m_nShareComponents) }
        , std::span{ FinalShareKeys.data(),                 static_cast<std::size_t>(m_nShareComponents) }
        , std::span{ m_InfoData.data() + m_nDataComponents, static_cast<std::size_t>(m_nShareComponents) }
        , std::span{ m_InfoData.data(),                     static_cast<std::size_t>(m_nDataComponents)  }
        );
    }

    //--------------------------------------------------------------------------------------------

    template
    < typename...T_SHARE_COMPONENTS
    > requires
    ( ((xecs::component::type::is_valid_v<T_SHARE_COMPONENTS>) && ...)
        && xecs::tools::all_components_are_share_types_v<T_SHARE_COMPONENTS...>
    )
xecs::pool::family& instance::getOrCreatePoolFamily
    ( T_SHARE_COMPONENTS&&... Components
    ) noexcept
    {
        static_assert( ((std::is_same_v<xcore::types::decay_full_t<T_SHARE_COMPONENTS>, T_SHARE_COMPONENTS>) && ...) );
        using the_tuple    = std::tuple<T_SHARE_COMPONENTS* ... >;
        using sorted_tuple = xecs::component::type::details::template sort_tuple_t< the_tuple >;

        constexpr static auto Infos = []<typename... T>(std::tuple<T...>*) constexpr
        {
            return std::array{ &xecs::component::type::info_v<T> ... };
        }( xcore::types::null_tuple_v<sorted_tuple>);
        
        the_tuple TupleComponents { &Components... };

        auto Args = std::array{ reinterpret_cast<std::byte*>(std::get< xcore::types::tuple_t2i_v< T_SHARE_COMPONENTS*, sorted_tuple> >(TupleComponents)) ... };

        return getOrCreatePoolFamily2( Infos, Args );
    }

    //--------------------------------------------------------------------------------------------
    template
    < typename T_CALLBACK
    > requires
    ( xecs::tools::assert_function_return_v<T_CALLBACK, void>
    ) 
    void
instance::CreateEntity
    ( xecs::pool::family&                                   PoolFamily
    , int                                                   Count
    , T_CALLBACK&&                                          Function
    ) noexcept
    {
        //
        // Allocate entity from one of the pools append pools if need it
        //
        PoolFamily.AppendEntities(Count, m_Mgr.m_GameMgr.m_ArchetypeMgr, [&]( xecs::pool::instance& Pool, xecs::pool::index Index, int nAlloc) noexcept
        {
            //
            // Connected with the global pool
            //
            if (m_Events.m_OnEntityCreated.m_Delegates.size())
            {
                for (int i = 0; i < nAlloc; ++i)
                {
                    xecs::pool::index NewIndex{ Index.m_Value + i };

                    //
                    // Officially add an entity in to the world
                    //
                    auto& Entity = Pool.getComponent<xecs::component::entity>(NewIndex) = m_Mgr.m_GameMgr.m_ComponentMgr.AllocNewEntity(NewIndex, *this, Pool);

                    //
                    // Call the user callback
                    //
                    if constexpr (false == std::is_same_v<xecs::tools::empty_lambda, T_CALLBACK >) Function(Entity, nAlloc);

                    //
                    // Notify anyone that cares
                    //
                    m_Events.m_OnEntityCreated.NotifyAll(Entity);
                }
            }
            else
            {
                for (int i = 0; i < nAlloc; ++i)
                {
                    xecs::pool::index NewIndex{ Index.m_Value + i };

                    //
                    // Officially add an entity in to the world
                    //
                    auto& Entity = Pool.getComponent<xecs::component::entity>(NewIndex) = m_Mgr.m_GameMgr.m_ComponentMgr.AllocNewEntity(NewIndex, *this, Pool);

                    //
                    // Call the user callback
                    //
                    if constexpr (false == std::is_same_v<xecs::tools::empty_lambda, T_CALLBACK >) Function(Entity, nAlloc);
                }
            }
        });
    }

    //--------------------------------------------------------------------------------------------
    xecs::component::entity
instance::CreateEntity
    ( xecs::pool::family&                                   PoolFamily
    , std::span< const xecs::component::type::info* const>  Infos
    , std::span< std::byte* >                               MoveData
    ) noexcept
    {
        assert( xecs::tools::HaveAllComponents(m_ComponentBits, Infos) );
        xecs::component::entity TheEntity;
        instance::CreateEntity( PoolFamily, 1, [&](xecs::component::entity Entity, int )
        {
            auto& Details   = m_Mgr.m_GameMgr.m_ComponentMgr.getEntityDetails( Entity );
            auto& Pool      = *Details.m_pPool;
            auto  InfosSpan = Pool.m_ComponentInfos;

            for( int i=0, j=0; i<Infos.size(); i++ )
            {
                auto&   Info    = *Infos[i];
                auto    iType   = Pool.findIndexComponentFromInfo(Info);
                assert(iType>=0);

                if( MoveData[i] )
                {
                    if( Info.m_pMoveFn )
                    {
                        Info.m_pMoveFn(&Pool.m_pComponent[iType][Details.m_PoolIndex.m_Value * Info.m_Size], MoveData[i]);
                    }
                    else
                    {
                        if( Info.m_pDestructFn ) Info.m_pDestructFn(&Pool.m_pComponent[iType][Details.m_PoolIndex.m_Value * Info.m_Size]);
                        std::memcpy( &Pool.m_pComponent[iType][Details.m_PoolIndex.m_Value * Info.m_Size], MoveData[i], Info.m_Size );
                    }
                }
            }

            TheEntity = Entity;
        });

        return TheEntity;
    }

    //--------------------------------------------------------------------------------------------

    xecs::component::entity instance::CreateEntity
    ( std::span< const xecs::component::type::info* const>  Infos
    , std::span< std::byte* >                               MoveData
    ) noexcept
    {
        // TODO: In order for the system to work we need to call this function for share-components that is why MoveData.size()==1 is there
        // However this is a hack and probably another function like this should be created just for the system.
        assert( m_nShareComponents == 0 || MoveData.size() == 1 );
        return CreateEntity( getOrCreatePoolFamily({},{}), Infos, MoveData);
    }

    //--------------------------------------------------------------------------------------------

    template
    < typename T_CALLBACK
    > requires
    ( xecs::tools::assert_function_return_v<T_CALLBACK, void>
        && xecs::tools::assert_function_args_have_no_share_or_tag_components_v<T_CALLBACK>
        && xecs::tools::assert_function_args_have_only_non_const_references_v<T_CALLBACK>
    )    xecs::component::entity
instance::CreateEntity
    ( T_CALLBACK&& Function 
    ) noexcept
    {
        using func_traits = xcore::function::traits<T_CALLBACK>;
        assert(xecs::tools::HaveAllComponents(m_ComponentBits, xcore::types::null_tuple_v<func_traits::args_tuple>));
        xecs::component::entity TheEntity;

        instance::CreateEntity
        ( getOrCreatePoolFamily( {}, {} )
        , 1
        , [&]
        {
            if constexpr (std::is_same_v<xecs::tools::empty_lambda, T_CALLBACK >) return xecs::tools::empty_lambda{};
            else return [&](xecs::component::entity Entity, int) noexcept
            {
                auto& Details        = m_Mgr.m_GameMgr.m_ComponentMgr.getEntityDetails(Entity);
                auto  CachedPointers = xecs::archetype::details::GetDataComponentPointerArray( *Details.m_pPool, Details.m_PoolIndex, xcore::types::null_tuple_v<func_traits::args_tuple>);
                xecs::archetype::details::CallFunction( Function, CachedPointers );

                TheEntity = Entity;
            };
        }()
        );

        return TheEntity;
    }

    //--------------------------------------------------------------------------------------------
    template
    < typename T_CALLBACK
    > requires
    ( false == xecs::tools::function_has_share_component_args_v<T_CALLBACK>
        && xecs::tools::assert_function_return_v<T_CALLBACK, void>
        && xecs::tools::assert_standard_setter_function_v<T_CALLBACK>
    )
    void
instance::CreateEntities
    ( const int         Count
    , T_CALLBACK&&      Function 
    ) noexcept
    {
        using func_traits = xcore::function::traits<T_CALLBACK>;
        assert(xecs::tools::HaveAllComponents(m_ComponentBits, xcore::types::null_tuple_v<func_traits::args_tuple>));

        instance::CreateEntity( getOrCreatePoolFamily({},{}), Count, [&](xecs::component::entity Entity, int)
        {
            if constexpr (std::is_same_v<xecs::tools::empty_lambda, T_CALLBACK >) return;

            auto& Details        = m_Mgr.m_GameMgr.m_ComponentMgr.getEntityDetails(Entity);
            auto  CachedPointers = xecs::archetype::details::GetDataComponentPointerArray
            ( *Details.m_pPool
            ,  Details.m_PoolIndex
            ,  xcore::types::null_tuple_v<func_traits::args_tuple> 
            );
            xecs::archetype::details::CallFunction( Function, CachedPointers );
        });
    }

    //--------------------------------------------------------------------------------------------

    template
    < typename T_CALLBACK
    > requires
    ( true == xecs::tools::function_has_share_component_args_v<T_CALLBACK>
        && xecs::tools::assert_function_return_v<T_CALLBACK, void>
        && xecs::tools::assert_standard_setter_function_v<T_CALLBACK>
    )
    void
instance::CreateEntities
    ( const int         Count
    , T_CALLBACK&&      Function 
    ) noexcept
    {
        using func_traits = xcore::function::traits<T_CALLBACK>;
        assert(xecs::tools::HaveAllComponents(m_ComponentBits, xcore::types::null_tuple_v<func_traits::args_tuple>));

        using no_refs_tuple     = std::invoke_result_t
        <   decltype([]<typename...T>( std::tuple<T...>* ) ->
            std::tuple
            <   std::remove_reference_t<T> ...
            >{})
        ,   typename func_traits::args_tuple*
        >;

        using share_only_tuple  = xcore::types::tuple_decay_full_t<xecs::component::type::details::share_only_tuple_t<no_refs_tuple>>;
        using data_only_tuple   = xcore::types::tuple_decay_full_t<xecs::component::type::details::data_only_tuple_t<no_refs_tuple>>;
        using data_sorted_tuple = xecs::component::type::details::sort_tuple_t<data_only_tuple>;

        static constexpr auto ShareTypeInfos = [&]<typename...T>(std::tuple<T...>*) constexpr noexcept
        {
            return std::array{ &xecs::component::type::info_v<T> ... };
        }(xcore::types::null_tuple_v<share_only_tuple>);

        const auto ComponentIndex = [&]< typename...T >(std::tuple<T...>*) constexpr noexcept
        {
            int Sequence = 0;
            return std::array
            { [&]<typename J>(J*) constexpr noexcept
                {
                    while( m_InfoData[Sequence] != &xecs::component::type::info_v<J> ) Sequence++;
                    return Sequence;
                }(reinterpret_cast<T*>(0))
                ...
            };
        }(xcore::types::null_tuple_v<data_sorted_tuple>);

        assert( ComponentIndex[0] != -1 );

        // Call the function with the tuple
        for( int i=0; i<Count; i++ )
        {
            no_refs_tuple Components{};

            [&]<typename...T>(std::tuple<T...>& Tuple ) constexpr noexcept
            {
                Function( std::get<T>(Tuple) ... );
            }( Components );

            auto       ShareData      = [&]<typename...T>(std::tuple<T...>*){ return std::array{ reinterpret_cast<std::byte*>( &std::get<T>(Components) ) ... }; }(xcore::types::null_tuple_v<share_only_tuple>);
            auto&      Family         = getOrCreatePoolFamily(ShareTypeInfos, ShareData);

            instance::CreateEntity( Family, 1, [&](xecs::component::entity Entity, int) constexpr noexcept
            {
                auto& Details = m_Mgr.m_GameMgr.m_ComponentMgr.getEntityDetails(Entity);

                // Move all the components
                [&]< typename...T >( std::tuple<T...>* ) constexpr noexcept
                {
                    (( reinterpret_cast<T&>(Details.m_pPool->m_pComponent[ComponentIndex[xcore::types::tuple_t2i_v<T, data_sorted_tuple>]][Details.m_PoolIndex.m_Value * sizeof(T)])
                        = std::move(std::get<T>(Components))), ... );
                }(xcore::types::null_tuple_v<data_sorted_tuple>);
            });
        }
    }

    //--------------------------------------------------------------------------------------------

    template
    < typename T_CALLBACK
    > requires
    ( xecs::tools::assert_function_return_v<T_CALLBACK, void>
        && xecs::tools::assert_function_args_have_no_share_or_tag_components_v<T_CALLBACK>
        && xecs::tools::assert_function_args_have_only_non_const_references_v<T_CALLBACK>
    )
xecs::component::entity instance::CreateEntity
    ( xecs::pool::family&   PoolFamily
    , T_CALLBACK&&          Function
    ) noexcept
    {
        using func_traits = xcore::function::traits<T_CALLBACK>;
        xecs::component::entity TheEntity;

        CreateEntity( PoolFamily, 1, [&]( xecs::component::entity& Entity, int )
        {
            if constexpr (std::is_same_v<xecs::tools::empty_lambda, T_CALLBACK >) return;

            TheEntity = Entity;
            auto& Details        = m_Mgr.m_GameMgr.m_ComponentMgr.getEntityDetails(Entity);
            auto  CachedPointers = xecs::archetype::details::GetDataComponentPointerArray
            ( *Details.m_pPool
            ,  Details.m_PoolIndex
            ,  xcore::types::null_tuple_v<func_traits::args_tuple> 
            );
            xecs::archetype::details::CallFunction( Function, CachedPointers );
        });

        return TheEntity;
    }

    //--------------------------------------------------------------------------------------------

    void instance::DestroyEntity
    ( xecs::component::entity& Entity 
    ) noexcept
    {
        assert( Entity.isZombie() == false );

        //
        // Validate the crap out of the entity
        //
        auto& GlobalEntry = m_Mgr.m_GameMgr.m_ComponentMgr.m_lEntities[Entity.m_GlobalIndex];

        if( Entity.m_Validation != GlobalEntry.m_Validation
        || GlobalEntry.m_pArchetype != this ) return;
        
        // Make sure that we are in sync with the pool entity
        auto& PoolEntity = GlobalEntry.m_pPool->getComponent<xecs::component::entity>(GlobalEntry.m_PoolIndex);
        if( PoolEntity != Entity )
            return;

        //
        // Add pull to the pending list
        //
        m_Mgr.AddToStructuralPendingList( *GlobalEntry.m_pPool );

        //
        // Notify any one that cares
        //
        if(m_Events.m_OnEntityDestroyed.m_Delegates.size())
        {
            m_Events.m_OnEntityDestroyed.NotifyAll(PoolEntity);
        }

        //
        // Make sure everything is marked as zombie
        //
        Entity.m_Validation.m_bZombie
            = PoolEntity.m_Validation.m_bZombie
            = GlobalEntry.m_Validation.m_bZombie
            = true;

        //
        // Delete from pool
        //
        GlobalEntry.m_pPool->Delete( GlobalEntry.m_PoolIndex );
    }

    //--------------------------------------------------------------------------------------------
    template
    < typename T_FUNCTION
    > requires
    ( xecs::tools::assert_function_return_v<T_FUNCTION, void>
        && xecs::tools::assert_function_args_have_no_share_or_tag_components_v<T_FUNCTION>
        && xecs::tools::assert_function_args_have_only_non_const_references_v<T_FUNCTION>
    ) xecs::component::entity
instance::MoveInEntity
    ( xecs::component::entity&  Entity
    , xecs::pool::family&       PoolFamily
    , T_FUNCTION&&              Function 
    ) noexcept
    {
        assert(Entity.isZombie() == false);

        //
        // Deal with events
        //
        {
            auto& GlobalEntity = m_Mgr.m_GameMgr.m_ComponentMgr.m_lEntities[Entity.m_GlobalIndex];
            if( GlobalEntity.m_Validation != Entity.m_Validation ) return { 0xffffffffffffffff };

            auto& FromArchetype = *GlobalEntity.m_pArchetype;
            if( FromArchetype.m_Events.m_OnEntityMovedOut.m_Delegates.size() )
            {
                auto& Pool       = *GlobalEntity.m_pPool;
                auto& PoolEntity = Pool.getComponent<xecs::component::entity>( GlobalEntity.m_PoolIndex );
                m_Events.m_OnEntityMovedOut.NotifyAll(PoolEntity);
                Entity = PoolEntity;
                if( GlobalEntity.m_Validation.m_bZombie ) return { 0xffffffffffffffff };
                if( GlobalEntity.m_Validation != Entity.m_Validation ) return { 0xffffffffffffffff };
            }
        }

        //
        // Ready to move then...
        //
        auto&       GlobalEntity  = m_Mgr.m_GameMgr.m_ComponentMgr.m_lEntities[Entity.m_GlobalIndex];
        auto&       FromArchetype = *GlobalEntity.m_pArchetype;
        auto&       FromPool      = *GlobalEntity.m_pPool;

        //
        // Move entity
        //
        xecs::component::entity NewEntity{ Entity };
        PoolFamily.AppendEntities( 1, m_Mgr, [&]( xecs::pool::instance& Pool, xecs::pool::index Index, int ) noexcept
        {
            //
            // Ok time to work
            //
            m_Mgr.AddToStructuralPendingList(FromPool);
            const auto  NewPoolIndex = Pool.MoveInFromPool( Index, GlobalEntity.m_PoolIndex, FromPool );

            GlobalEntity.m_pArchetype = this;
            GlobalEntity.m_PoolIndex  = NewPoolIndex;
            GlobalEntity.m_pPool      = &Pool;

            if constexpr ( std::is_same_v<T_FUNCTION, xecs::tools::empty_lambda> )
            {
                // Notify any that cares
                if (m_Events.m_OnEntityMovedIn.m_Delegates.size())
                {
                    auto& PoolEntity = Pool.getComponent<xecs::component::entity>(GlobalEntity.m_PoolIndex);
                    m_Events.m_OnEntityMovedIn.NotifyAll(PoolEntity);
                    if (GlobalEntity.m_Validation.m_bZombie) NewEntity = xecs::component::entity{ 0xffffffffffffffff };
                }
            }
            else
            {
                auto CachedPointer = details::GetDataComponentPointerArray
                ( Pool
                , NewPoolIndex
                , xcore::types::null_tuple_v<xcore::function::traits<T_FUNCTION>::args_tuple>
                );

                details::CallFunction
                ( Function
                , CachedPointer
                );

                // Notify any that cares
                if (m_Events.m_OnEntityMovedIn.m_Delegates.size())
                {
                    auto&   PoolEntity  = Pool.getComponent<xecs::component::entity>(GlobalEntity.m_PoolIndex);
                    m_Events.m_OnEntityMovedIn.NotifyAll(PoolEntity);
                    if (GlobalEntity.m_Validation.m_bZombie) NewEntity = xecs::component::entity{ 0xffffffffffffffff };
                }
            }
        });

        return NewEntity;
    }

    //--------------------------------------------------------------------------------------------
    template
    < typename T_FUNCTION
    > requires
    ( xecs::tools::assert_function_return_v<T_FUNCTION, void>
        && xecs::tools::assert_function_args_have_no_share_or_tag_components_v<T_FUNCTION>
        && xecs::tools::assert_function_args_have_only_non_const_references_v<T_FUNCTION>
    ) xecs::component::entity
    instance::MoveInEntity( xecs::component::entity& Entity, T_FUNCTION&& Function ) noexcept
    {
        return MoveInEntity(Entity, getOrCreatePoolFamilyFromDifferentArchetype(Entity), std::forward<T_FUNCTION&&>(Function) );
    }

    //--------------------------------------------------------------------------------------------

    void instance::UpdateStructuralChanges( xecs::pool::family& PoolFamily ) noexcept
    {
        if( m_FamilyHead.get() ) m_FamilyHead->m_pPrev = &PoolFamily;
        PoolFamily.m_Next = std::move(m_FamilyHead);
        m_FamilyHead = std::unique_ptr<xecs::pool::family>{ &PoolFamily };

        //
        // officially announce it
        //
        m_Events.m_OnPoolFamilyCreated.NotifyAll( *this, PoolFamily );
    }

    //--------------------------------------------------------------------------------------------

    xecs::pool::family& instance::getOrCreatePoolFamilyFromDifferentArchetype( xecs::component::entity Entity ) noexcept
    {
        if( m_nShareComponents == 0 )
        {
            if( m_FamilyHead.get() ) return *m_FamilyHead;
            return getOrCreatePoolFamily({},{});
        }

        std::array< int,                                xecs::settings::max_share_components_per_entity_v > IndexArray;
        std::array< std::byte*,                         xecs::settings::max_share_components_per_entity_v > DataArray;
        std::array< const xecs::component::type::info*, xecs::settings::max_share_components_per_entity_v > InfoArray;

        //
        // Compute Indices, Info, and data
        //
        auto&       FromEntityDetails   = m_Mgr.m_GameMgr.m_ComponentMgr.getEntityDetails(Entity);
        auto&       FromFamily          = *FromEntityDetails.m_pPool->m_pMyFamily;
        auto        ToShareInfos        = std::span{ m_InfoData.data() + m_nDataComponents, static_cast<std::size_t>(m_nShareComponents) };
        auto        ToIndices           = std::span{ IndexArray.data(), ToShareInfos.size() };
        auto        ToFinalInfos        = std::span{ InfoArray.data(),  ToShareInfos.size() };
        const auto  FinalCount          = details::ComputeIndexRemaps( ToIndices, ToFinalInfos, ToShareInfos, FromFamily.m_ShareInfos );
        auto        ToData              = std::span{ DataArray.data(), static_cast<std::size_t>(FinalCount) };

        details::CollectSharePointersFromFamily
        ( ToData
        , ToIndices
        , ToFinalInfos
        , FromFamily
        , m_Mgr.m_GameMgr.m_ComponentMgr
        );

        //
        // Get the family
        //
        return getOrCreatePoolFamily( ToShareInfos, ToData );
    }

    //--------------------------------------------------------------------------------------------

    xecs::pool::family& instance::CreateNewPoolFamily
    ( xecs::pool::family::guid                          NewFamilyGuid
    , std::span<xecs::component::entity>                ShareEntityList
    , std::span<xecs::component::type::share::key>      ShareKeyList
    , std::span<const xecs::component::type::info*>     ShareInfoList
    , std::span<const xecs::component::type::info*>     DataInfoList
    ) noexcept
    {
        //
        // Get the memory for the new family
        //
        xecs::pool::family* pPoolFamily;
        if ( m_DefaultPoolFamily2.m_Guid.isNull() )
        {
            // This is scary... but we are trying to keep it consistent
            pPoolFamily = &m_DefaultPoolFamily2;
        }
        else
        {
            pPoolFamily = new xecs::pool::family{};
        }

        ///DEBUG
#if _DEBUG
        {
            xecs::pool::family::guid Guid{ m_Guid.m_Value };
            for (auto& k : ShareKeyList)
            {
                Guid.m_Value += k.m_Value;
            }
            assert(Guid == NewFamilyGuid);
        }
#endif

        //
        // Initialize the family
        //
        pPoolFamily->Initialize
        ( NewFamilyGuid
        , *this
        , std::span{ ShareEntityList.data(),                static_cast<std::size_t>(m_nShareComponents) }
        , std::span{ ShareKeyList.data(),                   static_cast<std::size_t>(m_nShareComponents) }
        , std::span{ m_InfoData.data() + m_nDataComponents, static_cast<std::size_t>(m_nShareComponents) }
        , std::span{ m_InfoData.data(),                     static_cast<std::size_t>(m_nDataComponents)  }
        );

        //
        // Added to the pending list
        //
        m_Mgr.m_PoolFamily.emplace(NewFamilyGuid, pPoolFamily);
        m_Mgr.AddToStructuralPendingList( *pPoolFamily );

        return *pPoolFamily;
    }

}
