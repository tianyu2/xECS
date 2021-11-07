namespace xecs::prefab
{

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