namespace xecs::game_mgr
{
    //---------------------------------------------------------------------------

    instance::instance( void ) noexcept
    {
        // Register the Entity
        m_ComponentMgr.RegisterComponent<xecs::component::entity>();

        // Create a link list of empty entries
        for( int i=0, end = xecs::settings::max_entities_v-2; i<end; ++i )
        {
            m_lEntities[i].m_PoolIndex = i+1;
        }
    }

    //---------------------------------------------------------------------------

    template<typename...T_SYSTEMS>
    requires( std::derived_from< T_SYSTEMS, xecs::system::instance> && ... )
    void instance::RegisterSystems() noexcept
    {
        (m_SystemMgr.RegisterSystem<T_SYSTEMS>(*this), ... );
    }

    //---------------------------------------------------------------------------

    template< typename...T_COMPONENTS >
    void instance::RegisterComponents(void) noexcept
    {
        ((m_ComponentMgr.RegisterComponent<T_COMPONENTS>()), ...);
    }

    //---------------------------------------------------------------------------

    xecs::component::entity instance::AllocNewEntity( int PoolIndex, xecs::archetype::instance& Archetype ) noexcept
    {
        assert(m_EmptyHead>=0);

        auto  iEntityIndex  = m_EmptyHead;
        auto& Entry         = m_lEntities[iEntityIndex];
        m_EmptyHead = Entry.m_PoolIndex;

        Entry.m_PoolIndex   = PoolIndex;
        Entry.m_pArchetype  = &Archetype;
        return
        {
            .m_GlobalIndex        = static_cast<std::uint32_t>(iEntityIndex)
        ,   .m_Validation         = Entry.m_Validation
        };
    }

    //---------------------------------------------------------------------------

    const entity_info& instance::getEntityDetails( xecs::component::entity Entity ) const noexcept
    {
        auto& Entry = m_lEntities[Entity.m_GlobalIndex];
        assert( Entry.m_Validation == Entity.m_Validation );
        return Entry;
    }

    //---------------------------------------------------------------------------

    void instance::DeleteEntity( xecs::component::entity& Entity ) noexcept
    {
        assert(Entity.isZombie() == false);
        auto& Info = getEntityDetails(Entity);
        assert(Info.m_Validation == Entity.m_Validation);
        Info.m_pArchetype->DestroyEntity(Entity);
    }

    //---------------------------------------------------------------------------

    void instance::DeleteGlobalEntity( std::uint32_t GlobalIndex, xecs::component::entity& SwappedEntity ) noexcept
    {
        auto& Entry = m_lEntities[GlobalIndex];
        m_lEntities[SwappedEntity.m_GlobalIndex].m_PoolIndex = Entry.m_PoolIndex;

        Entry.m_Validation.m_Generation++;
        Entry.m_Validation.m_bZombie = false;
        Entry.m_PoolIndex            = m_EmptyHead;
        m_EmptyHead = static_cast<int>(GlobalIndex);
    }

    //---------------------------------------------------------------------------

    void instance::DeleteGlobalEntity(std::uint32_t GlobalIndex ) noexcept
    {
        auto& Entry = m_lEntities[GlobalIndex];
        Entry.m_Validation.m_Generation++;
        Entry.m_Validation.m_bZombie = false;
        Entry.m_PoolIndex            = m_EmptyHead;
        m_EmptyHead = static_cast<int>(GlobalIndex);
    }

    //---------------------------------------------------------------------------

    void instance::MovedGlobalEntity(std::uint32_t PoolIndex, xecs::component::entity& SwappedEntity ) noexcept
    {
        m_lEntities[SwappedEntity.m_GlobalIndex].m_PoolIndex = PoolIndex;
    }

    //---------------------------------------------------------------------------

    std::vector<archetype::instance*> instance::Search( std::span<const component::info* const> Types ) const noexcept
    {
        tools::bits Query;
        for( const auto& pE : Types )
            Query.setBit( pE->m_BitID );

        std::vector<archetype::instance*> ArchetypesFound;
        for (auto& E : m_lArchetypeBits)
        {
            if( E.Equals(Query) )
            {
                const auto Index = static_cast<std::size_t>(&E - &m_lArchetypeBits[0]);
                ArchetypesFound.push_back(m_lArchetype[Index].get());
            }
        }

        return std::move(ArchetypesFound);
    }

    //---------------------------------------------------------------------------

    template< typename... T_COMPONENTS >
    std::vector<archetype::instance*> instance::Search( void ) const noexcept
    {
        static constexpr auto ComponentList = std::array{ &component::info_v<T_COMPONENTS>... };
        return Search(ComponentList);
    }

    //---------------------------------------------------------------------------

