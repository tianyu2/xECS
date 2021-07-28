struct grid_cell
{
    static xcore::err Serialize(xcore::textfile::stream& TextFile, bool, std::byte* pData) noexcept
    {
        xcore::err  Error;
        auto&       GridCell = *reinterpret_cast<grid_cell*>(pData);

            (Error = TextFile.Field( "X", GridCell.m_X ))
        ||  (Error = TextFile.Field( "Y", GridCell.m_Y ));
        return Error;
    }

    constexpr static auto typedef_v = xecs::component::type::share
    {
        .m_pName        = "GridCell"
    ,   .m_bBuildFilter = true
    ,   .m_pSerilizeFn  = Serialize
    };

    std::int16_t m_X;
    std::int16_t m_Y;
};
