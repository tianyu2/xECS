namespace xecs::component
{
    //------------------------------------------------------------------------------
    // COMPONENT MGR
    //------------------------------------------------------------------------------

    //------------------------------------------------------------------------------

    mgr::mgr(void) noexcept
    {
        // Register the Entity
        RegisterComponent<xecs::component::entity>();
        RegisterComponent<xecs::component::share_as_data_exclusive_tag>();
        RegisterComponent<xecs::component::ref_count>();

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
        if (component::type::info_v<T_COMPONENT>.m_BitID == type::info::invalid_bit_id_v)
        {
            component::type::info_v<T_COMPONENT>.m_BitID = m_UniqueID++;

            if constexpr( component::type::info_v<T_COMPONENT>.m_TypeID == xecs::component::type::id::SHARE )
            {
                T_COMPONENT X{};
                component::type::info_v<T_COMPONENT>.m_DefaultShareKey = component::type::info_v<T_COMPONENT>.m_pComputeKeyFn(reinterpret_cast<const std::byte*>(&X));
            }

            switch( component::type::info_v<T_COMPONENT>.m_TypeID )
            {
                case xecs::component::type::id::DATA:           m_DataBits.setBit(component::type::info_v<T_COMPONENT>.m_BitID);
                                                                break;
                case xecs::component::type::id::TAG:            m_TagsBits.setBit(component::type::info_v<T_COMPONENT>.m_BitID);
                                                                if( component::type::info_v<T_COMPONENT>.m_bExclusiveTag )
                                                                    m_ExclusiveTagsBits.setBit(component::type::info_v<T_COMPONENT>.m_BitID);
                                                                break;
                case xecs::component::type::id::SHARE:          m_ShareBits.setBit(component::type::info_v<T_COMPONENT>.m_BitID);
                                                                break;
                default: assert(false);
            }
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
}