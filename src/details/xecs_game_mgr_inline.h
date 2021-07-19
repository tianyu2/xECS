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

        return ArchetypesFound;
    }

    //---------------------------------------------------------------------------

    archetype::instance& instance::getOrCreateArchetype( std::span<const component::type::info* const> Types ) noexcept
    {
        xecs::tools::bits Bits{};
        for( auto& e : Types )
            Bits.setBit( e->m_BitID );

        // Make sure we always include the entity
        Bits.setBit( xecs::component::type::info_v<xecs::component::entity>.m_BitID );

        return *m_ArchetypeMgr.getOrCreateArchetype(Bits).get();
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
    , auto     T_SHARE_AS_DATA_V
    > requires
    ( xecs::tools::assert_is_callable_v<T_FUNCTION>
        && (xecs::tools::function_return_v<T_FUNCTION, bool >
            || xecs::tools::function_return_v<T_FUNCTION, void >)
    )
    bool instance::Foreach(const std::span<xecs::archetype::instance* const> List, T_FUNCTION&& Function ) noexcept
    {
        std::conditional_t
        < T_SHARE_AS_DATA_V
        , xecs::query::iterator<T_FUNCTION, xecs::query::details::mode::DATA_ONLY>
        , xecs::query::iterator<T_FUNCTION>
        > Iterator(*this);

        for( const auto& pE : List )
        {
            Iterator.UpdateArchetype( *pE );
            for( auto pFamily = pE->getFamilyHead(); pFamily; pFamily = pFamily->m_Next.get() )
            {
                Iterator.UpdateFamilyPool( *pFamily );
                for( auto pPool = &pFamily->m_DefaultPool; pPool; pPool = pPool->m_Next.get() )
                {
                    if( 0 == pPool->Size() ) continue;
                    Iterator.UpdatePool( *pPool );

                    if constexpr (std::is_same_v<xecs::query::iterator<T_FUNCTION>::ret_t, bool >)
                    {
                        if( Iterator.ForeachEntity( std::forward<T_FUNCTION&&>(Function) ) )
                            return true;
                    }
                    else
                    {
                        Iterator.ForeachEntity( std::forward<T_FUNCTION&&>(Function));
                    }
                }
            }
        }
        return false;
    }

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