namespace xecs::prefab
{
    template
    < typename T_ADD_TUPLE
    , typename T_SUB_TUPLE
    , typename T_CALLBACK
    > requires
    (    ( std::is_same_v< std::tuple<>, T_ADD_TUPLE> || xecs::tools::assert_valid_tuple_components_v<T_ADD_TUPLE> )
      && ( std::is_same_v< std::tuple<>, T_SUB_TUPLE> || xecs::tools::assert_valid_tuple_components_v<T_SUB_TUPLE> )
      && xecs::tools::assert_standard_function_v<T_CALLBACK>
      && xecs::tools::assert_function_return_v<T_CALLBACK, void>
    ) __inline
    bool mgr::CreatePrefabInstance( int Count, xecs::prefab::guid PrefabGuid, T_CALLBACK&& Callback ) noexcept
    {
        xecs::component::entity Entity;

        if( auto Tuple = m_PrefabList.find(PrefabGuid.m_Instance.m_Value); Tuple == m_PrefabList.end() ) return false;
        else Entity = Tuple->second;
    
        //
        // Get the right set of bits
        //
        auto& PrefabDetails   = m_GameMgr.m_ComponentMgr.getEntityDetails(Entity);
        auto& PrefabArchetype = *PrefabDetails.m_pPool->m_pArchetype;

        xecs::tools::bits InstanceBits = PrefabArchetype.getComponentBits();

        InstanceBits.clearBit( xecs::prefab::tag );
        InstanceBits.clearBit( xecs::prefab::master );
        InstanceBits.AddFromComponents<T_ADD_TUPLE>();
        InstanceBits.ClearFromComponents<T_SUB_TUPLE>();

        //
        // Get the instance archetype
        //
        auto& InstanceArchetype = m_GameMgr.m_ArchetypeMgr.getOrCreateArchetype(InstanceBits);

        //
        // If we have to deal with share components...
        //
        using fn_traits = xcore::function::traits<T_CALLBACK>;
        if( InstanceArchetype.m_nShareComponents )
        {
            // Can we choose the fast path here?
            //  - We don't have a function or if we have a function but not share components then not changes in share components will happen which means we can speed up things.
            if constexpr( std::is_same_v< T_CALLBACK, xecs::tools::empty_lambda > || std::tuple_size_v<xecs::component::type::details::share_only_tuple_t<fn_traits::args_tuple>> == 0 )
            {
                auto& Family = PrefabArchetype.m_nShareComponents ? InstanceArchetype.getOrCreatePoolFamily( *PrefabDetails.m_pPool->m_pMyFamily ) : InstanceArchetype.getOrCreatePoolFamily({},{});

                InstanceArchetype.CreateEntities( Family, Count, Entity, std::forward<T_CALLBACK&&>(Callback) );
            }
            else
            {
                // complex case
                xassert(false);
            }
        }
        else
        {
            assert( std::tuple_size_v<xecs::component::type::details::share_only_tuple_t<fn_traits::args_tuple>> == 0 );
            InstanceArchetype.CreateEntities( Count, Entity, std::forward<T_CALLBACK&&>(Callback) );
        }

        //
        // Resolve references
        //
        {
            //...
        }
        
        return false;
    }

/*
    //-------------------------------------------------------------------------------

    template
    < typename T_FUNCTION = xecs::tools::empty_lambda
    > requires
    ( xcore::function::is_callable_v<T_FUNCTION>
    ) [[nodiscard]] xecs::component::entity 
    AddOrRemoveComponents
    ( xecs::game_mgr::instance&                            GameMgr
    , xecs::component::entity                              Entity
    , const xecs::tools::bits&                             Add
    , const xecs::tools::bits&                             Sub
    , T_FUNCTION&&                                         Function
    ) noexcept
    {
        auto newEntity = GameMgr.m_ArchetypeMgr( Entity, Add, Sub, std::function<T_FUNCTION>(Function) );
        if( newEntity.isZombie() ) return newEntity;

        GameMgr.getEntity( newEntity, [&]( override_tracker* pTracker ) noexcept
        {
            if( pTracker == nullptr ) return;

            const auto  PrefabBits = GameMgr.getArchetype(pTracker->m_PrefabEntity).getComponentBits();
            const auto  EntityBits = GameMgr.getArchetype(newEntity).getComponentBits();

            // Make sure we mark all the relevant components to add/remove
            pTracker->m_DeletedComponents = xecs::tools::bits{};
            pTracker->m_NewComponents     = xecs::tools::bits{};
            for (int i = 0, end = PrefabBits.m_Bits.size() * 64; i < end; ++i)
            {
                if( EntityBits.getBit(i) )
                {
                    if( PrefabBits.getBit(i) == false ) pTracker->m_NewComponents.setBit(i);
                }
                else
                {
                    if( PrefabBits.getBit(i) ) pTracker->m_DeletedComponents.setBit(i);
                }
            }
        });

    }

    //-------------------------------------------------------------------------------

    template
    < typename T_FUNCTION
    > requires
    ( xcore::function::is_callable_v<T_FUNCTION>
    ) [[nodiscard]] xecs::component::entity 
AddOrRemoveComponents
    ( xecs::game_mgr::instance&                            GameMgr
    , xecs::component::entity                              Entity
    , std::span<const xecs::component::type::info* const>  Add
    , std::span<const xecs::component::type::info* const>  Sub
    , T_FUNCTION&&                                         Function
    ) noexcept
    {
        xecs::tools::bits AddBits;
        xecs::tools::bits SubBits;

        for (auto& e : Add) AddBits.setBit(e->m_BitID);
        for (auto& e : Sub) AddBits.setBit(e->m_BitID);

        return AddOrRemoveComponents
        ( GameMgr
        , Entity
        , AddBits
        , SubBits
        , std::forward<T_FUNCTION&&>(Function)
        );
   }

    //-------------------------------------------------------------------------------

    template
    < typename T_TUPLE_ADD
    , typename T_TUPLE_SUBTRACT = std::tuple<>
    , typename T_FUNCTION = xecs::tools::empty_lambda
    > requires
    (  xcore::function::is_callable_v<T_FUNCTION>
    && xcore::types::is_specialized_v<std::tuple, T_TUPLE_ADD>
    && xcore::types::is_specialized_v<std::tuple, T_TUPLE_SUBTRACT>
    ) [[nodiscard]] xecs::component::entity 
    AddOrRemoveComponents
    ( xecs::game_mgr::instance&     GameMgr
    , xecs::component::entity       Entity
    , T_FUNCTION&&                  Function
    ) noexcept
    {
        xecs::tools::bits AddBits;
        xecs::tools::bits SubBits;

        [&]<typename...T>(std::tuple<T...>*){ AddBits.AddFromComponents<T...>(); }( xcore::types::null_tuple_v<T_TUPLE_ADD> );
        [&]<typename...T>(std::tuple<T...>*){ SubBits.AddFromComponents<T...>(); }( xcore::types::null_tuple_v<T_TUPLE_SUBTRACT> );

        return AddOrRemoveComponents
        ( GameMgr
        , Entity
        , AddBits
        , SubBits
        , Function
        );
    }
    */
}