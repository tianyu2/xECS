namespace xecs::archetype
{
    namespace details
    {
        //-------------------------------------------------------------------------------------------------

        template< typename T_FUNCTION >
        constexpr auto function_arguments_r_refs_v = []<typename...T>(std::tuple<T...>*) constexpr
        {
            return (std::is_reference_v<T> && ...);
        }( xcore::types::null_tuple_v<xcore::function::traits<T_FUNCTION>::args_tuple> );

        //-------------------------------------------------------------------------------------------------

        template< typename... T_FUNCTION_ARGS >
        requires( ((std::is_reference_v<T_FUNCTION_ARGS>) && ... ) )
        constexpr __inline
        auto GetComponentPointerArray
        ( const xecs::pool::instance&             Pool
        , const int                               StartingPoolIndex
        , std::tuple<T_FUNCTION_ARGS... >* 
        ) noexcept
        {
            assert( ((Pool.findIndexComponentFromGUID(xecs::component::info_v<T_FUNCTION_ARGS>.m_Guid) >= 0 ) && ... ) );

            using function_args = std::tuple< T_FUNCTION_ARGS... >;
            using sorted_tuple  = xecs::component::details::sort_tuple_t< function_args >;

            std::array< std::byte*, sizeof...(T_FUNCTION_ARGS) > CachePointers;
            [&]<typename... T_SORTED_COMPONENT >( std::tuple<T_SORTED_COMPONENT...>* ) constexpr noexcept
            {
                int Sequence = 0;
                ((CachePointers[xcore::types::tuple_t2i_v< T_SORTED_COMPONENT, function_args >] = 
                  &Pool.m_pComponent[Pool.findIndexComponentFromGUIDInSequence(xecs::component::info_v<T_SORTED_COMPONENT>.m_Guid, Sequence)]
                        [sizeof(std::remove_reference_t<T_SORTED_COMPONENT>) * StartingPoolIndex])
                , ... );

            }( xcore::types::null_tuple_v<sorted_tuple>);

            return CachePointers;
        }

        //-------------------------------------------------------------------------------------------------

        template< typename... T_FUNCTION_ARGS >
        requires( !((std::is_reference_v<T_FUNCTION_ARGS>) && ...) )
        constexpr __inline
        auto GetComponentPointerArray
        ( const xecs::pool::instance&             Pool
        , const int                               StartingPoolIndex
        , std::tuple<T_FUNCTION_ARGS... >* 
        ) noexcept
        {
            static_assert(((std::is_reference_v<T_FUNCTION_ARGS> || std::is_pointer_v<T_FUNCTION_ARGS>) && ...));

            using function_args = std::tuple< T_FUNCTION_ARGS... >;
            using sorted_tuple  = xecs::component::details::sort_tuple_t< function_args >;

            std::array< std::byte*, sizeof...(T_FUNCTION_ARGS) > CachePointers;
            [&]<typename... T_SORTED_COMPONENT >( std::tuple<T_SORTED_COMPONENT...>* ) constexpr noexcept
            {
                int Sequence = 0;
                ((CachePointers[xcore::types::tuple_t2i_v< T_SORTED_COMPONENT, function_args >] = [&]<typename T>(std::tuple<T>*) constexpr noexcept 
                {
                    const auto I = Pool.findIndexComponentFromGUIDInSequence(xecs::component::info_v<T>.m_Guid, Sequence);
                    if constexpr (std::is_pointer_v<T>)
                        return (I < 0) ? nullptr : &Pool.m_pComponent[I][sizeof(std::decay_t<T>) * StartingPoolIndex];
                    else
                        return &Pool.m_pComponent[I][sizeof(std::decay_t<T>) * StartingPoolIndex];
                }(xcore::types::make_null_tuple_v<T_SORTED_COMPONENT>))
                , ... );

            }( xcore::types::null_tuple_v<sorted_tuple>);

            return CachePointers;
        }

        //-------------------------------------------------------------------------------------------------
        template< typename T_FUNCTION, typename T_ARRAY >
        requires( function_arguments_r_refs_v<T_FUNCTION>
                  && std::is_same_v< void, typename xcore::function::traits<T_FUNCTION>::return_type >
                )
        constexpr __inline
        void CallFunction( T_FUNCTION&& Function, T_ARRAY& CachePointers) noexcept
        {
            using func_traits = xcore::function::traits<T_FUNCTION>;
            [&] <typename...T>(std::tuple<T...>*) constexpr noexcept
            {
                static_assert(((std::is_reference_v<T>) && ...));
                Function(reinterpret_cast<T>(*CachePointers[xcore::types::tuple_t2i_v<T, func_traits::args_tuple>])
                    ...);
                ((CachePointers[xcore::types::tuple_t2i_v<T, func_traits::args_tuple>] += sizeof(std::remove_reference_t<T>)), ...);
            }(xcore::types::null_tuple_v<func_traits::args_tuple>);
        }

        //-------------------------------------------------------------------------------------------------
        template< typename T_FUNCTION, typename T_ARRAY >
        requires( function_arguments_r_refs_v<T_FUNCTION>
                  && std::is_same_v< bool, typename xcore::function::traits<T_FUNCTION>::return_type >
                )
        constexpr __inline
        bool CallFunction( T_FUNCTION&& Function, T_ARRAY& CachePointers) noexcept
        {
            using func_traits = xcore::function::traits<T_FUNCTION>;
            return [&] <typename...T>(std::tuple<T...>*) constexpr noexcept
            {
                static_assert(((std::is_reference_v<T>) && ...));
                if( Function(reinterpret_cast<T>(*CachePointers[xcore::types::tuple_t2i_v<T, func_traits::args_tuple>])
                    ...) ) return true;
                ((CachePointers[xcore::types::tuple_t2i_v<T, func_traits::args_tuple>] += sizeof(std::remove_reference_t<T>)), ...);
                return false;
            }(xcore::types::null_tuple_v<func_traits::args_tuple>);
        }

        //-------------------------------------------------------------------------------------------------
        template< typename T_FUNCTION, typename T_ARRAY >
        requires( xcore::function::is_callable_v<T_FUNCTION>
                  && false == function_arguments_r_refs_v<T_FUNCTION>
                  && std::is_same_v< void, typename xcore::function::traits<T_FUNCTION>::return_type >
                )
        constexpr __inline
        void CallFunction( T_FUNCTION&& Function, T_ARRAY& CachePointers) noexcept
        {
            using func_traits = xcore::function::traits<T_FUNCTION>;
            [&] <typename... T_COMPONENTS>(std::tuple<T_COMPONENTS...>*) constexpr noexcept
            {
                Function([&]<typename T>(std::tuple<T>*) constexpr noexcept -> T
                {
                    auto& MyP = CachePointers[xcore::types::tuple_t2i_v<T, typename func_traits::args_tuple>];

                    if constexpr (std::is_pointer_v<T>) if (MyP == nullptr) return reinterpret_cast<T>(nullptr);

                    auto p = MyP;                   // Back up the pointer
                    MyP += sizeof(std::decay_t<T>); // Get ready for the next entity

                    if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(p);
                    else                                return reinterpret_cast<T>(*p);
                }(xcore::types::make_null_tuple_v<T_COMPONENTS>)
                ...);
            }(xcore::types::null_tuple_v<func_traits::args_tuple>);
        }

        //-------------------------------------------------------------------------------------------------
        template< typename T_FUNCTION, typename T_ARRAY >
        requires( xcore::function::is_callable_v<T_FUNCTION>
                  && false == function_arguments_r_refs_v<T_FUNCTION>
                  && std::is_same_v< bool, typename xcore::function::traits<T_FUNCTION>::return_type >
                )
        constexpr __inline
        bool CallFunction( T_FUNCTION&& Function, T_ARRAY& CachePointers) noexcept
        {
            using func_traits = xcore::function::traits<T_FUNCTION>;
            return [&] <typename... T_COMPONENTS>(std::tuple<T_COMPONENTS...>*) constexpr noexcept
            {
                return Function([&]<typename T>(std::tuple<T>*) constexpr noexcept -> T
                {
                    auto& MyP = CachePointers[xcore::types::tuple_t2i_v<T, typename func_traits::args_tuple>];

                    if constexpr (std::is_pointer_v<T>) if (MyP == nullptr) return reinterpret_cast<T>(nullptr);

                    auto p = MyP;                   // Back up the pointer
                    MyP += sizeof(std::decay_t<T>); // Get ready for the next entity

                    if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(p);
                    else                                return reinterpret_cast<T>(*p);
                }(xcore::types::make_null_tuple_v<T_COMPONENTS>)
                ...);
            }(xcore::types::null_tuple_v<func_traits::args_tuple>);
        }
    }

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
    , const tools::bits&                             Bits 
    ) noexcept
    {
        // Deep copy the infos just in case the user gave us data driven infos
        bool AreTheySorted = true;
        for( int i=0; i<Infos.size(); ++i )
        {
            m_InfoData[i] = Infos[i];
            if( i && m_InfoData[i-1]->m_Guid.m_Value > m_InfoData[i]->m_Guid.m_Value ) AreTheySorted = false;
        }

        // Short Infos base on their GUID (smaller first) entry 0 should be the entity
        if( false == AreTheySorted )
        {
            std::sort
            ( m_InfoData.begin()
            , m_InfoData.begin() + Infos.size()
            , []( const xecs::component::info* pA, const xecs::component::info* pB ) constexpr
            {
                return pA->m_Guid < pB->m_Guid;
            });
        }

        //
        // Lets run a sanity check
        //
#ifdef _DEBUG
        {
            // First component should the the entity
            assert(m_InfoData[0] == &xecs::component::info_v<xecs::component::entity>);

            // Entity bit should be turn on
            assert(Bits.getBit(xecs::component::info_v<xecs::component::entity>.m_BitID));

            // Check all other components
            for (int i = 1; i < Infos.size(); ++i)
            {
                // There should be no duplication of components
                assert(m_InfoData[i - 1] != m_InfoData[i]);

                // Check that the bits match
                assert( Bits.getBit(m_InfoData[i]->m_BitID));
            }
        }
#endif

        //
        // We can initialize our default pool
        //
        m_Pool.Initialize( { m_InfoData.data(), Infos.size() } );
        m_ComponentBits = Bits;
        m_nComponents   = xcore::types::static_cast_safe<std::uint8_t>(Infos.size());
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
            assert(m_ComponentBits.getBit(component::info_v<T_COMPONENTS>.m_BitID) && ...);

            // Allocate the entity
            const int   EntityIndexInPool   = m_Pool.Append(1);
            const auto  Entity              = m_GameMgr.AllocNewEntity(EntityIndexInPool, *this);
            m_Pool.getComponent<xecs::component::entity>(EntityIndexInPool) = Entity;

            // Call the user initializer if any
            if constexpr (std::is_same_v<xecs::tools::empty_lambda, T_CALLBACK >)
            {
                // Update the official count if we can
                if (0 == m_ProcessReference) m_Pool.UpdateStructuralChanges(m_GameMgr);
            }
            else
            {
                AccessGuard([&]
                {
                    Function(m_Pool.getComponent<std::remove_reference_t<T_COMPONENTS>>(EntityIndexInPool) ...);
                });
            }

            return Entity;
        }( xcore::types::null_tuple_v<func_traits::args_tuple> );
    }

    //--------------------------------------------------------------------------------------------
    template
    < typename T_CALLBACK
    > requires
    ( xcore::function::is_callable_v<T_CALLBACK>
   && std::is_same_v<typename xcore::function::traits<T_CALLBACK>::return_type, void>
    )
    void instance::CreateEntities
    ( const int         Count
    , T_CALLBACK&&      Function 
    ) noexcept
    {
        using func_traits = xcore::function::traits<T_CALLBACK>;
        static_assert( []<typename...T>(std::tuple<T...>*){ return (std::is_reference_v<T> && ...); }(xcore::types::null_tuple_v<func_traits::args_tuple>)
        , "This function requires only references in the user function");
        const int EntityIndexInPool = m_Pool.Append(Count);

        // Allocate the entity
        if constexpr ( std::is_same_v<xecs::tools::empty_lambda, T_CALLBACK > )
        {
            xecs::component::entity* pEntity = &reinterpret_cast<xecs::component::entity*>(m_Pool.m_pComponent[0])[EntityIndexInPool];
            for (int i = 0; i < Count; ++i)
            {
                *pEntity = m_GameMgr.AllocNewEntity(EntityIndexInPool+i, *this);
                pEntity++;
            }

            // Update the official count if we can
            if (0 == m_ProcessReference) m_Pool.UpdateStructuralChanges(m_GameMgr);
        }
        else
        {
            auto CachePointers = details::GetComponentPointerArray( m_Pool, EntityIndexInPool
                                                                  , xcore::types::null_tuple_v<func_traits::args_tuple> 
                                                                  );

            xecs::component::entity* pEntity = &reinterpret_cast<xecs::component::entity*>(m_Pool.m_pComponent[0])[EntityIndexInPool];
            assert( &m_Pool.getComponent<xecs::component::entity>(EntityIndexInPool) == pEntity ); 

            AccessGuard([&]
            {
                for( int i=0; i<Count; ++i )
                {
                    // Fill the entity data
                    *pEntity = m_GameMgr.AllocNewEntity(EntityIndexInPool+i, *this);
                    assert(m_GameMgr.getEntityDetails(*pEntity).m_pArchetype == this );
                    assert(m_GameMgr.getEntityDetails(*pEntity).m_PoolIndex  == EntityIndexInPool + i );
                    pEntity++;

                    // Call the user initializer
                    details::CallFunction( Function, CachePointers );
                }
            });
        }
    }

    //--------------------------------------------------------------------------------------------

    void instance::DestroyEntity
    ( xecs::component::entity& Entity 
    ) noexcept
    {
        assert( Entity.isZombie() == false );

        //
        // Validate the crap out of the entity
        //
        auto& GlobalEntry = m_GameMgr.m_lEntities[Entity.m_GlobalIndex];

        if(Entity.m_Validation != GlobalEntry.m_Validation
        || GlobalEntry.m_pArchetype != this ) return;
        
        // Make sure that we are in sync with the pool entity
        auto& PoolEntity = m_Pool.getComponent<xecs::component::entity>(GlobalEntry.m_PoolIndex);
        if( PoolEntity != Entity )
            return;

        //
        // Make sure everything is marked as zombie
        //
        Entity.m_Validation.m_bZombie
            = PoolEntity.m_Validation.m_bZombie
            = GlobalEntry.m_Validation.m_bZombie
            = true;

        //
        // Delete from pool
        //
        m_Pool.Delete(GlobalEntry.m_PoolIndex);

        if (0 == m_ProcessReference) m_Pool.UpdateStructuralChanges(m_GameMgr);
    }

    //--------------------------------------------------------------------------------------------

    void instance::UpdateStructuralChanges
    ( void
    ) noexcept
    {
        m_Pool.UpdateStructuralChanges(m_GameMgr);
    }
}