    template< typename... T_COMPONENTS >
    std::vector<archetype::instance*> instance::Search( const xecs::query::instance& Query ) const noexcept
    {
        std::vector<archetype::instance*> ArchetypesFound;
        for( auto& E : m_lArchetypeBits )
        {
            if( Query.Compare(E) )
            {
                const auto Index = static_cast<std::size_t>(&E - &m_lArchetypeBits[0]);
                ArchetypesFound.push_back(m_lArchetype[Index].get());
            }
        }

        return std::move(ArchetypesFound);
    }

    //---------------------------------------------------------------------------

    archetype::instance& instance::getOrCreateArchetype( std::span<const component::info* const> Types ) noexcept
    {
        tools::bits Query;
        for (const auto& pE : Types)
        {
            assert(pE->m_BitID != xecs::component::info::invalid_bit_id_v );
            Query.setBit(pE->m_BitID);
        }
            
        // Make sure the entity is part of the list at this point
        assert( Query.getBit(xecs::component::info_v<xecs::component::entity>.m_BitID) );
        
        for( auto& E : m_lArchetypeBits )
        {
            if ( E.Equals(Query) )
            {
                const auto Index = static_cast<std::size_t>( &E - m_lArchetypeBits.data() );
                return *m_lArchetype[Index];
            }
        }

        //
        // Create Archetype...
        //
        m_lArchetype.push_back      ( std::make_unique<archetype::instance>(*this) );
        m_lArchetypeBits.push_back  ( Query );

        auto& Archetype = *m_lArchetype.back();
        Archetype.Initialize(Types, Query);

        return Archetype;
    }

    //---------------------------------------------------------------------------

    template< typename... T_COMPONENTS >
    archetype::instance& instance::getOrCreateArchetype( void ) noexcept
    {
        static_assert( ((std::is_same_v<T_COMPONENTS, xecs::component::entity> == false ) && ...) );
        return getOrCreateArchetype
        ( xecs::component::details::sorted_info_array_v< xecs::component::details::combined_t<xecs::component::entity, T_COMPONENTS... >> );
    }

    //---------------------------------------------------------------------------

    template
    <   typename T_FUNCTION
    > requires
    (   xcore::function::is_callable_v<T_FUNCTION> 
    &&  std::is_same_v< bool, typename xcore::function::traits<T_FUNCTION>::return_type >
    )
    void instance::Foreach( const std::vector<xecs::archetype::instance*>& List, T_FUNCTION&& Function ) const noexcept
    {
        using func_traits = xcore::function::traits<T_FUNCTION>;
        
        for( const auto& pE : List )
        {
            const auto& Pool = pE->m_Pool;
            auto        CachePointers = archetype::details::GetComponentPointerArray(Pool, 0, xcore::types::null_tuple_v<func_traits::args_tuple>);

            bool bBreak = false;
            pE->AccessGuard([&]
            {
                for( int i=Pool.Size(); i; --i )
                {
                    if( archetype::details::CallFunction(Function, CachePointers) )
                    {
                        bBreak = true;
                        break;
                    }
                }
            });
            if(bBreak) break;
        }
    }

    //---------------------------------------------------------------------------
    template
    < typename T_FUNCTION
    > requires
    ( xcore::function::is_callable_v<T_FUNCTION>
    && std::is_same_v< void, typename xcore::function::traits<T_FUNCTION>::return_type >
    )
    void instance::Foreach( const std::vector<xecs::archetype::instance*>& List, T_FUNCTION&& Function ) const noexcept
    {
        using func_traits = xcore::function::traits<T_FUNCTION>;
        
        for( const auto& pE : List )
        {
            const auto& Pool = pE->m_Pool;
            auto        CachePointers = archetype::details::GetComponentPointerArray(Pool, 0, xcore::types::null_tuple_v<func_traits::args_tuple> );
            pE->AccessGuard([&]
            {
                for( int i=Pool.Size(); i; --i )
                {
                    archetype::details::CallFunction(Function, CachePointers);
                }
            });
        }
    }

    //---------------------------------------------------------------------------

    template< typename T_FUNCTION>
    requires( xcore::function::is_callable_v<T_FUNCTION> )
    bool instance::findEntity( xecs::component::entity Entity, T_FUNCTION&& Function ) const noexcept
    {
        if( Entity.isZombie() ) return false;
        auto& Entry = m_lEntities[Entity.m_GlobalIndex];
        if( Entry.m_Validation == Entity.m_Validation )
        {
            if constexpr ( !std::is_same_v< T_FUNCTION, xecs::tools::empty_lambda> )
            {
                Entry.m_pArchetype->AccessGuard([&]
                {
                    [&] <typename... T_COMPONENTS>(std::tuple<T_COMPONENTS...>*) constexpr noexcept
                    {
                        Function( Entry.m_pArchetype->m_Pool.getComponent<std::remove_reference_t<T_COMPONENTS>>(Entry.m_PoolIndex) ...);
                    }(xcore::types::null_tuple_v<xcore::function::traits<T_FUNCTION>::args_tuple>);
                });
            }
            return true;
        }
        return false;
    }

