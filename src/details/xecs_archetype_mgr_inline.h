namespace xecs::archetype
{
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
        tools::bits ArchetypeComponentBits;
        for (const auto& pE : Types)
        {
            assert(pE->m_BitID != xecs::component::type::info::invalid_bit_id_v );
            ArchetypeComponentBits.setBit(pE->m_BitID);
        }

        xecs::archetype::guid ArchetypeGuid{ ArchetypeComponentBits.GenerateUniqueID() };

        // Make sure the entity is part of the list at this point
        assert(ArchetypeComponentBits.getBit(xecs::component::type::info_v<xecs::component::entity>.m_BitID) );

        // Return the archetype
        if( auto I = m_ArchetypeMap.find(ArchetypeGuid); I != m_ArchetypeMap.end() )
            return std::shared_ptr<archetype::instance>{ I->second };

        //
        // Create Archetype...
        //
        auto SharedArchetype  = std::make_shared<archetype::instance>(*this);
        auto& Archetype       = *SharedArchetype;

        Archetype.Initialize( ArchetypeGuid, Types, ArchetypeComponentBits );

        m_lArchetype.push_back( std::move(SharedArchetype) );
        m_lArchetypeBits.push_back( {ArchetypeComponentBits, Archetype.m_ExclusiveTagsBits} );

        m_ArchetypeMap.emplace( ArchetypeGuid, &Archetype );

        //
        // Notify anyone interested on the new Archetype
        //
        m_Events.m_OnNewArchetype.NotifyAll(Archetype);

        return m_lArchetype.back();
    }

    //-------------------------------------------------------------------------------------

    void mgr::UpdateStructuralChanges( void ) noexcept
    {
        //
        // Update all the pools
        //
        for (auto p = m_pPoolStructuralPending; p != reinterpret_cast<xecs::pool::instance*>(end_structural_changes_v); )
        {
            auto pNext = p->m_pPendingStructuralChanges;
            p->UpdateStructuralChanges(m_GameMgr.m_ComponentMgr);
            p->m_pPendingStructuralChanges = nullptr;
            p = pNext;
        }
        m_pPoolStructuralPending = reinterpret_cast<xecs::pool::instance*>(end_structural_changes_v);

        //
        // Update all the archetypes
        //
        for( auto p = m_pArchetypeStrututalPending; p != reinterpret_cast<xecs::archetype::instance*>(end_structural_changes_v); )
        {
            auto pNext = p->m_pPendingStructuralChanges;
            p->UpdateStructuralChanges();
            p->m_pPendingStructuralChanges = nullptr;
            p = pNext;
        }
        m_pArchetypeStrututalPending = reinterpret_cast<xecs::archetype::instance*>(end_structural_changes_v);
    }

    //-------------------------------------------------------------------------------------

    void mgr::AddToStructutalPendingList( instance& Archetype ) noexcept
    {
        if( nullptr == Archetype.m_pPendingStructuralChanges )
        {
            Archetype.m_pPendingStructuralChanges = m_pArchetypeStrututalPending;
            m_pArchetypeStrututalPending = &Archetype;
        }
    }

    //-------------------------------------------------------------------------------------

    void mgr::AddToStructutalPendingList( pool::instance& Pool ) noexcept
    {
        if( nullptr == Pool.m_pPendingStructuralChanges )
        {
            Pool.m_pPendingStructuralChanges = m_pPoolStructuralPending;
            m_pPoolStructuralPending = &Pool;
        }
    }

    //-------------------------------------------------------------------------------------
    __inline
    archetype::instance* mgr::findArchetype( xecs::archetype::guid Guid ) noexcept
    {
        auto It = m_ArchetypeMap.find( Guid );
        return It == m_ArchetypeMap.end() ? nullptr : It->second;
    }

    //-------------------------------------------------------------------------------------

    template
    < typename T_FUNCTION
    > requires
    ( xcore::function::is_callable_v<T_FUNCTION>
    )
    xecs::component::entity
mgr::AddOrRemoveComponents
    ( xecs::component::entity                               Entity
    , std::span<const xecs::component::type::info* const>   Add
    , std::span<const xecs::component::type::info* const>   Sub
    , T_FUNCTION&&                                          Function 
    ) noexcept
    {
        assert(Entity.isZombie() == false);
        auto& Entry = m_GameMgr.m_ComponentMgr.getEntityDetails(Entity);
        auto  Bits  = Entry.m_pArchetype->m_ComponentBits;
        assert(Entry.m_Validation.m_bZombie == false);

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

        //
        // Search for the right archetype
        //
        if( auto p = findArchetype( archetype::guid{ Bits.GenerateUniqueID() } ); p )
        {
            if constexpr (std::is_same_v<T_FUNCTION, xecs::tools::empty_lambda >) return p->MoveInEntity(Entity);
            else                                                                  return p->MoveInEntity(Entity, Function);
        }

        //
        // Fail to find the archetype which means that we must build one
        //
        std::array<const xecs::component::type::info*, xecs::settings::max_components_per_entity_v > ComponentList;
        int Count = 0;

        // Copy the existing ones
        for( auto& pE : std::span{ Entry.m_pArchetype->m_InfoData.data(), (std::size_t)Entry.m_pArchetype->m_nDataComponents + (std::size_t)Entry.m_pArchetype->m_nShareComponents } )
            ComponentList[Count++] = pE;

        assert(Count);

        // Add
        for( auto& pE : Add )
        {
            const auto Index = static_cast<std::size_t>(std::upper_bound(ComponentList.begin(), ComponentList.begin() + Count -1, pE, [](const xecs::component::type::info* pA, const xecs::component::type::info* pB)
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
                std::memmove( &ComponentList[Index+1], &ComponentList[Index], (Count - Index) * sizeof(xecs::component::type::info*) );
            }
            ComponentList[Index] = pE;
            Count++;
        }

        // Remove
        for (auto& pE : Sub)
        {
            const auto Index = static_cast<std::size_t>(std::upper_bound(ComponentList.begin(), ComponentList.begin() + Count -1 , pE, [](const xecs::component::type::info* pA, const xecs::component::type::info* pB)
            {
                return pA->m_Guid < pB->m_Guid;
            }) - ComponentList.begin());
            assert(Index > 0);

            // Check if we found it
            if ( ComponentList[Index - 1] == pE )
            {
                std::memmove(&ComponentList[Index-1], &ComponentList[Index], (Count - Index) * sizeof(xecs::component::type::info*));
                Count--;
            }
        }

        //
        // Create Archetype...
        //
        auto  SharedArchetype   = std::make_shared<archetype::instance>(*this);
        auto& Archetype         = *SharedArchetype;
        auto  ArchetypeGuid     = xecs::archetype::guid{ Bits.GenerateUniqueID() };

        Archetype.Initialize( ArchetypeGuid, { ComponentList.data(), static_cast<std::size_t>(Count) }, Bits);

        m_lArchetype.push_back( std::move(SharedArchetype) );
        m_lArchetypeBits.push_back( {Bits, Archetype.m_ExclusiveTagsBits} );
        m_ArchetypeMap.emplace( ArchetypeGuid, &Archetype );

        // Notify anyone intested
        m_Events.m_OnNewArchetype.NotifyAll(Archetype);

        if constexpr (std::is_same_v<T_FUNCTION, xecs::tools::empty_lambda >) return Archetype.MoveInEntity(Entity);
        else                                                                  return Archetype.MoveInEntity(Entity, Function);
    }


}