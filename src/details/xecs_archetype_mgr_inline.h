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
        return CreateArchetype( ArchetypeGuid, ArchetypeComponentBits );
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
        // Update all the pool families
        //
        for (auto p = m_pPoolFamilyPending; p != reinterpret_cast<xecs::pool::family*>(end_structural_changes_v); )
        {
            auto pNext = p->m_pPendingStructuralChanges;
            p->m_pArchetypeInstance->UpdateStructuralChanges( *p );
            p->m_pPendingStructuralChanges = nullptr;
            p = pNext;
        }
        m_pPoolFamilyPending = reinterpret_cast<xecs::pool::family*>(end_structural_changes_v);

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

    inline
    void mgr::AddToStructuralPendingList( pool::family& PoolFamily ) noexcept
    {
        assert(nullptr == PoolFamily.m_pPendingStructuralChanges);
        {
            PoolFamily.m_pPendingStructuralChanges = m_pPoolFamilyPending;
            m_pPoolFamilyPending = &PoolFamily;
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
        // Convert from bits to component Infos
        //
        std::array< const xecs::component::type::info*, xecs::settings::max_components_per_entity_v > ComponentInfos;

        int nComponents = 0;
        int GlobalBit   = 0;
        for( int i=0; i< Bits.m_Bits.size(); ++i )
        {
            std::uint64_t V = Bits.m_Bits[i];
            if (V)
            {
                int nBit = 0;
                do
                {
                    const int c = std::countr_zero(V);
                    nBit += c;
                    ComponentInfos[nComponents++] = xecs::component::mgr::s_BitsToInfo[GlobalBit + nBit];
                    V >>= (1 + c);
                    nBit++;
                } while (V);
            }

            GlobalBit += 32;
        }

        std::sort
        ( ComponentInfos.begin()
        , ComponentInfos.begin() + nComponents - 1
        , xecs::component::type::details::CompareTypeInfos 
        );

        //
        // Create Archetype...
        //
        auto  SharedArchetype = std::make_shared<archetype::instance>(*this);
        auto& Archetype = *SharedArchetype;

        Archetype.Initialize( NewArchetypeGuid, { ComponentInfos.data(), static_cast<std::size_t>(nComponents) }, Bits );

        m_lArchetype.push_back(SharedArchetype);
        m_lArchetypeBits.push_back({ Bits, Archetype.m_ExclusiveTagsBits });
        m_ArchetypeMap.emplace(NewArchetypeGuid, &Archetype);

        // Notify anyone interested
        m_Events.m_OnNewArchetype.NotifyAll(Archetype);

        return SharedArchetype;
    }
}