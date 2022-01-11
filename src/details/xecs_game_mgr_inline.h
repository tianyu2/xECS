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
        Info.m_pPool->m_pArchetype->DestroyEntity(Entity);
    }


    //---------------------------------------------------------------------------

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

        return m_ArchetypeMgr.getOrCreateArchetype(Bits);
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
            return m_ArchetypeMgr.getOrCreateArchetype(Bits);

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
    template
    < typename T_FUNCTION
    , auto     T_SHARE_AS_DATA_V
    > requires
    ( xecs::tools::assert_is_callable_v<T_FUNCTION>
        && (xecs::tools::function_return_v<T_FUNCTION, bool >
            || xecs::tools::function_return_v<T_FUNCTION, void >)
    )
    bool instance::Foreach( const std::vector<const xecs::archetype::instance*>& List, T_FUNCTION&& Function ) noexcept
    {
        return Foreach( List, std::forward<T_FUNCTION>(Function) );
    }

    //---------------------------------------------------------------------------

    template< typename T_FUNCTION>
    requires
    ( xecs::tools::assert_standard_function_v<T_FUNCTION>
    ) [[nodiscard]] xecs::component::entity 
    instance::findEntity( xecs::component::entity Entity, T_FUNCTION&& Function ) noexcept
    {
        //
        // is the entity already dead?
        //
        if (Entity.isZombie()) return {};

        auto& EntityDetails = m_ComponentMgr.getEntityDetails(Entity);
        if ( Entity.m_Validation != EntityDetails.m_Validation ) return {};

        static_assert(xcore::types::is_const_v<const int&>);

        //
        // does the user want to do anything?
        //
        if constexpr (std::is_same_v< T_FUNCTION, xecs::tools::empty_lambda>) return Entity;

        //
        // Time to do some work...
        //
        else if constexpr ( xecs::tools::function_has_share_component_args_v<T_FUNCTION> )
        {
            using sorted_shares_t = xecs::component::type::details::sort_tuple_t<xecs::component::type::details::share_only_tuple_t< xcore::function::traits<T_FUNCTION>::args_tuple >>;

            if constexpr( xecs::tools::tuple_only_const_types_v< xecs::component::type::details::share_only_tuple_t< xcore::function::traits<T_FUNCTION>::args_tuple > > )
            {
                using data_t = xecs::component::type::details::data_only_tuple_t < xcore::function::traits<T_FUNCTION>::args_tuple >;

                auto ShareTuple = [&]< typename...T_ARGS >( std::tuple<T_ARGS...>* ) constexpr noexcept
                {
                    using share_ptr_tuple   = std::tuple< xcore::types::decay_full_t<T_ARGS>* ... >;
                    auto& Family            = *EntityDetails.m_pPool->m_pMyFamily;
                    int   i                 = 0;

                    return share_ptr_tuple
                    {
                        [&]<typename T>(std::tuple<T>*) constexpr noexcept 
                        {
                            using t = xcore::types::decay_full_t<T>;

                            if constexpr ( std::is_pointer_v<T> )
                            {                                
                                if( i >= Family.m_ShareInfos.size() )
                                {
                                    return static_cast<t*>(nullptr);
                                }
                                else 
                                {
                                    int n = i;
                                    while( Family.m_ShareInfos[i] != &xecs::component::type::info_v<T> )
                                    {
                                        i++;
                                        if (i >= Family.m_ShareInfos.size())
                                        {
                                            i = n;
                                            return static_cast<t*>(nullptr);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                while( Family.m_ShareInfos[i] != &xecs::component::type::info_v<T> ) i++;
                            }

                            auto Entity = Family.m_ShareDetails[i].m_Entity;
                            i++;
                            return &m_ComponentMgr.getEntityDetails(Entity).m_pPool->getComponent<t>(EntityDetails.m_PoolIndex);

                        }( xcore::types::make_null_tuple_v<T_ARGS> ) ...
                    };

                }( xcore::types::null_tuple_v<sorted_shares_t> );

                //
                // Collect all the data components
                //
                auto Pointers = xecs::archetype::details::GetDataComponentPointerArray
                ( *EntityDetails.m_pPool
                , EntityDetails.m_PoolIndex
                , xcore::types::null_tuple_v<data_t>
                );

                //
                // Call user function
                //
                [&]<typename...T_ARG>( std::tuple<T_ARG...>* ) constexpr noexcept
                {
                    Function
                    (
                        [&]<typename T>( std::tuple<T>* ) constexpr noexcept -> T
                        {
                            if constexpr ( xecs::component::type::info_v<T>.m_TypeID == xecs::component::type::id::SHARE )
                            {
                                if constexpr ( std::is_pointer_v<T> ) return reinterpret_cast<T>(  std::get<xcore::types::decay_full_t<T>* >(ShareTuple) );
                                else                                  return reinterpret_cast<T>( *std::get<xcore::types::decay_full_t<T>* >(ShareTuple) );
                            }
                            else
                            {
                                if constexpr ( std::is_pointer_v<T> ) return reinterpret_cast<T>(  Pointers[xcore::types::tuple_t2i_v<T, data_t>] ? Pointers[xcore::types::tuple_t2i_v<T, data_t>] : nullptr );
                                else                                  return reinterpret_cast<T>( *Pointers[xcore::types::tuple_t2i_v<T, data_t>] );
                            }
                        }( xcore::types::make_null_tuple_v<T_ARG> )...
                    );

                }(xcore::types::null_tuple_v< xcore::function::traits<T_FUNCTION>::args_tuple >);
            }
            else
            {
                [&] < typename...T_ARGS > (std::tuple<T_ARGS...>*)
                {
                    using shares_t        = xecs::component::type::details::sort_tuple_t<xecs::component::type::details::share_only_tuple_t<std::tuple<xcore::types::decay_full_t<T_ARGS>...>>>;
                    using share_index     = std::array< std::int8_t, std::tuple_size_v<shares_t> >;

                    [&]< typename...T_SHARE_ARGS >( std::tuple<T_SHARE_ARGS...>* )
                    {
                        constexpr auto we_have_share_pointers_v = ((std::is_pointer_v< std::tuple_element< xcore::types::tuple_t2i_v<T_SHARE_ARGS, shares_t>, sorted_shares_t > >) || ... );

                        auto&           Family      = *EntityDetails.m_pPool->m_pMyFamily;
                        int             i           = 0;
                        share_index     ShareIndex  {};
                        std::uint64_t   KeySum      = 0;

                        //
                        // Copy all the share data
                        //
                        shares_t Shares{ [&] < typename T_SHARE >(T_SHARE*) constexpr noexcept
                        {
                            if constexpr (std::is_pointer_v< std::tuple_element< xcore::types::tuple_t2i_v<T_SHARE, shares_t>, sorted_shares_t > >)
                            {
                                while (Family.m_ShareInfos[i] != &xecs::component::type::info_v<T_SHARE>) ++i;
                            }
                            else
                            {
                                int n = i;
                                while (Family.m_ShareInfos[i] != &xecs::component::type::info_v<T_SHARE>)
                                {
                                    i++;
                                    if (i == static_cast<int>(Family.m_ShareInfos.size()))
                                    {
                                        i = n;
                                        ShareIndex[xcore::types::tuple_t2i_v<T_SHARE, shares_t>] = -1;
                                        return T_SHARE{};
                                    }
                                }
                            }

                            KeySum += Family.m_ShareDetails[i].m_Key.m_Value;
                            ShareIndex[xcore::types::tuple_t2i_v<T_SHARE, shares_t>] = i;
                            auto& Details = m_ComponentMgr.getEntityDetails(Family.m_ShareDetails[i].m_Entity);

                            // get ready for the next type
                            i++;
                            return Details.m_pPool->getComponent<T_SHARE>(Details.m_PoolIndex);

                        }(static_cast<T_SHARE_ARGS*>(nullptr)) ... };

                        //
                        // get the data pointers
                        //
                        using data_t = xecs::component::type::details::sort_tuple_t<xecs::component::type::details::data_only_tuple_t < xcore::function::traits<T_FUNCTION>::args_tuple >>;
                        auto Pointers = xecs::archetype::details::GetDataComponentPointerArray( *EntityDetails.m_pPool, EntityDetails.m_PoolIndex, xcore::types::null_tuple_v<data_t> );

                        //
                        // Call the user function
                        //
                        [&]<typename...J>(std::tuple<J...>*) constexpr noexcept
                        {
                            Function
                            (
                                [&]<typename T>(std::tuple<T>*) constexpr noexcept -> T
                                {
                                    if constexpr ( xecs::component::type::info_v<T>.m_TypeID == xecs::component::type::id::SHARE )
                                    {
                                        if constexpr( std::is_pointer_v<T> )  return reinterpret_cast<T>( ShareIndex[ xcore::types::tuple_t2i_v<T, sorted_shares_t> ] == -1 ? nullptr : &std::get< xcore::types::decay_full_t<T> >(Shares) );
                                        else                                  return std::get< xcore::types::decay_full_t<T> >(Shares);
                                    }
                                    else
                                    {
                                        if constexpr ( std::is_pointer_v<T> ) return reinterpret_cast<T>(  Pointers[ xcore::types::tuple_t2i_v< T, data_t > ] );
                                        else                                  return reinterpret_cast<T>( *Pointers[ xcore::types::tuple_t2i_v< T, data_t > ] );
                                    }
                                }(xcore::types::make_null_tuple_v<J>)...
                            );

                        }(xcore::types::null_tuple_v< xcore::function::traits<T_FUNCTION>::args_tuple >);

                        //
                        // Check if we need to move our entity
                        //
                        std::uint64_t                                                                NewKeySum          = 0;
                        std::array< xecs::component::type::share::key, std::tuple_size_v<shares_t> > NewShareKeys       {};
                        std::array< std::byte*, std::tuple_size_v<shares_t> >                        PointerToShares    {};
                        i=0;
                        ( ([&] < typename T_SHARE >(T_SHARE*) constexpr noexcept
                        {
                            if constexpr (std::is_pointer_v< std::tuple_element< xcore::types::tuple_t2i_v<T_SHARE, shares_t>, sorted_shares_t > >)
                            {
                                if( ShareIndex[xcore::types::tuple_t2i_v<T_SHARE, shares_t>] == -1) return;
                            }

                            PointerToShares[i] = reinterpret_cast<std::byte*>(&std::get<T_SHARE>(Shares));
                            NewShareKeys[i]    = xecs::component::type::details::ComputeShareKey(EntityDetails.m_pPool->m_pArchetype->getGuid(), xecs::component::type::info_v<T_SHARE>, PointerToShares[i] );                        
                            NewKeySum         += NewShareKeys[i].m_Value;
                            i++;

                        }(static_cast<T_SHARE_ARGS*>(nullptr)) ), ... );

                        if( NewKeySum != KeySum )
                        {
                            xecs::tools::bits UpdatedComponentsBits;
                            UpdatedComponentsBits.AddFromComponents< xcore::types::decay_full_t<T_SHARE_ARGS>... >();

                            //
                            // Get the new family and move the entity there
                            //
                            EntityDetails.m_pPool->m_pArchetype->getOrCreatePoolFamilyFromSameArchetype
                            ( *EntityDetails.m_pPool->m_pMyFamily
                            , UpdatedComponentsBits
                            , { PointerToShares.data(), static_cast<std::size_t>(i) }
                            , { NewShareKeys.data(), static_cast<std::size_t>(i) }
                            )
                            .MoveIn
                            ( *this
                            , *EntityDetails.m_pPool->m_pMyFamily
                            , *EntityDetails.m_pPool
                            , EntityDetails.m_PoolIndex
                            );
                        }

                    }( xcore::types::null_tuple_v<shares_t> );

                }(xcore::types::null_tuple_v< xcore::function::traits<T_FUNCTION>::args_tuple >);
            }
        }
        else
        {
            auto Pointers = xecs::archetype::details::GetDataComponentPointerArray
            ( *EntityDetails.m_pPool
            , EntityDetails.m_PoolIndex
            , xcore::types::null_tuple_v<xcore::function::traits<T_FUNCTION>::args_tuple> 
            );
            xecs::archetype::details::CallFunction( std::forward<T_FUNCTION>(Function), Pointers );
        }

        return Entity;
    }

    //---------------------------------------------------------------------------

    [[nodiscard]] xecs::archetype::instance& instance::getArchetype( xecs::component::entity Entity ) const noexcept
    {
        assert(Entity.isZombie() == false);
        auto& Entry = m_ComponentMgr.m_lEntities[Entity.m_GlobalIndex];
        assert( Entity.m_Validation == Entry.m_Validation );
        return *Entry.m_pPool->m_pArchetype;
    }

    //---------------------------------------------------------------------------

    template< typename T_FUNCTION>
    requires( xecs::tools::assert_standard_function_v<T_FUNCTION> )
    [[nodiscard]] xecs::component::entity instance::getEntity( xecs::component::entity Entity, T_FUNCTION&& Function ) noexcept
    {
        auto b = findEntity( Entity, std::forward<T_FUNCTION&&>(Function) );
        assert( b.isValid() );
        return b;
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

    //---------------------------------------------------------------------------
    namespace details::create_prefab_instance
    {
        template< typename T >
        struct filter;

        template< typename...T >
        struct filter< std::tuple<T...> >
        {
            // If we are going to have to modify shares we must copy all the components before hand
            // Because we must know how the user modify the shares so that we can know which family we need to put the entity in
            static constexpr bool has_modify_shares_v   = sizeof...(T) > 0 && (((xecs::component::type::info_v<T>.m_TypeID == xecs::component::type::id::SHARE && std::is_const_v<T> == false) || ...));
            using                 func_copy_tuple_t     = xcore::types::tuple_cat_t< std::conditional_t< has_modify_shares_v, std::tuple< xcore::types::decay_full_t<T> >, std::tuple<> > ... >;


            using                   func_tuple_t        = std::tuple<T...>;
            using                         data_t        = xecs::component::type::details::data_only_tuple_t<std::tuple<std::remove_reference_t<T>...>>;
            using                 shorted_data_t        = xecs::component::type::details::sort_tuple_t<data_t>;
            using                       shares_t        = xecs::component::type::details::share_only_tuple_t<std::tuple<std::remove_reference_t<T>...>>;
            using               shorted_shares_t        = xecs::component::type::details::sort_tuple_t<shares_t>;
            using              writable_shares_t        = xcore::types::tuple_cat_t< std::conditional_t<xecs::component::type::info_v<T>.m_TypeID == xecs::component::type::id::SHARE && std::is_const_v<T> == false, std::tuple<xcore::types::decay_full_t<T>>, std::tuple<>> ... >;
        };
    }

    template
    < bool     T_IS_VARIANT
    , typename T_FUNCTION
    > requires
    ( xecs::tools::assert_standard_function_v<T_FUNCTION>
    ) [[nodiscard]] xecs::component::entity             
    instance::CreatePrefabInstance( xecs::component::entity PrefabEntity, T_FUNCTION&& Function ) noexcept
    {
        using   info_t = details::create_prefab_instance::filter< xcore::function::traits<T_FUNCTION>::args_tuple >;

        typename info_t::func_copy_tuple_t   FuncCopyTuple        {};
        xecs::tools::bits                    FuncCopyNullComponent{};

        // Handle the case where we need to modify shares.
        // This case is special since in order to know which family the new entity is going to be place
        // we must first let the user modify the data
        if constexpr ( info_t::has_modify_shares_v )
        {
            [&] < typename...T_ARGS >( std::tuple<T_ARGS...>* ) constexpr noexcept
            {
                (void)findEntity( PrefabEntity, [&]( const T_ARGS... Args ) constexpr noexcept
                {
                    // Backup the data first then call the user function to do what he must
                    Function
                    ( 
                        ([&]<typename T>(std::tuple<T>*) constexpr noexcept -> T
                        {
                            if constexpr ( std::is_pointer_v<T> )
                            {
                                if( Args )  
                                {
                                    std::get< xcore::types::decay_full_t<T> >(FuncCopyTuple) = *Args;
                                    return &std::get< xcore::types::decay_full_t<T> >(FuncCopyTuple);
                                }
                                else 
                                {
                                    FuncCopyNullComponent.setBit(xecs::component::type::info_v<T>.m_BitID);
                                    return static_cast<T>(nullptr);
                                }
                            }
                            else
                            {
                                std::get< xcore::types::decay_full_t<T> >(FuncCopyTuple) = Args;
                                return std::get< xcore::types::decay_full_t<T> >(FuncCopyTuple);
                            }
                        }( xcore::types::make_null_tuple_v<T_ARGS> )) ... 
                    );
                });

            }( xcore::types::null_tuple_v< info_t::func_tuple_t > );
        }

        auto& PrefabArchetype = getArchetype(PrefabEntity);
        auto  Bits            = PrefabArchetype.getComponentBits();

        // Lets make sure that this is in fact a prefab
        xassert( Bits.getBit(xecs::component::type::info_v<xecs::prefab::tag>.m_BitID) );

        // Lets convert the bits to prefab instance
        // TODO: Note the syntax is stupid here because visual studio is crashing other wise...
        if constexpr ( T_IS_VARIANT == false ) Bits.clearBit( []{ return xecs::component::type::info_v<xecs::prefab::tag>.m_BitID; }() );

        //TODO: This is only needed when we are dealing with editors
        //TODO: Note the syntax is stupid here because visual studio is crashing other wise...
        Bits.setBit([]{ return xecs::component::type::info_v<xecs::prefab::instance>.m_BitID; }() );

        // Lets get the archetype of the prefab instance
        auto& InstanceArchetype = m_ArchetypeMgr.getOrCreateArchetype(Bits);
        auto& PrefabDetails     = m_ComponentMgr.getEntityDetails(PrefabEntity);

        //
        // Create the prefab instance entity
        //
        xecs::component::entity InstanceEntity = ( PrefabArchetype.m_nShareComponents == 0u )
        ? InstanceArchetype.CreateEntity()
        : [&]() constexpr noexcept -> xecs::component::entity
        {
            if constexpr ( info_t::has_modify_shares_v )
            {
                auto&   PrefabFamily    = *PrefabDetails.m_pPool->m_pMyFamily;
                auto    SharePointers   = std::array< std::byte*, xecs::settings::max_share_components_per_entity_v >{};

                // Collect all the share pointers from the tuple where we backed up the components previously
                [&]<typename...T_SHARES>(std::tuple<T_SHARES...>*) constexpr noexcept
                {
                    int i=0;
                    (([&]<typename T>( std::tuple<T>* ) constexpr noexcept
                    {
                        while( PrefabFamily.m_ShareInfos[i]->m_BitID < xecs::component::type::info_v<T>.m_BitID )
                        {
                            // If we are missing a component then just grab it directly from the share
                            // since we will just use it to copy it
                            auto& Details = m_ComponentMgr.getEntityDetails(PrefabFamily.m_ShareDetails[i].m_Entity);
                            auto& Pool    = *Details.m_pPool;
                            for( int j = 1, end_j = static_cast<int>(Pool.m_ComponentInfos.size()); j != end_j; ++j )
                            {
                                if( Pool.m_ComponentInfos[j] != PrefabFamily.m_ShareInfos[i] )
                                {
                                    SharePointers[i] = &Pool.m_pComponent[j][Details.m_PoolIndex.m_Value * PrefabFamily.m_ShareInfos[i]->m_Size];
                                    break;
                                }
                            }

                            // Go to the next share component
                            i++;
                        }

                        if constexpr ( std::is_pointer_v<T> )
                        {
                            if( PrefabFamily.m_ShareInfos[i] != &xecs::component::type::info_v<T> )
                                return;
                        }
                        else
                        {
                            xassert( PrefabFamily.m_ShareInfos[i] == &xecs::component::type::info_v<T> );
                        }

                        SharePointers[i++] = reinterpret_cast<std::byte*>(&std::get< xcore::types::decay_full_t<T> >(FuncCopyTuple));

                    }( xcore::types::make_null_tuple_v<T_SHARES> )), ... );

                }( xcore::types::null_tuple_v<info_t::shorted_shares_t> );

                // Find the prefab instance family
                auto& Family = InstanceArchetype.getOrCreatePoolFamily
                ( PrefabFamily.m_ShareInfos
                , { SharePointers.data(), PrefabFamily.m_ShareInfos.size() }
                );

                // Create the instance (Not need to call the user function since we already did that)
                return InstanceArchetype.CreateEntity( Family, xecs::tools::empty_lambda{});
            }
            else
            {
                auto& Family = InstanceArchetype.getOrCreatePoolFamily(*PrefabDetails.m_pPool->m_pMyFamily);

                // No need to call the user function we will do that later after we collect all the data
                return InstanceArchetype.CreateEntity(Family, xecs::tools::empty_lambda{});
            }
        }();

        //
        // Copy the data from the original Prefab
        //
        auto& InstanceDetails       = m_ComponentMgr.getEntityDetails(InstanceEntity);
        auto& PrefabPool            = *PrefabDetails.m_pPool;
        auto& InstancePool          = *InstanceDetails.m_pPool;
        auto  DataForModifyShares   = [&]<typename...T_DATAS>(std::tuple<T_DATAS...>*) constexpr noexcept
        {
            if constexpr ( info_t::has_modify_shares_v )
            {
                int  i    = 0;
                auto Data = std::array< std::byte*, xecs::settings::max_data_components_per_entity_v >{};

                (([&]<typename T>( std::tuple<T>* ) constexpr noexcept
                {
                    while( PrefabPool.m_ComponentInfos[i]->m_BitID < xecs::component::type::info_v<T>.m_BitID )
                    {
                        Data[i++] = nullptr;
                    }

                    if constexpr (std::is_pointer_v<T>)
                    {
                        if (PrefabPool.m_ComponentInfos[i] != &xecs::component::type::info_v<T> )
                            return;
                    }
                    else
                    {
                        xassert(PrefabPool.m_ComponentInfos[i] == &xecs::component::type::info_v<T> );
                    }

                    Data[i++] = reinterpret_cast<std::byte*>( &std::get< xcore::types::decay_full_t<T> >(FuncCopyTuple) );

                }( xcore::types::make_null_tuple_v<T_DATAS> )), ... );

                return Data;
            }
            else
            {
                return 0;
            }
            
        }( xcore::types::null_tuple_v<info_t::shorted_data_t> );

        // Copy all the data
        // k is a helper to ignore the xecs::prefab::override_tracker component only used for the editor...
        for( int i=1, k = 0, end = static_cast<int>(PrefabPool.m_ComponentInfos.size()); i<end; ++i )
        {
            auto& TypeInfo = *PrefabPool.m_ComponentInfos[i];

            //TODO: This is only needed when we are dealing with editors
            if( TypeInfo.m_BitID != InstancePool.m_ComponentInfos[i+k]->m_BitID )
            {
                xassert(k==0);
                xassert( InstancePool.m_ComponentInfos[i+k]->m_BitID == xecs::component::type::info_v<xecs::prefab::instance>.m_BitID );
                k++;
            }

            xassert(InstancePool.m_ComponentInfos[i+k]->m_BitID == TypeInfo.m_BitID);
            if constexpr ( info_t::has_modify_shares_v )
            {
                if( DataForModifyShares[i] )
                {
                    if( TypeInfo.m_pMoveFn )
                        TypeInfo.m_pMoveFn( &InstancePool.m_pComponent[i + k][InstanceDetails.m_PoolIndex.m_Value * TypeInfo.m_Size], DataForModifyShares[ i ] );
                    else
                        std::memcpy( &InstancePool.m_pComponent[i+k][InstanceDetails.m_PoolIndex.m_Value * TypeInfo.m_Size]
                           , DataForModifyShares[i]
                           , TypeInfo.m_Size 
                           );

                    continue;
                }
            }

            if( TypeInfo.m_pCopyFn )
            {
                TypeInfo.m_pCopyFn( &InstancePool.m_pComponent[i+k][InstanceDetails.m_PoolIndex.m_Value * TypeInfo.m_Size]
                                  , &PrefabPool  .m_pComponent[i]  [PrefabDetails.m_PoolIndex.m_Value   * TypeInfo.m_Size]
                                  );
            }
            else
            {
                std::memcpy( &InstancePool.m_pComponent[i+k][InstanceDetails.m_PoolIndex.m_Value * TypeInfo.m_Size]
                           , &PrefabPool  .m_pComponent[i]  [PrefabDetails.m_PoolIndex.m_Value   * TypeInfo.m_Size]
                           , TypeInfo.m_Size 
                           );
            }
        }

        // Let the user do what he must
        if constexpr ( info_t::has_modify_shares_v )
        {
            // Nothing to do
        }
        else if constexpr ( std::tuple_size_v<info_t::shares_t> )
        {
            auto Pointers = xecs::archetype::details::GetDataComponentPointerArray( *InstanceDetails.m_pPool, InstanceDetails.m_PoolIndex, xcore::types::null_tuple_v<info_t::data_t> );

            [&]<typename...T_SHARE>()
            {
                auto& PrefabFamily = *PrefabDetails.m_pPool->m_pMyFamily;
                std::array< std::byte*, std::tuple_size_v<info_t::shares_t> > ShareDatas{};

                // Copy the share pointer 
                {
                    int i = 0;
                    int k = 0;
                    (([&]<typename T>(T*) constexpr noexcept
                    {
                        while (PrefabFamily.m_ShareInfos[i] != &xecs::component::type::info_v<T>) ++i;
                        ShareDatas[i++] = reinterpret_cast<std::byte*>(&std::get<xcore::types::decay_full_t<T>>(FuncCopyTuple));
                    }(static_cast<std::remove_reference_t<T_SHARE>*>(nullptr))), ...);
                }

                // Call the function
                [&]<typename...T_ARG>(std::tuple<T_ARG...>*)constexpr noexcept
                {
                    Function
                    (
                        [&]< typename J >( std::tuple<J>* ) constexpr noexcept -> J
                        {
                            if constexpr( xecs::component::type::info_v<J>.m._TypeID == xecs::component::type::id::SHARE )
                            {
                                if constexpr (std::is_pointer_v<J>) return reinterpret_cast<J>( ShareDatas[ xcore::types::tuple_t2i_v<xcore::types::decay_full_t<J>, info_t::shorted_shares_t> ] );
                                else                                return reinterpret_cast<J>(*ShareDatas[ xcore::types::tuple_t2i_v<xcore::types::decay_full_t<J>, info_t::shorted_shares_t> ] );
                            }
                            else
                            {
                                if constexpr (std::is_pointer_v<J>) return reinterpret_cast<J>( Pointers[ xcore::types::tuple_t2i_v<xcore::types::decay_full_t<J>, info_t::data_t> ] );
                                else                                return reinterpret_cast<J>(*Pointers[ xcore::types::tuple_t2i_v<xcore::types::decay_full_t<J>, info_t::data_t> ] );
                            }
                            
                        }( xcore::types::make_null_tuple_v<T_ARG> ) ...
                    );

                }(xcore::types::null_tuple_v< info_t::func_tuple_t >);

            }( xcore::types::null_tuple_v<info_t::shorted_shares_t> );
        }
        else if constexpr ( !std::is_same_v< T_FUNCTION, xecs::tools::empty_lambda> )
        {
            auto Pointers = xecs::archetype::details::GetDataComponentPointerArray( *InstanceDetails.m_pPool, InstanceDetails.m_PoolIndex, xcore::types::null_tuple_v<xcore::function::traits<T_FUNCTION>::args_tuple> );
            xecs::archetype::details::CallFunction(std::forward<T_FUNCTION>(Function), Pointers);
        }

        // TODO: This is needed Only because the editor
        // Update the prefab tracker
        (void)getEntity( InstanceEntity, [&]( xecs::prefab::instance& Tracker ) constexpr noexcept
        {
            Tracker.m_PrefabEntity = PrefabEntity;
        });

        return InstanceEntity;
    }

    //---------------------------------------------------------------------------

    template< typename T_FUNCTION > requires
    ( xecs::tools::assert_standard_function_v<T_FUNCTION> ) 
    [[nodiscard]] xecs::component::entity             
    instance::CreatePrefabVariant   ( xecs::component::entity   PrefabEntity
                                    , T_FUNCTION&&              Function
                                    ) noexcept
    {
        if constexpr ( std::is_same_v<T_FUNCTION, xecs::tools::empty_lambda> ) return CreatePrefabInstance<true>( PrefabEntity, std::forward<T_FUNCTION&&>(Function) );
        else
        {
            auto  Entity = CreatePrefabInstance<true>(PrefabEntity, std::forward<T_FUNCTION&&>(Function));

            (void)getEntity(Entity, [&]( xecs::prefab::instance& PrefabInstance ) constexpr noexcept
            {
            /*
                // Mark all the components that we are setting as fully overwritten 
                for( const auto& pInfo : xecs::component::type::details::sorted_info_array_v<xcore::function::traits<T_FUNCTION>::args_tuple> )
                {
                    PrefabInstance.m_lComponents.emplace_back
                    (
                        xecs::prefab::instance::component
                        { .m_ComponentTypeGuid = pInfo->m_Guid
                        , .m_Type              = xecs::prefab::instance::component::type::NEW
                        }
                    );
                }
                */
            });

            return Entity;
        }
    }
}