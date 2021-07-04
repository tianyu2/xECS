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
        requires( xcore::function::is_callable_v<T_FUNCTION>
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
    }

    //--------------------------------------------------------------------------------------------

    instance::instance
    ( xecs::archetype::mgr& Mgr
    ) noexcept
    : m_Mgr{ Mgr }
    {}

    //--------------------------------------------------------------------------------------------

    void instance::Initialize
    ( std::span<const xecs::component::type::info* const>   Infos
    , const tools::bits&                                    Bits
    , bool                                                  bTreatShareComponentsAsData
    ) noexcept
    {
        // Deep copy the infos just in case the user gave us data driven infos
        bool AreTheySorted = true;
        m_nShareComponents = 0;
        for( int i=0; i<Infos.size(); ++i )
        {
            m_InfoData[i] = Infos[i];
            if( i && AreTheySorted )
            {
                if( (int)m_InfoData[i - 1]->m_TypeID > (int)m_InfoData[i]->m_TypeID )         AreTheySorted = false;
                else if ( m_InfoData[i - 1]->m_Guid.m_Value > m_InfoData[i]->m_Guid.m_Value ) AreTheySorted = false;
            }

            if(m_InfoData[i]->m_TypeID == xecs::component::type::id::SHARE) ++m_nShareComponents;
        }

        // Short Infos base on their GUID (smaller first) entry 0 should be the entity
        if( false == AreTheySorted )
        {
            std::sort
            ( m_InfoData.begin()
            , m_InfoData.begin() + (Infos.size() - 1)
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
            for (int i = 1; i < Infos.size(); ++i)
            {
                // There should be no duplication of components
                assert(m_InfoData[i - 1] != m_InfoData[i]);

                // Check that the bits match
                assert( Bits.getBit(m_InfoData[i]->m_BitID));
            }
        }
#endif

        //
        // We can initialize our default pool
        //
        if( bTreatShareComponentsAsData ) m_nShareComponents = 0;
        if( m_nShareComponents == 0 ) 
        {
            m_DefaultPoolFamily.Initialize( pool::family::guid{42ull}, {}, {}, {}, { m_InfoData.data(), Infos.size() } );
        }

        m_ComponentBits = Bits;
        m_nDataComponents = xcore::types::static_cast_safe<std::uint8_t>(Infos.size()) - m_nShareComponents;
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
            assert(TypeInfos.size() == 0);

            // For these types of archetypes we only have one family
            return m_DefaultPoolFamily;
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
#endif

        //
        // Fast Path... They give me all the data so just execute 
        //
        if( TypeInfos.size() == m_nShareComponents )
        {
            auto FamilyGuid = xecs::pool::family::ComputeGuid
            (   m_Guid
                , TypeInfos
                , MoveData
            );

            if (auto It = m_Mgr.m_PoolFamily.find(FamilyGuid); It != m_Mgr.m_PoolFamily.end())
                return *It->second;
        }

        //
        // Make sure that all the pools for our share components are cached
        //
        if (nullptr == m_ShareArchetypesArray[0].get())
        {
            for (int i = 0; i < m_nShareComponents; ++i)
            {
                // TODO: Add special TAG
                m_ShareArchetypesArray[i] = m_Mgr.getOrCreateArchetype(std::array
                    { &xecs::component::type::info_v<xecs::component::entity>
                    , &xecs::component::type::info_v<xecs::component::ref_count>
                    , m_InfoData[m_nDataComponents + i]
                    });
            }
        }

        //
        // Lets compute all the requires Keys
        //
        std::array<xecs::component::type::share::key,   xecs::settings::max_components_per_entity_v> AllKeys;
        std::array<std::byte*,                          xecs::settings::max_components_per_entity_v> DataInOrder;
        std::array<xecs::component::entity,             xecs::settings::max_components_per_entity_v> ShareComponentEntityRefs;

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
                if(DataInOrder[i]) Entity = m_ShareArchetypesArray[i]->CreateEntity
                ( { &pInfo, 1u }
                , { &DataInOrder[i], 1u }
                );
                else Entity = m_ShareArchetypesArray[i]->CreateEntity({},{});
                m_Mgr.m_ShareComponentEntityMap.emplace(AllKeys[i], Entity);
                ShareComponentEntityRefs[i] = Entity;
            }
            else
            {
                ShareComponentEntityRefs[i] = It->second;
                m_Mgr.m_GameMgr.findEntity(ShareComponentEntityRefs[i], [](xecs::component::ref_count& RefCount )
                {
                    RefCount.m_Value++;
                });
            }
        }

        //
        // Create new Pool Family
        //
        xecs::pool::family* pPoolFamily = nullptr;
        if( m_DefaultPoolFamily.m_Guid.isNull() )
        {
            pPoolFamily = &m_DefaultPoolFamily;
        }
        else
        {
            auto NewFamily = std::make_unique<xecs::pool::family>();
            pPoolFamily    = NewFamily.get();

            if( m_DefaultPoolFamily.m_Next.get() )
            {
                m_DefaultPoolFamily.m_Next->m_pPrev = pPoolFamily;
                NewFamily->m_Next = std::move(m_DefaultPoolFamily.m_Next);
            }

            m_DefaultPoolFamily.m_Next = std::move(NewFamily);
        }
        m_Mgr.m_PoolFamily.emplace(FamilyGuid, pPoolFamily);

        pPoolFamily->Initialize
        ( FamilyGuid
        , std::span{ ShareComponentEntityRefs.data(),       static_cast<std::size_t>(m_nShareComponents) }
        , std::span{ AllKeys.data(),                        static_cast<std::size_t>(m_nShareComponents) }
        , std::span{ m_InfoData.data() + m_nDataComponents, static_cast<std::size_t>(m_nShareComponents) }
        , std::span{ m_InfoData.data(),                     static_cast<std::size_t>(m_nDataComponents)  }
        );

        return *pPoolFamily;
    }

    //--------------------------------------------------------------------------------------------

    xecs::pool::family& 
