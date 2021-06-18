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
            auto nPages     = getPageFromIndex( *m_Infos[i], xecs::settings::max_entities_v ) + 1;
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

    int instance::Append( void ) noexcept
    {
        assert( m_Size < (xecs::settings::max_entities_v-1) );

        for( int i = 0; i < m_Infos.size(); ++i )
        {
            const auto&   MyInfo  = *m_Infos[i];
            const auto    NexPage = getPageFromIndex(MyInfo, m_Size+1);

            // Create pages when needed 
            if( getPageFromIndex(MyInfo, m_Size) != NexPage)
            {
                auto pNewPagePtr = m_pComponent[i] + xecs::settings::virtual_page_size_v * NexPage;
                auto p           = reinterpret_cast<std::byte*>(VirtualAlloc(pNewPagePtr, xecs::settings::virtual_page_size_v, MEM_COMMIT, PAGE_READWRITE));
                assert(p == pNewPagePtr);
            }

            // Construct if required
            if( MyInfo.m_pConstructFn ) MyInfo.m_pConstructFn( &m_pComponent[i][m_Size * MyInfo.m_Size] );
        }

        return m_Size++;
    }

    //-------------------------------------------------------------------------------------

    void instance::Delete( int Index ) noexcept
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
                if (MyInfo.m_pDestructFn) MyInfo.m_pDestructFn( &pData[m_Size * MyInfo.m_Size] );

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
                    if (MyInfo.m_pDestructFn) MyInfo.m_pDestructFn(&pData[Index * MyInfo.m_Size]);
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
        return m_Size;
    }

    //-------------------------------------------------------------------------------------
    constexpr
    int instance::findIndexComponentFromUIDComponent( const std::uint16_t UIDComponent ) const noexcept
    {
        for( int i=0, end = static_cast<int>(m_Infos.size()); i<end; ++i )
        {
            if(m_Infos[i]->m_UID == UIDComponent) return i;
        }
        return -1;
    }

    //-------------------------------------------------------------------------------------

    template< typename T_COMPONENT >
    T_COMPONENT& instance::getComponent( const std::uint32_t EntityIndex ) const noexcept
    {
        static_assert( std::is_same_v<T_COMPONENT, std::decay_t<T_COMPONENT>> );
        const auto iComponent = findIndexComponentFromUIDComponent( xecs::component::info_v<T_COMPONENT>.m_UID );
        return *reinterpret_cast<T_COMPONENT*>
        (
            &m_pComponent[iComponent][ EntityIndex * sizeof(T_COMPONENT) ]
        );
    }
}