    //---------------------------------------------------------------------------

    template
    < typename T_FUNCTION
    > requires
    ( xcore::function::is_callable_v<T_FUNCTION>
    )
    void instance::AddOrRemoveComponents( xecs::component::entity                       Entity
                                        , std::span<const xecs::component::info* const> Add
                                        , std::span<const xecs::component::info* const> Sub
                                        , T_FUNCTION&&                                  Function ) noexcept
    {
        auto& Entry = getEntityDetails(Entity);
        auto  Bits  = Entry.m_pArchetype->m_ComponentBits;

        for( auto& pE : Add ) 
        {
            // Cant add the entity
            assert( pE->m_BitID !=0 );
            Bits.setBit( pE->m_BitID );
        }
        for( auto& pE : Sub ) 
        {
            // Cant remove the entity
            assert(pE->m_BitID != 0);
            Bits.clearBit(pE->m_BitID);
        }
        for( auto& E : m_lArchetypeBits )
        {
            if( E.Equals(Bits) )
            {
                const auto Index = static_cast<std::size_t>(&E - &m_lArchetypeBits[0]);
                if constexpr (std::is_same_v<T_FUNCTION, xecs::tools::empty_lambda >) m_lArchetype[Index]->MoveInEntity( Entity );
                else                                                                  m_lArchetype[Index]->MoveInEntity( Entity, Function );
                return;
            }
        }

        std::array<const xecs::component::info*, xecs::settings::max_components_per_entity_v > ComponentList;
        int Count = 0;

        // Copy the existing ones
        for( auto& pE : std::span{ Entry.m_pArchetype->m_InfoData.data(), Entry.m_pArchetype->m_nComponents } ) 
            ComponentList[Count++] = pE;

        // Add
        for( auto& pE : Add )
        {
            const auto Index = static_cast<std::size_t>(std::upper_bound(ComponentList.begin(), ComponentList.begin() + Count, pE, [](const xecs::component::info* pA, const xecs::component::info* pB)
            {
                return pA->m_Guid < pB->m_Guid;
            }) - ComponentList.begin());
            assert(Index > 0);

            // Check for duplicates
            if( ComponentList[Index - 1] == pE )
                continue;

            // Create a hole to insert our entry
            if( Index != Count )
            {
                std::memmove( &ComponentList[Index+1], &ComponentList[Index], (Count - Index) * sizeof(xecs::component::info*) );
            }
            ComponentList[Index] = pE;
            Count++;
        }

        // Remove
        for (auto& pE : Sub)
        {
            const auto Index = static_cast<std::size_t>(std::upper_bound(ComponentList.begin(), ComponentList.begin() + Count, pE, [](const xecs::component::info* pA, const xecs::component::info* pB)
            {
                return pA->m_Guid < pB->m_Guid;
            }) - ComponentList.begin());
            assert(Index > 0);

            // Check if we found it
            if ( ComponentList[Index - 1] == pE )
            {
                std::memmove(&ComponentList[Index-1], &ComponentList[Index], (Count - Index) * sizeof(xecs::component::info*));
                Count--;
            }
        }

        //
        // Create Archetype...
        //
        m_lArchetype.push_back(std::make_unique<archetype::instance>(*this));
        m_lArchetypeBits.push_back(Bits);

        auto& Archetype = *m_lArchetype.back();
        Archetype.Initialize({ ComponentList.data(), static_cast<std::size_t>(Count) }, Bits);
        if constexpr (std::is_same_v<T_FUNCTION, xecs::tools::empty_lambda >) Archetype.MoveInEntity(Entity);
        else                                                                  Archetype.MoveInEntity(Entity, Function);
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
    void instance::AddOrRemoveComponents( xecs::component::entity   Entity
                                        , T_FUNCTION&&              Function 
                                        ) noexcept
    {
        if constexpr ( std::is_same_v<T_FUNCTION, xecs::tools::empty_lambda > )
            AddOrRemoveComponents
            ( Entity
            , xecs::component::details::sorted_info_array_v<T_TUPLE_ADD>
            , xecs::component::details::sorted_info_array_v<T_TUPLE_SUBTRACT>
            );
        else
            AddOrRemoveComponents
            ( Entity
            , xecs::component::details::sorted_info_array_v<T_TUPLE_ADD>
            , xecs::component::details::sorted_info_array_v<T_TUPLE_SUBTRACT>
            , Function
            );
    }

    //---------------------------------------------------------------------------

    void instance::Run( void ) noexcept
    {
        XCORE_PERF_FRAME_MARK()
        XCORE_PERF_FRAME_MARK_START("ecs::Frame")

        //
        // Run systems
        //
        m_SystemMgr.Run();

        XCORE_PERF_FRAME_MARK_END("ecs::Frame")
    }
}