instance::getOrCreatePoolFamily
    ( xecs::pool::family&                                   FromFamily
    , std::span<int>                                        IndexRemaps
    , std::span< const xecs::component::type::info* const>  TypeInfos
    , std::span< std::byte* >                               MoveData
    , std::span< xecs::component::entity >                  EntitySpan
    , std::span< xecs::component::type::share::key >        Keys
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
        std::array< xecs::component::entity,            xecs::settings::max_components_per_entity_v > FinalShareEntities;
        std::array< xecs::component::type::share::key,  xecs::settings::max_components_per_entity_v > FinalShareKeys;

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
                    (  { TypeInfos[i], 1u }
                     , { MoveData[i],  1u }
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
        auto NewFamily = std::make_unique<xecs::pool::family>();
        m_Mgr.m_PoolFamily.emplace( NewFamilyGuid, NewFamily.get() );

        NewFamily->Initialize
        ( NewFamilyGuid
        , std::span{ FinalShareEntities.data(),             static_cast<std::size_t>(m_nShareComponents) }
        , std::span{ FinalShareKeys.data(),                 static_cast<std::size_t>(m_nShareComponents) }
        , std::span{ m_InfoData.data() + m_nDataComponents, static_cast<std::size_t>(m_nShareComponents) }
        , std::span{ m_InfoData.data(),                     static_cast<std::size_t>(m_nDataComponents)  }
        );

        NewFamily->m_Next = std::move(m_PendingFamilies);
        m_PendingFamilies = std::move(NewFamily);

        return *m_PendingFamilies.get();
    }

    //--------------------------------------------------------------------------------------------

    template
    < typename...T_SHARE_COMPONENTS
    > requires
    ( xecs::tools::all_components_are_share_types_v<T_SHARE_COMPONENTS...>
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
        PoolFamily.AppendEntities(Count, m_Mgr.m_GameMgr.m_ComponentMgr, [&]( xecs::pool::instance& Pool, xecs::pool::index Index, int nAlloc) noexcept
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
        return CreateEntity( m_DefaultPoolFamily, Infos, MoveData);
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
        ( m_DefaultPoolFamily
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

        instance::CreateEntity( m_DefaultPoolFamily, Count, [&](xecs::component::entity Entity, int)
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
            auto&      Family         = getOrCreatePoolFamily2(ShareTypeInfos, ShareData);

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
        // Lock the pool
        //
        xecs::pool::access_guard Lk( *GlobalEntry.m_pPool, m_Mgr.m_GameMgr.m_ComponentMgr );

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
                auto& Pool = *GlobalEntity.m_pPool;

                xecs::pool::access_guard Lk(Pool, m_Mgr.m_GameMgr.m_ComponentMgr );

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
        PoolFamily.AppendEntities( 1, m_Mgr.m_GameMgr.m_ComponentMgr, [&]( xecs::pool::instance& Pool, xecs::pool::index Index, int ) noexcept
        {
            //
            // Ok time to work
            //
            xecs::pool::access_guard Lk1(FromPool,  m_Mgr.m_GameMgr.m_ComponentMgr);

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
        return MoveInEntity(Entity, m_DefaultPoolFamily, std::forward<T_FUNCTION&&>(Function) );
    }

    //-------------------------------------------------------------------------------------
    // ARCHETYPE MANAGER
    //-------------------------------------------------------------------------------------

    //-------------------------------------------------------------------------------------

    mgr::mgr( xecs::game_mgr::instance& GameMgr ) noexcept
    : m_GameMgr{ GameMgr }
    {
    }

    //-------------------------------------------------------------------------------------

    std::shared_ptr<archetype::instance>
mgr::getOrCreateArchetype
    ( std::span<const component::type::info* const> Types 
    ) noexcept
    {
        tools::bits Query;
        xecs::archetype::guid ArchetypeGuid{};
        for (const auto& pE : Types)
        {
            assert(pE->m_BitID != xecs::component::type::info::invalid_bit_id_v );
            Query.setBit(pE->m_BitID);
            ArchetypeGuid.m_Value += pE->m_Guid.m_Value;
        }
            
        // Make sure the entity is part of the list at this point
        assert( Query.getBit(xecs::component::type::info_v<xecs::component::entity>.m_BitID) );

        // Return the archetype
        if( auto I = m_ArchetypeMap.find(ArchetypeGuid); I != m_ArchetypeMap.end() )
            return std::shared_ptr<archetype::instance>{ I->second };

        //
        // Create Archetype...
        //
        m_lArchetype.push_back      ( std::make_shared<archetype::instance>(*this) );
        m_lArchetypeBits.push_back  ( Query );

        auto& Archetype = *m_lArchetype.back();
        Archetype.Initialize(Types, Query, true);

        m_ArchetypeMap.emplace( ArchetypeGuid, &Archetype );

        //
        // Notify anyone interested on the new Archetype
        //
        m_Events.m_OnNewArchetype.NotifyAll(Archetype);

        return m_lArchetype.back();
    }
}
