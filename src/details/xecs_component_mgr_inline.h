namespace xecs::component
{
    //------------------------------------------------------------------------------
    // COMPONENT MGR
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------

    mgr::mgr(void) noexcept
    {
        // Register Key system components
        RegisterComponent<xecs::component::entity>();
        RegisterComponent<xecs::component::share_as_data_exclusive_tag>();
        RegisterComponent<xecs::component::ref_count>();
        RegisterComponent<xecs::component::share_filter>();

        // Create a link list of empty entries
        for (int i = 0, end = xecs::settings::max_entities_v - 2; i < end; ++i)
        {
            m_lEntities[i].m_PoolIndex = xecs::pool::index{ i + 1 };
        }
    }

    //------------------------------------------------------------------------------

    template< typename T_COMPONENT >
    requires (xecs::component::type::is_valid_v<T_COMPONENT>)
    void mgr::RegisterComponent(void) noexcept
    {
        assert( s_isLocked == false );
        if (component::type::info_v<T_COMPONENT>.m_BitID == type::info::invalid_bit_id_v)
        {
            if constexpr( component::type::info_v<T_COMPONENT>.m_TypeID == xecs::component::type::id::SHARE )
            {
                T_COMPONENT X{};
                component::type::info_v<T_COMPONENT>.m_DefaultShareKey = component::type::info_v<T_COMPONENT>.m_pComputeKeyFn(reinterpret_cast<const std::byte*>(&X));
            }

            // Put there an invalid bitUD that indicates that we are waiting to be assign the right ID
            component::type::info_v<T_COMPONENT>.m_BitID = type::info::invalid_bit_id_v-1;

            s_BitsToInfo[s_nTypes++] = &component::type::info_v<T_COMPONENT>;
        }
    }

    //---------------------------------------------------------------------------

    const entity::global_info& mgr::getEntityDetails( entity Entity ) const noexcept
    {
        assert( Entity.isValid() );
        auto& Entry = m_lEntities[Entity.m_GlobalIndex];
        assert(Entry.m_Validation == Entity.m_Validation);
        return Entry;
    }

    //---------------------------------------------------------------------------

    void mgr::DeleteGlobalEntity( std::uint32_t GlobalIndex, xecs::component::entity& SwappedEntity ) noexcept
    {
        auto& Entry = m_lEntities[GlobalIndex];
        m_lEntities[SwappedEntity.m_GlobalIndex].m_PoolIndex = Entry.m_PoolIndex;

        Entry.m_Validation.m_Generation++;
        Entry.m_Validation.m_bZombie = false;
        Entry.m_PoolIndex.m_Value    = m_EmptyHead;
        m_EmptyHead = static_cast<int>(GlobalIndex);
    }

    //---------------------------------------------------------------------------

    void mgr::DeleteGlobalEntity(std::uint32_t GlobalIndex ) noexcept
    {
        auto& Entry = m_lEntities[GlobalIndex];
        Entry.m_Validation.m_Generation++;
        Entry.m_Validation.m_bZombie = false;
        Entry.m_PoolIndex.m_Value    = m_EmptyHead;
        m_EmptyHead = static_cast<int>(GlobalIndex);
    }

    //---------------------------------------------------------------------------

    void mgr::MovedGlobalEntity( xecs::pool::index PoolIndex, xecs::component::entity& SwappedEntity ) noexcept
    {
        m_lEntities[SwappedEntity.m_GlobalIndex].m_PoolIndex = PoolIndex;
    }

    //---------------------------------------------------------------------------

    entity mgr::AllocNewEntity( pool::index PoolIndex, xecs::archetype::instance& Archetype, xecs::pool::instance& Pool) noexcept
    {
        assert(m_EmptyHead >= 0);

        auto  iEntityIndex = m_EmptyHead;
        auto& Entry        = m_lEntities[iEntityIndex];
        m_EmptyHead = Entry.m_PoolIndex.m_Value;

        Entry.m_PoolIndex  = PoolIndex;
        Entry.m_pArchetype = &Archetype;
        Entry.m_pPool      = &Pool;
        return
        {
            .m_GlobalIndex = static_cast<std::uint32_t>(iEntityIndex)
        ,   .m_Validation = Entry.m_Validation
        };
    }

    //---------------------------------------------------------------------------

    inline
    void mgr::LockComponentTypes( void ) noexcept
    {
        if(s_isLocked) return;
        s_isLocked = true;

        //
        // Short the final list of component types infos 
        //
        std::sort
        ( s_BitsToInfo.begin()
        , s_BitsToInfo.begin() + s_nTypes
        , xecs::component::type::details::CompareTypeInfos
        );

        //
        // Officially register each of the components
        //
        for( int i=0; i<s_nTypes; ++i )
        {
            // Everyone should be waiting for us to assign their BitID
            assert( s_BitsToInfo[i]->m_BitID == (type::info::invalid_bit_id_v - 1) );

            // Ok we just officially assing their ID now in shorted order
            s_BitsToInfo[i]->m_BitID = i;

            // Now we are ready to assign the IDs...
            switch( s_BitsToInfo[i]->m_TypeID )
            {
                case xecs::component::type::id::DATA:           s_DataBits.setBit(s_BitsToInfo[i]->m_BitID);
                                                                break;
                case xecs::component::type::id::TAG:            s_TagsBits.setBit(s_BitsToInfo[i]->m_BitID);
                                                                if( s_BitsToInfo[i]->m_bExclusiveTag )
                                                                    s_ExclusiveTagsBits.setBit(s_BitsToInfo[i]->m_BitID);
                                                                break;
                case xecs::component::type::id::SHARE:          s_ShareBits.setBit(s_BitsToInfo[i]->m_BitID);
                                                                break;
                default: assert(false);
            }
        }
    }
}