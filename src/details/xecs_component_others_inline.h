namespace xecs::component
{
    xcore::err ref_count::Serialize( xecs::serializer::stream& TextFile, bool, std::byte* pData ) noexcept
    { 
        auto& RefCount = *reinterpret_cast<ref_count*>(pData);
        return TextFile.Field("GlobalIndex", RefCount.m_Value);
    }

    //----------------------------------------------------------------------------------------------------

    xcore::err parent::Serialize( xecs::serializer::stream& TextFile, bool, std::byte* pData ) noexcept
    {
        auto& Parent = *reinterpret_cast<parent*>(pData);
        return TextFile.Field("Parent", Parent.m_Value );
    }

    //----------------------------------------------------------------------------------------------------

    xcore::err children::FullSerialize( xecs::serializer::stream& TextFile, bool isRead, std::byte* pData, int& Count ) noexcept
    {
        xcore::err Error;

        //
        // For each of the components write how many children they have
        //
        int nTotalChildren = 0;
        if( TextFile.Record(Error, "Children"
        ,[&](std::size_t& Size, xcore::err& ) noexcept
        {
            if( isRead ) Count  = static_cast<int>(Size);
            else         Size   = static_cast<std::size_t>(Count);
        }
        , [&]( std::size_t Index, xcore::err& Err ) noexcept
        {
            auto& Children = reinterpret_cast<children*>(pData)[Index];

            int nChildren = isRead ? static_cast<int>(Children.m_List.size()) : 0;
            if( Err = TextFile.Field("nChildren", nChildren); Err ) return;
            if(isRead) Children.m_List.resize(nChildren);

            nTotalChildren += nChildren;

        }) ) return Error;

        //
        // Now that we know all the children from all the components list write all the children together
        //
        auto pChildren = reinterpret_cast<children*>(pData);
        int  iChild    = 0;
        if( nTotalChildren && TextFile.Record(Error, "AllChildren"
        , [&](std::size_t& Size, xcore::err& ) noexcept
        {
            if (isRead) xassert( nTotalChildren == static_cast<int>(Size) );
            else        Size = static_cast<std::size_t>(nTotalChildren);
        }
        , [&]( std::size_t, xcore::err& Err ) noexcept
        {
            while(iChild >= pChildren->m_List.size() )
            {
                pChildren++;
                iChild = 0;
            }

            Err = TextFile.Field("Entity", pChildren->m_List[iChild] );

        })) return Error;

        return Error;
    }

}