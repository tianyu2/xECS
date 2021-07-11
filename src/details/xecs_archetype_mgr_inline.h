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
    ( const xecs::tools::bits& ComponentBits
    ) noexcept
    {
        xecs::archetype::guid ArchetypeGuid{ ComponentBits.GenerateUniqueID() };

        // Make sure the entity is part of the list at this point
        assert(ComponentBits.getBit(xecs::component::type::info_v<xecs::component::entity>.m_BitID) );

        // Return the archetype
        if( auto I = m_ArchetypeMap.find(ArchetypeGuid); I != m_ArchetypeMap.end() )
            return std::shared_ptr<archetype::instance>{ I->second };

        //
        // Create Archetype...
        //
        return CreateArchetype( ArchetypeGuid, ComponentBits );
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

    void mgr::AddToStructuralPendingList( instance& Archetype ) noexcept
    {
        if( nullptr == Archetype.m_pPendingStructuralChanges )
        {
            Archetype.m_pPendingStructuralChanges = m_pArchetypeStrututalPending;
            m_pArchetypeStrututalPending = &Archetype;
        }
    }

    //-------------------------------------------------------------------------------------

    void mgr::AddToStructuralPendingList( pool::instance& Pool ) noexcept
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
    ( xecs::component::entity       Entity
    , const xecs::tools::bits&      Add
    , const xecs::tools::bits&      Sub
    , T_FUNCTION&&                  Function 
    ) noexcept
    {
        // Make sure that the entity is not in the list of things to delete
        assert( !Sub.getBit( 0 ) );

        //
        // Get the entry and make sure we are dealing with a valid entity
        //
        assert(Entity.isZombie() == false);
        auto& Entry = m_GameMgr.m_ComponentMgr.getEntityDetails(Entity);
        assert(Entry.m_Validation.m_bZombie == false);

        //
        // Add or remove Components
        //
        auto Bits = Entry.m_pArchetype->m_ComponentBits;
        for( int i=0; i<Bits.m_Bits.size(); ++i )
        {
            Bits.m_Bits[i] = (Bits.m_Bits[i] & ~Sub.m_Bits[i]) | Add.m_Bits[i];
        }

        //
        // Search for the right archetype
        //
        const auto  NewArchetypeGuid = xecs::archetype::guid{ Bits.GenerateUniqueID() };
        if( auto p = findArchetype(NewArchetypeGuid); p )
        {
            if constexpr (std::is_same_v<T_FUNCTION, xecs::tools::empty_lambda >) return p->MoveInEntity(Entity);
            else                                                                  return p->MoveInEntity(Entity, Function);
        }

        //
        // Fail to find the archetype which means that we must build one
        //
        auto& Archetype = *CreateArchetype(NewArchetypeGuid, Bits);

        if constexpr (std::is_same_v<T_FUNCTION, xecs::tools::empty_lambda >) return Archetype.MoveInEntity(Entity);
        else                                                                  return Archetype.MoveInEntity(Entity, Function);
    }

    //-------------------------------------------------------------------------------------

    std::shared_ptr<archetype::instance> mgr::CreateArchetype( archetype::guid NewArchetypeGuid, const tools::bits& Bits ) noexcept
    {
        //
        // Create Archetype...
        //
        auto  SharedArchetype = std::make_shared<archetype::instance>(*this);
        auto& Archetype       = *SharedArchetype;

        m_ArchetypeMap.emplace(NewArchetypeGuid, &Archetype);

        Archetype.Initialize( NewArchetypeGuid, Bits );

        m_lArchetype.push_back(SharedArchetype);
        m_lArchetypeBits.push_back({ Bits, Archetype.m_ExclusiveTagsBits });

        // Notify anyone interested
        m_Events.m_OnNewArchetype.NotifyAll(Archetype);

        return SharedArchetype;
    }
}