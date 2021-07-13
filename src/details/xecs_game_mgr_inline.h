namespace xecs::game_mgr
{
    //---------------------------------------------------------------------------

    instance::instance( void ) noexcept
    {
        // Add the System Mgr to the On New Archetype Event
        m_ArchetypeMgr.m_Events.m_OnNewArchetype.Register<&xecs::system::mgr::OnNewArchetype>(m_SystemMgr);
    }

    //---------------------------------------------------------------------------

    template<typename...T_SYSTEMS>
    requires( std::derived_from< T_SYSTEMS, xecs::system::instance> && ... )
    void instance::RegisterSystems() noexcept
    {
        m_ComponentMgr.LockComponentTypes();
        (m_SystemMgr.RegisterSystem<T_SYSTEMS>(*this), ... );
    }

    //---------------------------------------------------------------------------

    template< typename...T_COMPONENTS >
    void instance::RegisterComponents(void) noexcept
    {
        ((m_ComponentMgr.RegisterComponent<T_COMPONENTS>()), ...);
    }

    //---------------------------------------------------------------------------

    template
    < typename...T_GLOBAL_EVENTS
    > requires
    ( std::derived_from< T_GLOBAL_EVENTS, xecs::event::overrides> 
      && ...
    )
    void instance::RegisterGlobalEvents( void ) noexcept
    {
        m_ComponentMgr.LockComponentTypes();
        ((m_EventMgr.Register<T_GLOBAL_EVENTS>()), ...);
    }

    //---------------------------------------------------------------------------

    template
    < typename      T_GLOBAL_EVENT
    , typename...   T_ARGS
    > requires
    ( std::derived_from< T_GLOBAL_EVENT, xecs::event::overrides>
    )
    void instance::SendGlobalEvent( T_ARGS&&... Args ) const noexcept
    {
        m_EventMgr.getEvent<T_GLOBAL_EVENT>().NotifyAll( std::forward<T_ARGS&&>(Args) ... );
    }

    //---------------------------------------------------------------------------

    template
    < typename      T_GLOBAL_EVENT
    > requires
    ( std::derived_from< T_GLOBAL_EVENT, xecs::event::overrides>
    )
    T_GLOBAL_EVENT& instance::getGlobalEvent( void ) const noexcept
    {
        return m_EventMgr.getEvent<T_GLOBAL_EVENT>();
    }

    //---------------------------------------------------------------------------

    void instance::DeleteEntity( xecs::component::entity& Entity ) noexcept
    {
        assert(Entity.isZombie() == false);
        auto& Info = m_ComponentMgr.getEntityDetails(Entity);
        assert(Info.m_Validation == Entity.m_Validation);
        Info.m_pArchetype->DestroyEntity(Entity);
    }


    //---------------------------------------------------------------------------

    template< typename... T_COMPONENTS >
    std::vector<archetype::instance*> instance::Search( const xecs::query::instance& Query ) const noexcept
    {
        std::vector<archetype::instance*> ArchetypesFound;
        for( auto& E : m_ArchetypeMgr.m_lArchetypeBits )
        {
            if( Query.Compare(E.first, E.second) )
            {
                const auto Index = static_cast<std::size_t>(&E - &m_ArchetypeMgr.m_lArchetypeBits[0]);
                ArchetypesFound.push_back(m_ArchetypeMgr.m_lArchetype[Index].get());
            }
        }

        return std::move(ArchetypesFound);
    }

    //---------------------------------------------------------------------------

    archetype::instance& instance::getOrCreateArchetype( std::span<const component::type::info* const> Types ) noexcept
    {
        xecs::tools::bits Bits{};
        for( auto& e : Types )
            Bits.setBit( e->m_BitID );

        // Make sure we always include the entity
        Bits.setBit( xecs::component::type::info_v<xecs::component::entity>.m_BitID );

        return *m_ArchetypeMgr.getOrCreateArchetype(Bits);
    }

    //---------------------------------------------------------------------------
    inline
    archetype::instance* instance::findArchetype( xecs::archetype::guid ArchetypeGuid ) const noexcept
    {
        if ( auto I = m_ArchetypeMgr.m_ArchetypeMap.find(ArchetypeGuid); I != m_ArchetypeMgr.m_ArchetypeMap.end() )
            return I->second;
        return nullptr;
    }

    //---------------------------------------------------------------------------

    template
    < typename... T_TUPLES_OF_COMPONENTS_OR_COMPONENTS
    > requires
    ( 
        (   (  xecs::tools::valid_tuple_components_v<T_TUPLES_OF_COMPONENTS_OR_COMPONENTS>
            || xecs::component::type::is_valid_v<T_TUPLES_OF_COMPONENTS_OR_COMPONENTS> 
            ) &&... )
    )
    archetype::instance& instance::getOrCreateArchetype( void ) noexcept
    {
        return [&]<typename...T>( std::tuple<T...>* ) constexpr noexcept -> archetype::instance&
        {
            xecs::tools::bits Bits;
            Bits.AddFromComponents< T... >();
            return *m_ArchetypeMgr.getOrCreateArchetype(Bits);

        }( xcore::types::null_tuple_v<xecs::component::type::details::combined_t<xecs::component::entity, T_TUPLES_OF_COMPONENTS_OR_COMPONENTS... >>);
    }

    //---------------------------------------------------------------------------
    template
    < typename T_FUNCTION
    > requires
    ( xecs::tools::function_return_v<T_FUNCTION, bool >
        || xecs::tools::function_return_v<T_FUNCTION, void >
    )
    void instance::Foreach(const std::span<xecs::archetype::instance* const> List, T_FUNCTION&& Function ) noexcept
    {
        xecs::query::iterator<T_FUNCTION> Iterator;
        for( const auto& pE : List )
        {
            Iterator.ForeachArchetype( *pE );
            for( auto pFamily = pE->getFamilyHead(); pFamily; pFamily = pFamily->m_Next.get() )
            {
                Iterator.ForeachFamilyPool( m_ComponentMgr, *pFamily );
                for( auto pPool = &pFamily->m_DefaultPool; pPool; pPool = pPool->m_Next.get() )
                {
                    int i = pPool->Size();
                    if(i==0) continue;
                    Iterator.ForeachPool( pE->m_ComponentBits, *pPool );
                    do
                    {
                        if constexpr (std::is_same_v<xecs::query::iterator<T_FUNCTION>::ret_t, bool >)
                        {
                            if( Iterator.CallUserFunction( std::forward<T_FUNCTION&&>(Function) ) )
                                return;
                        }
                        else
                        {
                            Iterator.CallUserFunction(*pE, *pFamily, *pPool, std::forward<T_FUNCTION&&>(Function));
                        }
                    } while(--i);
                }
            }
        }
    }
/*
    //---------------------------------------------------------------------------
    template
    < typename T_FUNCTION
    > requires
    ( xecs::tools::function_return_v<T_FUNCTION, void >
        && true == xecs::tools::function_has_share_component_args_v<T_FUNCTION>
    )
    void instance::Foreach( const std::span<xecs::archetype::instance* const> List, T_FUNCTION&& Function ) noexcept
    {
        using func_traits               = xcore::function::traits<T_FUNCTION>;
        using share_only_tuple          = xecs::component::type::details::share_only_tuple_t<typename func_traits::args_tuple>;
        using data_only_tuple           = xecs::component::type::details::data_only_tuple_t<typename func_traits::args_tuple>;
        using share_sorted_tuple        = xcore::types::tuple_decay_full_t<xecs::component::type::details::sort_tuple_t<share_only_tuple>>;
        using data_sorted_tuple         = xecs::component::type::details::sort_tuple_t<data_only_tuple>;

        std::array<std::byte*,                        std::tuple_size_v<share_sorted_tuple> > SortedSharePointerArray   {};
        std::array<xecs::component::type::share::key, std::tuple_size_v<share_sorted_tuple> > SortedShareKeyArray       {};
        share_sorted_tuple                                                                    SortedShares              {};
        
        const auto UpdatedComponentsBits    = []<typename...T>(std::tuple<T...>*) constexpr noexcept
        {
            xecs::tools::bits UpdatedComponents;
            UpdatedComponents.AddFromComponents<T...>();
            return UpdatedComponents;
        }(xcore::types::null_tuple_v<share_sorted_tuple>);

        for( const auto& pArchetype : List )
        {
            //
            // Create a remap between what the function requires to the original family share components
            // ShareIndices[ Index of the function argument in the sorted order] --> Index of the share component in the family
            //
            const auto ShareIndices = [&]<typename...T>( std::tuple<T...>* ) constexpr noexcept
            {
                const auto  ShareInfos  = std::span{ &pArchetype->m_InfoData[pArchetype->m_nDataComponents], static_cast<std::size_t>(pArchetype->m_nShareComponents) };
                int         Sequence    = 0;
                return std::array
                {
                    [&]<typename J>(J*) constexpr noexcept -> int
                    {
                        int SubSequence = Sequence;
                        while( SubSequence < ShareInfos.size() && ShareInfos[SubSequence] != &xecs::component::type::info_v<J> ) SubSequence++;
                        if(SubSequence == ShareInfos.size()) return -1;
                        Sequence = SubSequence + 1;
                        return SubSequence;
                    }( reinterpret_cast<std::decay_t<T>*>(0) )
                    ...
                };
            }( xcore::types::null_tuple_v<share_sorted_tuple> );

            //
            // Loop through each of the families
            //
            for( auto pFamily = pArchetype->getFamilyHead(); pFamily; pFamily = pFamily->m_Next.get() )
            {
                //
                // Updates all the share pointers if need be
                //
                for( int i=0; i<SortedSharePointerArray.size(); ++i )
                {
                    assert( ShareIndices[i] != -1 );
                    const auto& FamilyShareDetails = pFamily->m_ShareDetails[ShareIndices[i]];
                    if( SortedShareKeyArray[i] != FamilyShareDetails.m_Key )
                    {
                        auto&       EntityDetails   = m_ComponentMgr.getEntityDetails(FamilyShareDetails.m_Entity);
                        const auto  ComponentIndex  = EntityDetails.m_pPool->findIndexComponentFromInfo(*pFamily->m_ShareInfos[ShareIndices[i]]);
                        assert(-1 != ComponentIndex);

                        // Back up the pointer to the data
                        SortedSharePointerArray[i] = &EntityDetails.m_pPool->m_pComponent[ComponentIndex][EntityDetails.m_PoolIndex.m_Value];
                        SortedShareKeyArray[i]     = FamilyShareDetails.m_Key;
                    }
                }

                //
                // Copy the shares into a temporary memory
                //
                [&]<typename... T>(std::tuple<T...>*) constexpr noexcept
                {
                    (([&]<typename J>(J*) constexpr noexcept
                    {
                        const int Index = xcore::types::tuple_t2i_v<J, share_sorted_tuple>;
                        assert( SortedSharePointerArray[Index] );
                        std::get<J>(SortedShares) = reinterpret_cast<J&>(*SortedSharePointerArray[Index]);
                    }( reinterpret_cast<T*>(0) )), ... );
                }(xcore::types::null_tuple_v<share_sorted_tuple>);

                //
                // Compute the key Sum Guid (We will use it to detect if there are changes made in the shares)
                //
                const auto KeySumGuid = xecs::pool::family::guid
                { [&]<typename... T>(std::tuple<T...>*) constexpr noexcept
                    {
                        return (([&]<typename J>(J*) constexpr noexcept
                        {
                            return pFamily->m_ShareDetails[ShareIndices[xcore::types::tuple_t2i_v<J, share_sorted_tuple>]].m_Key.m_Value;
                        }(reinterpret_cast<T*>(0))) + ...);
                    }(xcore::types::null_tuple_v<share_sorted_tuple>)
                };

                //
                // Loop through each of the pools inside the families
                //
                for( auto pPool = &pFamily->m_DefaultPool; pPool; pPool = pPool->m_Next.get() )
                {
                    // If we don't have any entities here just move on
                    int i = pPool->Size();
                    if( i==0 ) 
                        continue;

                    // Lock the pool and cache the data pointers
                    auto CacheDataPointers = archetype::details::GetDataComponentPointerArray( *pPool, pool::index{0}, xcore::types::null_tuple_v<data_only_tuple> );

                    //
                    // Loop for each entity in the pool
                    //
                    for( ; i; --i )
                    {
                        //
                        // Call the user function
                        //
                        [&] <typename...T>(std::tuple<T...>*) constexpr noexcept
                        {
                            static_assert(((std::is_reference_v<T>) && ...));
                            Function
                            (   [&]<typename J>(std::tuple<J>*) constexpr noexcept -> J
                                {
                                    if constexpr (xecs::component::type::info_v<J>.m_TypeID == xecs::component::type::id::SHARE )
                                    {
                                        return std::get<xcore::types::decay_full_t<J>>(SortedShares);
                                    }
                                    else
                                    {
                                        return reinterpret_cast<J>(*CacheDataPointers[xcore::types::tuple_t2i_v<J, typename func_traits::args_tuple>]);
                                    }
                                    
                                }( xcore::types::make_null_tuple_v<T> )
                                ...
                            );

                        }(xcore::types::null_tuple_v<func_traits::args_tuple>);

                        //
                        // Did the user change any of the share components?
                        // If so... we need to move this entity into another family
                        //
                        [&]<typename...T>(std::tuple<T...>*) constexpr noexcept
                        {
                            //
                            // Compute the new Sum Guid which will allow us to detect if something has change
                            //
                            std::array<xecs::component::type::share::key, std::tuple_size_v<share_sorted_tuple> >   UpdatedKeyArray{};
                            xecs::pool::family::guid                                                                NewKeySumGuid{};
                            (([&]<typename J>(J*) constexpr noexcept
                            {
                                const auto Index        = xcore::types::tuple_t2i_v<J, share_sorted_tuple>;
                                UpdatedKeyArray[Index]  = xecs::component::type::details::ComputeShareKey
                                                          ( pArchetype->m_Guid
                                                          , xecs::component::type::info_v<J>
                                                          , reinterpret_cast<const std::byte*>(&std::get<J>(SortedShares))
                                                          );
                                NewKeySumGuid.m_Value  += UpdatedKeyArray[Index].m_Value;

                            }( reinterpret_cast<T*>(nullptr) )), ... );

                            //
                            // Check if we need to change family
                            //
                            if( NewKeySumGuid != KeySumGuid )
                            {
                                std::array<std::byte*, std::tuple_size_v<share_sorted_tuple> >   PointersToShares;

                                // Collect the rest of the data
                                (([&]<typename J>(J*) constexpr noexcept
                                {
                                    const auto Index = xcore::types::tuple_t2i_v<J, share_sorted_tuple>;
                                    PointersToShares[Index] = reinterpret_cast<std::byte*>(&std::get<J>(SortedShares));

                                }(reinterpret_cast<T*>(nullptr))), ...);

                                //
                                // Get the new family and move the entity there
                                //
                                pArchetype->getOrCreatePoolFamilyFromSameArchetype
                                ( *pFamily
                                , UpdatedComponentsBits
                                , PointersToShares
                                , UpdatedKeyArray
                                )
                                .MoveIn
                                ( *this
                                , *pFamily
                                , *pPool
                                , {(pPool->Size() - i)} 
                                );
                            }

                        }(xcore::types::null_tuple_v<share_sorted_tuple>);

                        //
                        // Increment the pointers
                        //
                        [&]<typename...T>(std::tuple<T...>*) constexpr noexcept
                        {
                            ((CacheDataPointers[xcore::types::tuple_t2i_v<T, data_only_tuple>] += sizeof(xcore::types::decay_full_t<T>)), ...);
                        }(xcore::types::null_tuple_v<data_only_tuple>);
                    }
                }
            }
        }
    }
    */

    //---------------------------------------------------------------------------

    template< typename T_FUNCTION>
    requires( xcore::function::is_callable_v<T_FUNCTION> )
    bool instance::findEntity( xecs::component::entity Entity, T_FUNCTION&& Function ) noexcept
    {
        if( Entity.isZombie() ) return false;
        auto& Entry = m_ComponentMgr.m_lEntities[Entity.m_GlobalIndex];
        if( Entry.m_Validation == Entity.m_Validation )
        {
            if constexpr ( !std::is_same_v< T_FUNCTION, xecs::tools::empty_lambda> )
            {
                [&] <typename... T_COMPONENTS>(std::tuple<T_COMPONENTS...>*) constexpr noexcept
                {
                    Function(Entry.m_pPool->getComponent<std::remove_reference_t<T_COMPONENTS>>(Entry.m_PoolIndex) ...);
                }(xcore::types::null_tuple_v<xcore::function::traits<T_FUNCTION>::args_tuple>);
            }
            return true;
        }
        return false;
    }

    //---------------------------------------------------------------------------

    template< typename T_FUNCTION>
    requires( xcore::function::is_callable_v<T_FUNCTION> )
    void instance::getEntity( xecs::component::entity Entity, T_FUNCTION&& Function ) noexcept
    {
        auto b = findEntity( Entity, std::forward<T_FUNCTION&&>(Function) );
        assert(b);
    }

    //---------------------------------------------------------------------------

    template
    < typename T_FUNCTION
    > requires
    ( xcore::function::is_callable_v<T_FUNCTION>
    )
    xecs::component::entity instance::AddOrRemoveComponents
    ( xecs::component::entity                               Entity
    , std::span<const xecs::component::type::info* const>   Add
    , std::span<const xecs::component::type::info* const>   Sub
    , T_FUNCTION&&                                          Function 
    ) noexcept
    {
        xecs::tools::bits AddBits;
        xecs::tools::bits SubBits;

        for( auto& e : Add ) AddBits.setBit(e->m_BitID);
        for (auto& e : Sub ) AddBits.setBit(e->m_BitID);

        return m_ArchetypeMgr.AddOrRemoveComponents
        ( Entity
        , AddBits
        , SubBits
        , std::forward<T_FUNCTION&&>(Function)
        );
    }

    //---------------------------------------------------------------------------

    template
    <   typename T_TUPLE_ADD
    ,   typename T_TUPLE_SUBTRACT
    ,   typename T_FUNCTION
    > requires
    ( xcore::function::is_callable_v<T_FUNCTION>
        && xcore::types::is_specialized_v<std::tuple, T_TUPLE_ADD>
        && xcore::types::is_specialized_v<std::tuple, T_TUPLE_SUBTRACT>
    )
    xecs::component::entity
instance::AddOrRemoveComponents
    ( xecs::component::entity   Entity
    , T_FUNCTION&&              Function 
    ) noexcept
    {
        xecs::tools::bits AddBits;
        xecs::tools::bits SubBits;

        [&]<typename...T>(std::tuple<T...>*){ AddBits.AddFromComponents<T...>(); }( xcore::types::null_tuple_v<T_TUPLE_ADD> );
        [&]<typename...T>(std::tuple<T...>*){ SubBits.AddFromComponents<T...>(); }( xcore::types::null_tuple_v<T_TUPLE_SUBTRACT> );

        if constexpr ( std::is_same_v<T_FUNCTION, xecs::tools::empty_lambda > )
            return m_ArchetypeMgr.AddOrRemoveComponents
            ( Entity
            , AddBits
            , SubBits
            );
        else
            return m_ArchetypeMgr.AddOrRemoveComponents
            ( Entity
            , AddBits
            , SubBits
            , Function
            );
    }

    //---------------------------------------------------------------------------

    void instance::Run( void ) noexcept
    {
        if( m_isRunning == false )
        {
            m_isRunning = true;
            m_ArchetypeMgr.UpdateStructuralChanges();
            m_SystemMgr.m_Events.m_OnGameStart.NotifyAll();
        }

        XCORE_PERF_FRAME_MARK()
        XCORE_PERF_FRAME_MARK_START("ecs::Frame")

        //
        // Run systems
        //
        m_SystemMgr.Run();

        XCORE_PERF_FRAME_MARK_END("ecs::Frame")
    }

    //---------------------------------------------------------------------------

    void instance::Stop(void) noexcept
    {
        if(m_isRunning)
        {
            m_isRunning = false;
            m_SystemMgr.m_Events.m_OnGameEnd.NotifyAll();
        }
    }

    //---------------------------------------------------------------------------
    template< typename T_SYSTEM >
    T_SYSTEM* instance::findSystem( void ) noexcept
    {
        return m_SystemMgr.find<T_SYSTEM>();
    }

    //---------------------------------------------------------------------------
    template< typename T_SYSTEM >
    T_SYSTEM& instance::getSystem(void) noexcept
    {
        auto p = m_SystemMgr.find<T_SYSTEM>();
        assert(p);
        return *p;
    }

}