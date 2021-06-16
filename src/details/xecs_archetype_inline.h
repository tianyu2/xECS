namespace xecs::archetype
{
    //--------------------------------------------------------------------------------------------

    instance::instance
    ( xecs::game_mgr::instance& GameMgr 
    ) noexcept
    : m_GameMgr{ GameMgr }
    {}

    //--------------------------------------------------------------------------------------------

    template< typename T_FUNCTION >
    requires
    ( xcore::function::is_callable_v<T_FUNCTION>
    && std::is_same_v<typename xcore::function::traits<T_FUNCTION>::return_type, void>
    && xcore::function::traits<T_FUNCTION>::arg_count_v == 0
    )
    void instance::AccessGuard( T_FUNCTION&& Function ) noexcept
    {
        ++m_ProcessReference;
        Function();
        if(--m_ProcessReference == 0 ) UpdateStructuralChanges();
    }

    //--------------------------------------------------------------------------------------------

    void instance::Initialize
    ( std::span<const xecs::component::info* const>  Infos
    , const tools::bits&                            Bits 
    ) noexcept
    {
        // Deep copy the infos just in case the user gave us data driven infos
        for( int i=0; i<Infos.size(); ++i )
        {
            m_InfoData[i] = Infos[i];
        }

        m_Pool.Initialize( { m_InfoData.data(), Infos.size() } );
        m_ComponentBits = Bits;
    }

    //--------------------------------------------------------------------------------------------

    template
    < typename T_CALLBACK
    > requires
    ( xcore::function::is_callable_v<T_CALLBACK>
   && std::is_same_v<typename xcore::function::traits<T_CALLBACK>::return_type, void>
    )
    xecs::component::entity instance::CreateEntity
    ( T_CALLBACK&& Function 
    ) noexcept
    {
        using func_traits = xcore::function::traits<T_CALLBACK>;
        return[&]< typename... T_COMPONENTS >(std::tuple<T_COMPONENTS...>*) constexpr noexcept
        {
            assert(m_ComponentBits.getBit(component::info_v<T_COMPONENTS>.m_UID) && ...);

            // Allocate the entity
            const int   EntityIndexInPool   = m_Pool.Append();
            const auto  Entity              = m_GameMgr.AllocNewEntity(EntityIndexInPool, *this);
            m_Pool.getComponent<xecs::component::entity>(EntityIndexInPool) = Entity;

            // Call the user initializer if any
            if constexpr (false == std::is_same_v<xecs::tools::empty_lambda, T_CALLBACK >)
                Function(m_Pool.getComponent<std::remove_reference_t<T_COMPONENTS>>(EntityIndexInPool) ...);

            return Entity;
        }( xcore::types::null_tuple_v<func_traits::args_tuple> );
    }

    //--------------------------------------------------------------------------------------------

    void instance::DestroyEntity
    ( xecs::component::entity& Entity 
    ) noexcept
    {
        assert( Entity.isZombie() == false );
        Entity.m_Validation.m_bZombie
        = m_GameMgr.m_lEntities[Entity.m_GlobalIndex].m_Validation.m_bZombie
        = true;
        m_ToDelete.push_back(Entity);
        if( 0 == m_ProcessReference ) UpdateStructuralChanges();
    }

    //--------------------------------------------------------------------------------------------

    void instance::UpdateStructuralChanges
    ( void
    ) noexcept
    {
        if( m_ToDelete.size() )
        {
            for( auto& Entity : m_ToDelete )
            {
                auto& EntityDetails = m_GameMgr.getEntityDetails(Entity);
                assert(EntityDetails.m_pArchetype == this);

                // Free Entity
                m_Pool.Delete(EntityDetails.m_PoolIndex);
                if(EntityDetails.m_PoolIndex != m_Pool.Size()) 
                    m_GameMgr.DeleteEntity(Entity, m_Pool.getComponent<xecs::component::entity>(EntityDetails.m_PoolIndex));
            }
            m_ToDelete.clear();
        }
    }
}
