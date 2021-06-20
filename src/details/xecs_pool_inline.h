namespace xecs::pool
{
    //-------------------------------------------------------------------------------------

    instance::~instance(void) noexcept
    {
        if (m_pComponent[0])
        {
            Clear();
            for (int i = 0; i < m_Infos.size(); ++i)
            {
                VirtualFree(m_pComponent[i], 0, MEM_RELEASE);
            }
        }
    }

    //-------------------------------------------------------------------------------------
    // This function return which page is the last byte of a given entry
    //-------------------------------------------------------------------------------------
    constexpr inline 
    int getPageFromIndex( const component::info& Info, int iEntity ) noexcept
    {
        return ((iEntity * Info.m_Size)-1) / xecs::settings::virtual_page_size_v;
    }

    //-------------------------------------------------------------------------------------

    void instance::Initialize( std::span<const component::info* const > Span ) noexcept
    {
        m_Infos        = Span;
        for( int i=0; i< m_Infos.size(); ++i )
        {
            assert(m_Infos[i]->m_Size <= xecs::settings::virtual_page_size_v);
            auto nPages     = getPageFromIndex( *m_Infos[i], xecs::settings::max_entity_count_per_pool_v ) + 1;
            m_pComponent[i] = reinterpret_cast<std::byte*>(VirtualAlloc(nullptr, nPages * xecs::settings::virtual_page_size_v, MEM_RESERVE, PAGE_NOACCESS));
            assert(m_pComponent[i]);
        }
    }

    //-------------------------------------------------------------------------------------

    void instance::Clear( void ) noexcept
    {
        while(m_Size)
        {
            Delete(m_Size-1);
        }
    }

    //-------------------------------------------------------------------------------------

    int instance::Append( int Count ) noexcept
    {
        assert( m_Size < (xecs::settings::max_entities_v-1) );

        for( int i = 0; i < m_Infos.size(); ++i )
        {
            const auto&   MyInfo  = *m_Infos[i];
            const auto    NexPage = getPageFromIndex(MyInfo, m_Size+Count);

            // Create pages when needed 
            if( auto Cur = getPageFromIndex(MyInfo, m_Size); Cur != NexPage )
            {
                auto pNewPagePtr = m_pComponent[i] + xecs::settings::virtual_page_size_v * (Cur+1);
                auto p           = reinterpret_cast<std::byte*>(VirtualAlloc(pNewPagePtr, (NexPage - Cur) * xecs::settings::virtual_page_size_v, MEM_COMMIT, PAGE_READWRITE));
                assert(p == pNewPagePtr);
            }

            // Construct if required
            if( MyInfo.m_pConstructFn ) 
            {
                auto p = &m_pComponent[i][m_Size * MyInfo.m_Size];
                for(int j=Count; j; --j )
                {
                    MyInfo.m_pConstructFn(p);
                    p += MyInfo.m_Size;
                }
            }
        }

        auto Index = m_Size;
        m_Size += Count;
        return Index;
    }

    //-------------------------------------------------------------------------------------

    void instance::Free( int Index, bool bCallDestructors ) noexcept
    {
        assert(Index < m_Size );
        assert(Index>=0);
        
        m_Size--;
        if( Index == m_Size )
        {
            for (int i = 0; i < m_Infos.size(); ++i)
            {
                const auto& MyInfo = *m_Infos[i];
                auto        pData  = m_pComponent[i];
                if (MyInfo.m_pDestructFn && bCallDestructors) MyInfo.m_pDestructFn( &pData[m_Size * MyInfo.m_Size] );

                // Free page if we cross over
                const auto    LastEntryPage = getPageFromIndex(MyInfo, m_Size+1);
                if( getPageFromIndex(MyInfo, m_Size) != LastEntryPage )
                {
                    auto pRaw = &pData[xecs::settings::virtual_page_size_v * LastEntryPage ];
                    auto b    = VirtualFree(pRaw, xecs::settings::virtual_page_size_v, MEM_DECOMMIT);
                    assert(b);
                }
            }
        }
        else
        {
            for (int i = 0; i < m_Infos.size(); ++i)
            {
                const auto& MyInfo = *m_Infos[i];
                auto        pData  = m_pComponent[i];

                if ( MyInfo.m_pMoveFn )
                {
                    MyInfo.m_pMoveFn( &pData[Index * MyInfo.m_Size], &pData[m_Size * MyInfo.m_Size] );
                }
                else
                {
                    memcpy(&pData[Index * MyInfo.m_Size], &pData[m_Size * MyInfo.m_Size], MyInfo.m_Size );
                }

                // Free page if we cross over
                const auto    LastEntryPage = getPageFromIndex(MyInfo, m_Size+1);
                if( getPageFromIndex(MyInfo, m_Size) != LastEntryPage )
                {
                    auto pRaw = &pData[xecs::settings::virtual_page_size_v * LastEntryPage ];
                    auto b    = VirtualFree(pRaw, xecs::settings::virtual_page_size_v, MEM_DECOMMIT);
                    assert(b);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------
    constexpr
    int instance::Size( void ) const noexcept
    {
        return m_CurrentCount;
    }

    //-------------------------------------------------------------------------------------

    constexpr
    int instance::findIndexComponentFromGUIDInSequence
    (xecs::component::type::guid  SearchGuid
    , int&                              Sequence 
    ) const noexcept
    {
        const auto Backup = Sequence;
        for( const auto end = static_cast<const int>(m_Infos.size()); Sequence < end; ++Sequence)
        {
            const auto InfoGuid = m_Infos[Sequence]->m_Guid;
            if (InfoGuid == SearchGuid) return Sequence++;
            [[unlikely]] if ( InfoGuid.m_Value > SearchGuid.m_Value) break;
        }
        Sequence = Backup;
        return -1;
    }

    //-------------------------------------------------------------------------------------
    constexpr
    int instance::findIndexComponentFromGUID( xecs::component::type::guid SearchGuid) const noexcept
    {
        for( int i=0, end = static_cast<int>(m_Infos.size()); i<end; ++i )
        {
            const auto InfoGuid = m_Infos[i]->m_Guid;
            if(InfoGuid == SearchGuid) return i;
            [[unlikely]] if(InfoGuid.m_Value > SearchGuid.m_Value) return -1;
        }
        return -1;
    }

    //-------------------------------------------------------------------------------------

    template
    < typename T_COMPONENT
    > requires
    ( std::is_same_v<T_COMPONENT, std::decay_t<T_COMPONENT>>
    )
    T_COMPONENT& instance::getComponent( const std::uint32_t EntityIndex ) const noexcept
    {
        if constexpr( std::is_same_v<xecs::component::entity, T_COMPONENT>)
        {
            return *reinterpret_cast<T_COMPONENT*>
            (
                &m_pComponent[0][ EntityIndex * sizeof(T_COMPONENT) ]
            );
        }
        else
        {
            const auto iComponent = findIndexComponentFromGUID( xecs::component::info_v<T_COMPONENT>.m_Guid );
            return *reinterpret_cast<T_COMPONENT*>
            (
                &m_pComponent[iComponent][ EntityIndex * sizeof(T_COMPONENT) ]
            );
        }
    }

    //-------------------------------------------------------------------------------------

    template
    < typename T_COMPONENT
    > requires
    ( std::is_same_v<T_COMPONENT, std::decay_t<T_COMPONENT>>
    )
    T_COMPONENT& instance::getComponentInSequence
    ( std::uint32_t EntityIndex
    , int&          Sequence
    ) const noexcept
    {
        if constexpr (std::is_same_v<xecs::component::entity, T_COMPONENT>)
        {
            assert(Sequence==0);
            Sequence = 1;
            return *reinterpret_cast<T_COMPONENT*>
            (
                &m_pComponent[0][EntityIndex * sizeof(T_COMPONENT)]
            );
        }
        else
        {
            const auto iComponent = findIndexComponentFromGUIDInSequence(xecs::component::info_v<T_COMPONENT>.m_Guid, Sequence );
            return *reinterpret_cast<T_COMPONENT*>
            (
                &m_pComponent[iComponent][ EntityIndex * sizeof(T_COMPONENT) ]
            );
        }
    }

    //-------------------------------------------------------------------------------------

    void instance::UpdateStructuralChanges( xecs::game_mgr::instance& GameMgr ) noexcept
    {
        //
        // Delete the regular entries
        //
        while( m_DeleteGlobalIndex != invalid_delete_global_index_v )
        {
            auto&       Entry                   = GameMgr.m_lEntities[m_DeleteGlobalIndex];
            auto&       PoolEntity              = reinterpret_cast<xecs::component::entity&>(m_pComponent[0][sizeof(xecs::component::entity)*Entry.m_PoolIndex]);
            const auto  NextDeleteGlobalIndex   = PoolEntity.m_Validation.m_Generation;
            assert(PoolEntity.m_Validation.m_bZombie);

            Free( Entry.m_PoolIndex, true );
            if (Entry.m_PoolIndex == m_Size )
            {
                GameMgr.DeleteGlobalEntity(m_DeleteGlobalIndex);
            }
            else
            {
                GameMgr.DeleteGlobalEntity(m_DeleteGlobalIndex, PoolEntity);
            }

            m_DeleteGlobalIndex = NextDeleteGlobalIndex;
        }

        //
        // Delete Entities that were moved
        //
        while (m_DeleteMoveIndex != invalid_delete_global_index_v)
        {
            auto&       PoolEntity            = reinterpret_cast<xecs::component::entity&>(m_pComponent[0][sizeof(xecs::component::entity) * m_DeleteMoveIndex]);
            const auto  NextDeleteGlobalIndex = PoolEntity.m_Validation.m_Generation;
            Free( m_DeleteMoveIndex, false );
            if (m_DeleteMoveIndex != m_Size)
            {
                GameMgr.MovedGlobalEntity( m_DeleteMoveIndex, PoolEntity );
            }

            m_DeleteMoveIndex = NextDeleteGlobalIndex;
        }

        // Update the current count
        m_CurrentCount = m_Size;
    }

    //-------------------------------------------------------------------------------------
    inline
    void instance::Delete( int Index ) noexcept
    {
        auto& Entity = getComponent<xecs::component::entity>(Index);
        assert(Entity.m_Validation.m_bZombie);
        Entity.m_Validation.m_Generation = m_DeleteGlobalIndex;
        m_DeleteGlobalIndex = Entity.m_GlobalIndex;
    }

    //-------------------------------------------------------------------------------------
    inline
    void instance::MoveDelete( int Index ) noexcept
    {
        auto& Entity = getComponent<xecs::component::entity>(Index);
        Entity.m_Validation.m_bZombie    = true;
        Entity.m_Validation.m_Generation = m_DeleteMoveIndex;
        m_DeleteMoveIndex = Index;
    }

    //--------------------------------------------------------------------------------------------
    constexpr
    bool instance::isLastEntry( int Index ) const noexcept
    {
        return Index == m_Size - 1;
    }

    //--------------------------------------------------------------------------------------------
    inline
    int instance::MoveInFromPool( int IndexToMove, pool::instance& FromPool ) noexcept
    {
        const int iNewIndex = Append(1);

        int iPoolFrom = 0;
        int iPoolTo   = 0;
        while( true )
        {
            if( FromPool.m_Infos[iPoolFrom] == m_Infos[iPoolTo] )
            {
                auto& Info = *FromPool.m_Infos[iPoolFrom];
                if( Info.m_pMoveFn )
                {
                    Info.m_pMoveFn
                    (
                        &m_pComponent[iPoolTo][Info.m_Size* iNewIndex]
                    ,   &FromPool.m_pComponent[iPoolFrom][Info.m_Size* IndexToMove]
                    );
                }
                else
                {
                    std::memcpy
                    ( 
                        &m_pComponent[iPoolTo][Info.m_Size * iNewIndex]
                    ,   &FromPool.m_pComponent[iPoolFrom][Info.m_Size * IndexToMove]
                    ,   Info.m_Size
                    );
                }
                iPoolFrom++;
                iPoolTo++;
                if (iPoolFrom >= FromPool.m_Infos.size() || iPoolTo >= m_Infos.size())
                    break;
            }
            else if(FromPool.m_Infos[iPoolFrom]->m_Guid.m_Value < m_Infos[iPoolTo]->m_Guid.m_Value )
            {
                auto& Info = *FromPool.m_Infos[iPoolFrom];
                if( Info.m_pDestructFn ) Info.m_pDestructFn( &FromPool.m_pComponent[iPoolFrom][Info.m_Size * IndexToMove] );
                iPoolFrom++;
                if (iPoolFrom >= FromPool.m_Infos.size())
                    break;
            }
            else
            {
                // This is already constructed so there is nothing for me to do
                iPoolTo++;
                if (iPoolTo >= m_Infos.size())
                    break;
            }
        }

        //
        // Destruct any pending component
        //
        while (iPoolFrom < FromPool.m_Infos.size())
        {
            auto& Info = *FromPool.m_Infos[iPoolFrom];
            if (Info.m_pDestructFn) Info.m_pDestructFn(&FromPool.m_pComponent[iPoolFrom][Info.m_Size * IndexToMove]);
            iPoolFrom++;
            if (iPoolFrom >= FromPool.m_Infos.size()) break;
        }

        return iNewIndex;
    }

}