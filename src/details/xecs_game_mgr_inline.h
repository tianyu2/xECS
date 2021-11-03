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

    template< typename T_FUNCTION>
    requires
    ( xecs::tools::assert_standard_function_v<T_FUNCTION>
        && (false == xecs::tools::function_has_share_component_args_v<T_FUNCTION> ) 
    )
    bool instance::findEntity( xecs::component::entity Entity, T_FUNCTION&& Function ) noexcept
    {
        if( Entity.isZombie() ) return false;
        auto& Entry = m_ComponentMgr.m_lEntities[Entity.m_GlobalIndex];
        if( Entry.m_Validation == Entity.m_Validation )
        {
            if constexpr ( !std::is_same_v< T_FUNCTION, xecs::tools::empty_lambda> )
            {
                auto Pointers = xecs::archetype::details::GetDataComponentPointerArray
                ( *Entry.m_pPool, Entry.m_PoolIndex, xcore::types::null_tuple_v<xcore::function::traits<T_FUNCTION>::args_tuple>);
                xecs::archetype::details::CallFunction( std::forward<T_FUNCTION>(Function), Pointers );
            }
            return true;
        }
        return false;
    }

    //---------------------------------------------------------------------------

    [[nodiscard]] xecs::archetype::instance& instance::getArchetype( xecs::component::entity Entity ) const noexcept
    {
        assert(Entity.isZombie() == false);
        auto& Entry = m_ComponentMgr.m_lEntities[Entity.m_GlobalIndex];
        assert( Entity.m_Validation == Entry.m_Validation );
        return *Entry.m_pArchetype;
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

    //---------------------------------------------------------------------------

    xcore::err instance::SerializeGameState
    ( const char* pFileName
    , bool        isRead
    , bool        isBinary
    ) noexcept
    {
        // visual studio crashes when-ever I make this into a constant expression
        constexpr static auto textfile_property_types_v = []() constexpr
        {
            std::array<xcore::textfile::user_defined_types, std::variant_size_v<property::settings::data_variant> > List{};

                                                                                                           // prop_types   textfile types
                                                                                                           // ------------ -------------
            List[xcore::types::variant_t2i_v<int,                       property::settings::data_variant>] = { "d",         "d"  };
            List[xcore::types::variant_t2i_v<bool,                      property::settings::data_variant>] = { "b",         "c"  };
            List[xcore::types::variant_t2i_v<float,                     property::settings::data_variant>] = { "f",         "f"  };
            List[xcore::types::variant_t2i_v<string_t,                  property::settings::data_variant>] = { "s",         "s"  };
            List[xcore::types::variant_t2i_v<xcore::math::vector2,      property::settings::data_variant>] = { "v2",        "ff" };
            List[xcore::types::variant_t2i_v<xecs::component::entity,   property::settings::data_variant>] = { "entity",    "G"  };
            List[xcore::types::variant_t2i_v<std::int16_t,              property::settings::data_variant>] = { "C",         "C"  };

            static_assert( std::variant_size_v<property::settings::data_variant> == 7, "You need to change code inside this function ");
            return List;
        }();

        std::array<xecs::component::entity,             xecs::settings::max_share_components_per_entity_v> ShareEntities{};
        std::array<xecs::component::type::share::key,   xecs::settings::max_share_components_per_entity_v> ShareKeys    {};

        xcore::textfile::stream     TextFile;
        xcore::err                  Error;

        //
        // Have the file system know the mapping of the property types
        //
        TextFile.AddUserTypes( textfile_property_types_v );
         
        //
        // Make sure that we are all up to date
        //
        if(isRead == false) m_ArchetypeMgr.UpdateStructuralChanges();

        //
        // Open file for writing
        //
        if(Error = TextFile.Open
        ( isRead
        , std::string_view{ pFileName }
        , isBinary ? xcore::textfile::file_type::BINARY : xcore::textfile::file_type::TEXT
        , {xcore::textfile::flags::WRITE_FLOATS}
        ); Error ) return Error;

        //
        // Serialize some basic info
        //
        int ArchetypeCount = isRead ? 0 : static_cast<int>(m_ArchetypeMgr.m_lArchetype.size());
        if (TextFile.Record(Error, "GameMgr"
            , [&](std::size_t i, xcore::err& Error) noexcept
            {
                Error = TextFile.Field("nArchetypes", ArchetypeCount);
            }
        )) return Error;

        //
        // Write Global Guids
        //
        if( TextFile.Record( Error, "GlobalEntities"
        ,   [&]( std::size_t& C, xcore::err& ) noexcept
            {
                if( false == isRead )
                {
                    // We will save at least one entry even if the validation is zero
                    // because we don't want to deal with the case where there is block does not save
                    C = 1;

                    //
                    // Determine the max index we want to write
                    //
                    auto Span = std::span{ m_ComponentMgr.m_lEntities.get(), xecs::settings::max_entities_v };
                    for (auto It = Span.rbegin(); It != Span.rend(); ++It)
                    {
                        auto& E = *It;
                        if (E.m_Validation.m_Value)
                        {
                            C = 1 + static_cast<int>(static_cast<std::size_t>(&E - m_ComponentMgr.m_lEntities.get()));
                            break;
                        }
                    }
                }
            }
        ,   [&]( std::size_t i, xcore::err& Error )
            {
                Error = TextFile.Field("Validation", m_ComponentMgr.m_lEntities[i].m_Validation.m_Value );
            }
        )) return Error;

        //
        // Serialize all the archetypes
        //
        for( int iArchetype=0; iArchetype < ArchetypeCount; ++iArchetype)
        {
            enum serialized_mode : std::int8_t
            { MODE_NONE
            , MODE_SERIALIZER
            , MODE_PROPERTY            
            };

            //
            // Archetype main header
            //
            xecs::component::entity::info_array Infos           {};
            int                                 InfoCount       {};
            int                                 nDataTypes      {};
            int                                 nShareTypes     {};
            int                                 nTagTypes       {};
            int                                 nFamilies       {};
            xecs::archetype::guid               ArchetypeGuid   {};
            std::array<std::int8_t, xecs::settings::max_components_per_entity_v> SerializedModes {};

            if( false == isRead )
            {
                auto&               Archetype   = m_ArchetypeMgr.m_lArchetype[iArchetype];
                xecs::tools::bits   TagBits     = xecs::tools::bits{}.setupAnd(Archetype->m_ComponentBits, xecs::component::mgr::s_TagsBits);

                nDataTypes      = Archetype->m_nDataComponents;
                nShareTypes     = Archetype->m_nShareComponents;
                nTagTypes       = TagBits.CountComponents();
                InfoCount       = Archetype->m_ComponentBits.ToInfoArray(Infos);
                nFamilies       = 0;
                ArchetypeGuid   = Archetype->m_Guid;

                // Save only families which have entites
                for( auto pF = Archetype->getFamilyHead(); pF; pF = pF->m_Next.get() )
                {
                    nFamilies++;
                }

                //
                // Write comments to help the user read the file
                //
                if ((Error = TextFile.WriteComment(" TypeInfo Details: "))) return Error;
                for(int i=0; i< InfoCount; i++ )
                {
                    if( (Error = TextFile.WriteComment( xcore::string::Fmt( "   Guid:%.16llX   Type:%s   Name:%s"
                        , Infos[i]->m_Guid.m_Value
                        , Infos[i]->m_TypeID == xecs::component::type::id::DATA 
                            ? "Data"
                            : Infos[i]->m_TypeID == xecs::component::type::id::SHARE
                            ? "Share"
                            : "Tag"
                        , Infos[i]->m_pName ).getView()))) return Error;
                }
            }

            //
            // Save the basic archetype info
            //
            if( TextFile.Record( Error, "Archetype"
                ,   [&]( std::size_t, xcore::err& Error ) noexcept
                    {
                            (Error = TextFile.Field("Guid",         ArchetypeGuid.m_Value))
                        ||  (Error = TextFile.Field("nFamilies",    nFamilies))
                        ||  (Error = TextFile.Field("nDataTypes",   nDataTypes))
                        ||  (Error = TextFile.Field("nShareTypes",  nShareTypes))
                        ||  (Error = TextFile.Field("nTagTypes",    nTagTypes));
                    }
                )) return Error;

            //
            // Read the archetype types
            //
            if( TextFile.Record( Error, "ArchetypeTypes"
                ,   [&]( std::size_t& C, xcore::err& ) noexcept
                    {
                        if( isRead ) InfoCount  = static_cast<int>(C); 
                        else         C          = InfoCount;
                    }
                ,   [&]( std::size_t i, xcore::err& Error ) noexcept
                    {
                        xecs::component::type::guid Guid
                            = isRead
                            ? xecs::component::type::guid{}
                            : Infos[i]->m_Guid;
                        
                        if( (Error = TextFile.Field( "TypeGuid", Guid.m_Value )) ) return;

                        if( isRead )
                        {
                            auto pInfo = m_ComponentMgr.findComponentTypeInfo(Guid);
                            if( pInfo == nullptr )
                            {
                                // TODO: Change this to a warning?
                                Error = xerr_failure_s("Serialization Error, Fail to find one of the components types");
                                return;
                            }

                            // Set the info into the structure this includes tags
                            Infos[i] = pInfo;
                        }

                        // We need to know how this type was serialized when loading. This is only relevant when
                        // we saved with properties and then we add the serializing function, for the loader to do the
                        // right thing it should used the properties loader since this is how it was saved with.
                        if( isRead == false )
                        {
                            if( Infos[i]->m_pSerilizeFn         ) SerializedModes[i] = serialized_mode::MODE_SERIALIZER;
                            else if (Infos[i]->m_pPropertyTable ) SerializedModes[i] = serialized_mode::MODE_PROPERTY;
                            else                                  SerializedModes[i] = serialized_mode::MODE_NONE;
                        }
                        TextFile.Field("SerializationMode", SerializedModes[i]).clear();
                    }
                )) return Error;

            //
            // Get or create the actual archetype
            //
            xecs::archetype::instance* pArchetype
                = isRead
                ? &getOrCreateArchetype({ Infos.data(), static_cast<std::size_t>(InfoCount) })
                : m_ArchetypeMgr.m_lArchetype[iArchetype].get();

            //
            // Serialize all families
            //
            xecs::pool::family* pF = isRead ? nullptr : pArchetype->getFamilyHead();
            for( int iFamily = 0; iFamily != nFamilies; ++iFamily, pF = pF->m_Next.get() )
            {
                int                         nPools      = 0;
                int                         nEntities   = 0;

                //
                // Count how many pools we have
                //
                if( false == isRead )
                {
                    for( auto pP = &pF->m_DefaultPool; pP; pP = pP->m_Next.get() )
                    {
                        nEntities += pP->Size();
                        nPools++;
                    }
                }

                //
                // Deal with the families
                //
                xecs::pool::family::guid    FamilyGuid = isRead ? xecs::pool::family::guid{} : pF->m_Guid;

                if( TextFile.Record( Error, "Family"
                ,   [&]( std::size_t, xcore::err& Error ) noexcept
                    {
                          (Error = TextFile.Field("Guid",       FamilyGuid.m_Value ))
                        ||(Error = TextFile.Field("nPools",     nPools))
                        ||(Error = TextFile.Field("nEntities",  nEntities));
                    }
                )) return Error;

                if( nShareTypes && TextFile.Record( Error, "FamilyDetails"
                ,   [&]( std::size_t& C, xcore::err& ) noexcept
                    {
                        if( false == isRead ) C = pF->m_ShareInfos.size();
                        else                  assert( C == nShareTypes );
                    }
                ,   [&]( std::size_t i, xcore::err& Error ) noexcept
                    {
                        if( isRead )
                        {
                              (Error = TextFile.Field("Entity",     ShareEntities[i].m_Value ))
                            ||(Error = TextFile.Field("ShareKey",   ShareKeys[i].m_Value));
                        }
                        else
                        {
                              (Error = TextFile.Field("Entity",     pF->m_ShareDetails[i].m_Entity.m_Value ))
                            ||(Error = TextFile.Field("ShareKey",   pF->m_ShareDetails[i].m_Key.m_Value));
                        }
                    }
                )) return Error;

                // Create a family in case of reading
                if(isRead)
                {
                    // TODO: Make sure the order of families match the saved order in memory
                    //      To do that we need to fix the link list of pending families
                    pF = &pArchetype->CreateNewPoolFamily
                    ( FamilyGuid
                    , std::span{ ShareEntities.data(),  static_cast<std::size_t>(nShareTypes) }
                    , std::span{ ShareKeys.data(),      static_cast<std::size_t>(nShareTypes) }
                    );

                    //
                    // If the Family has shares then we must insert the shares into the hash map
                    //
                    if(nShareTypes)
                    {
                        for (int i = 0; i < nShareTypes; i++)
                        {
                            auto& Details = pF->m_ShareDetails[i];
                            m_ArchetypeMgr.m_ShareComponentEntityMap.emplace( std::pair{ Details.m_Key, Details.m_Entity });
                        }
                    }
                }

                //
                // Serialize Pools
                //
                auto pP = &pF->m_DefaultPool;
                for( int iPool =0; iPool < nPools; ++iPool, pP = pP->m_Next.get() )
                {
                    int     nEntitiesInPool = isRead ? 0 : pP->Size();

                    if( TextFile.Record( Error, "PoolInfo"
                    ,   [&]( std::size_t i, xcore::err& Error ) noexcept
                        {
                            Error = TextFile.Field("nEntities", nEntitiesInPool);
                        }
                    )) return Error;

                    // Do we have any entities that we need to deal with?
                    if( nEntitiesInPool == 0 )
                    {
                        //
                        // If we need to add another pool lets do so
                        //
                        if( isRead )
                        {
                            if ((iPool + 1) != nPools)
                            {
                                pP->m_Next = std::make_unique<pool::instance>();
                                pP->m_Next->Initialize(pF->m_DefaultPool.m_ComponentInfos, *pF);
                            }
                        }

                        // Go next pool
                        continue;
                    }

                    //
                    // Serialize Entities
                    //
                    if( isRead )
                    {
                        pP->Append( nEntitiesInPool );

                        for( auto iType = 0, end = (int)pP->m_ComponentInfos.size(); iType != end; iType++ )
                        {
                            if( SerializedModes[iType] == serialized_mode::MODE_SERIALIZER && pP->m_ComponentInfos[iType]->m_pSerilizeFn)
                            {
                                int Count = 0;
                                Error = pP->m_ComponentInfos[iType]->m_pSerilizeFn(TextFile, isRead, pP->m_pComponent[iType], Count);
                                if (Error) return Error;
                                assert(Count == nEntitiesInPool);
                            }
                            else if( SerializedModes[iType] == serialized_mode::MODE_PROPERTY && pP->m_ComponentInfos[iType]->m_pPropertyTable )
                            {
                                int             Count    = nEntitiesInPool;
                                std::byte*      pData    = pP->m_pComponent[iType];
                                auto            TypeSize = pP->m_ComponentInfos[iType]->m_Size;
                                auto&           Table    = *pP->m_ComponentInfos[iType]->m_pPropertyTable;
                                property::entry PropEntry;
                                xcore::crc<32>  CRCType;

                                for( int i = 0; i < Count; ++i )
                                {
                                    if( TextFile.Record
                                    (   Error
                                        , "Props"
                                        , [&](std::size_t& C, xcore::err&) noexcept
                                        {
                                            // No need to care about how many properties we need to read...
                                        }
                                        , [&](std::size_t, xcore::err& Err) noexcept
                                        {
                                            if( Err = TextFile.Field( "Name",     PropEntry.first ); Err ) return;

                                            TextFile.ReadFieldUserType( CRCType, "Data:?" ).clear();
                                            switch(CRCType.m_Value)
                                            {
                                                case textfile_property_types_v[xcore::types::variant_t2i_v<int,             property::settings::data_variant>].m_CRC.m_Value:
                                                    if( Err = TextFile.Field("Data:?", PropEntry.second.emplace<int>()     ); Err) return; 
                                                    break;
                                                case textfile_property_types_v[xcore::types::variant_t2i_v<float,           property::settings::data_variant>].m_CRC.m_Value:
                                                    if( Err = TextFile.Field("Data:?", PropEntry.second.emplace<float>()   ); Err) return; 
                                                    break;
                                                case textfile_property_types_v[xcore::types::variant_t2i_v<bool,            property::settings::data_variant>].m_CRC.m_Value:
                                                    if( Err = TextFile.Field("Data:?", PropEntry.second.emplace<bool>()    ); Err) return; 
                                                    break;
                                                case textfile_property_types_v[xcore::types::variant_t2i_v<string_t,        property::settings::data_variant>].m_CRC.m_Value:
                                                    if( Err = TextFile.Field("Data:?", PropEntry.second.emplace<string_t>()); Err) return; 
                                                    break;
                                                case textfile_property_types_v[xcore::types::variant_t2i_v<xcore::vector2,  property::settings::data_variant>].m_CRC.m_Value:
                                                {
                                                    auto& V2 = PropEntry.second.emplace<xcore::vector2>();
                                                    if( Err = TextFile.Field("Data:?", V2.m_X, V2.m_Y ); Err ) return; break;
                                                    break;
                                                }
                                                case textfile_property_types_v[xcore::types::variant_t2i_v<xecs::component::entity, property::settings::data_variant>].m_CRC.m_Value:
                                                    if( Err = TextFile.Field("Data:?", PropEntry.second.emplace<xecs::component::entity>().m_Value ); Err) return; 
                                                    break;
                                                case textfile_property_types_v[xcore::types::variant_t2i_v<std::int16_t,            property::settings::data_variant>].m_CRC.m_Value:
                                                    if( Err = TextFile.Field("Data:?", PropEntry.second.emplace<std::int16_t>() ); Err) return; 
                                                    break;
                                                default: assert(false);
                                            }
                                            static_assert(std::variant_size_v<property::settings::data_variant> == 7, "You need to change code inside this function ");

                                            if( false == property::set( Table, pData, PropEntry.first.c_str(), PropEntry.second ) )
                                            {
                                                printf("Warning failt to set a property while loading: %s", PropEntry.first.c_str() );
                                            }
                                            pData += TypeSize;
                                        }
                                    ); Error )
                                    {
                                        if( Error.getCode().getState<xcore::textfile::error_state>() == xcore::textfile::error_state::UNEXPECTED_RECORD )
                                        {
                                            printf( "Warning: We were expecting a table but we failed to find it");
                                            Error.clear();
                                        }
                                        else
                                        {
                                            return Error;
                                        }
                                    }
                                } // for
                            }
                            else
                            {
                                if( SerializedModes[iType] != serialized_mode::MODE_NONE )
                                {
                                    if( SerializedModes[iType] == serialized_mode::MODE_SERIALIZER )
                                    {
                                        return xerr_failure_s("Error: TextFile should have a skip record function, but none the less we failed to load file because we don't have serialized functions any more");
                                    }
                                    else if( SerializedModes[iType] == serialized_mode::MODE_PROPERTY )
                                    {
                                        //TextFile.SkipRecord();
                                        return xerr_failure_s("Error: TextFile should have a skip record function, but none the less we failed to load file because we don't have properties any more");
                                    }
                                    else
                                    {
                                        assert(false);
                                    }
                                }

                                continue;
                            }
        
                            // Are we dealing with the entities?
                            if( iType == 0 )
                            {
                                //
                                // Hook up with the global entries
                                //
                                for( int i=0; i<nEntitiesInPool; i++ )
                                {
                                    auto& Entity = reinterpret_cast<xecs::component::entity*>(pP->m_pComponent[0])[i];
                                    auto& Global = m_ComponentMgr.m_lEntities[Entity.m_GlobalIndex];

                                    assert( Global.m_Validation == Entity.m_Validation );
                                    Global.m_PoolIndex  = xecs::pool::index{i};
                                    Global.m_pArchetype = pArchetype;
                                    Global.m_pPool      = pP;
                                }
                            }
                        }

                        //
                        // Add to the pending list
                        //
                        m_ArchetypeMgr.AddToStructuralPendingList(*pP);

                        //
                        // If we need to add another pool lets do so
                        //
                        if((iPool+1) != nPools)
                        {
                            pP->m_Next = std::make_unique<pool::instance>();
                            pP->m_Next->Initialize(pF->m_DefaultPool.m_ComponentInfos, *pF);
                        }
                    }
                    else
                    {
                        for( auto iType=0, end = (int)pP->m_ComponentInfos.size(); iType != end; iType++ )
                        {
                            auto& PropInfo = *pP->m_ComponentInfos[iType];
                            if( SerializedModes[iType] == serialized_mode::MODE_SERIALIZER )
                            {
                                int Count = pP->Size();
                                Error = pP->m_ComponentInfos[iType]->m_pSerilizeFn( TextFile, isRead, pP->m_pComponent[iType], Count );
                                if(Error) return Error;
                            }
                            else if( SerializedModes[iType] == serialized_mode::MODE_PROPERTY )
                            {
                                int         Count    = pP->Size();
                                std::byte*  pData    = pP->m_pComponent[iType];
                                auto        TypeSize = PropInfo.m_Size;
                                auto&       Table    = *PropInfo.m_pPropertyTable;

                                std::vector<property::entry> PropertyList;
                                for( int i=0; i<Count; ++i )
                                {
                                    // Collect all the properties for a single component
                                    PropertyList.clear();
                                    property::SerializeEnum( Table, pData + TypeSize * i, [&]( std::string_view PropertyName, property::data&& Data, const property::table&, std::size_t, property::flags::type Flags )
                                    {
                                        // If we are dealing with a scope that is not an array someone may have change the SerializeEnum to a DisplayEnum they only show up there.
                                        assert( Flags.m_isScope == false || PropertyName.back() == ']' );
                                        if( Flags.m_isDontSave || Flags.m_isScope ) return;
                                        PropertyList.push_back( property::entry { PropertyName, Data } );
                                    });

                                    // Serialize the properties for the give component
                                    TextFile.Record
                                    (   Error
                                        , "Props"
                                        , [&](std::size_t& C, xcore::err&) noexcept
                                        {
                                            assert(isRead == false);
                                            C = PropertyList.size();
                                        }
                                        , [&](std::size_t c, xcore::err& Error) noexcept
                                        {
                                            auto&       Entry       = PropertyList[c];

                                            if( auto Err = TextFile.Field( "Name", Entry.first ); Err ) return;

                                            std::visit( [&]( auto&& Value )
                                            {
                                                using           T           = std::decay_t<decltype( Value )>;
                                                constexpr auto  prop_type_v = xcore::types::variant_t2i_v<T, property::settings::data_variant>;
                                                constexpr auto  CRC         = textfile_property_types_v[prop_type_v].m_CRC;

                                                if constexpr ( std::is_same_v<T, int> )
                                                {
                                                    if (auto Err = TextFile.Field(CRC, "Data:?", Value ); Err) return;
                                                }
                                                else if constexpr ( std::is_same_v<T, float> )
                                                {
                                                    if (auto Err = TextFile.Field(CRC, "Data:?", Value); Err) return;
                                                }
                                                else if constexpr ( std::is_same_v<T, bool> )
                                                {
                                                    if (auto Err = TextFile.Field(CRC, "Data:?", Value); Err) return;
                                                }
                                                else if constexpr ( std::is_same_v<T, string_t> )
                                                {
                                                    if (auto Err = TextFile.Field(CRC, "Data:?", Value); Err) return;
                                                }
                                                else if constexpr ( std::is_same_v<T, xcore::vector2> )
                                                {
                                                    if (auto Err = TextFile.Field(CRC, "Data:?", Value.m_X, Value.m_Y ); Err) return;
                                                }
                                                else if constexpr (std::is_same_v<T, xecs::component::entity>)
                                                {
                                                    if (auto Err = TextFile.Field(CRC, "Data:?", Value.m_Value); Err) return;
                                                }
                                                else if constexpr (std::is_same_v<T, std::int16_t>)
                                                {
                                                    if (auto Err = TextFile.Field(CRC, "Data:?", Value ); Err) return;
                                                }
                                                else static_assert( always_false<T>::value, "We are not covering all the cases!" );
                                            }
                                            , Entry.second );
                                            

                                        }
                                    );
                                }
                            }
                            else
                            {
                                assert( SerializedModes[iType] == serialized_mode::MODE_NONE );
                                continue;
                            }
                        }
                    }
                }
            }
        }

        //
        // Fill unused global entities to the empty list
        //
        if( isRead )
        {
            m_ComponentMgr.m_EmptyHead = -1;
            auto Span = std::span{ m_ComponentMgr.m_lEntities.get(), xecs::settings::max_entities_v };
            for( auto It = Span.rbegin(); It != Span.rend(); ++It )
            {
                auto& E = *It;
                if( E.m_pPool == nullptr )
                {
                    E.m_PoolIndex.m_Value = m_ComponentMgr.m_EmptyHead;
                    m_ComponentMgr.m_EmptyHead = static_cast<int>(static_cast<std::size_t>(&E - m_ComponentMgr.m_lEntities.get()));
                }
            }

            //
            // Lets Update the structural changes
            //
            m_ArchetypeMgr.UpdateStructuralChanges();
        }

        return Error;
    }
}