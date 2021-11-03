struct grid_cell
{
    static xcore::err Serialize(xcore::textfile::stream& TextFile, bool, std::byte* pData) noexcept
    {
        auto& GridCell = *reinterpret_cast<grid_cell*>(pData);
        TextFile.Field( "X", GridCell.m_X ).clear();
        TextFile.Field( "Y", GridCell.m_Y ).clear();
        return {};
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

property_begin(grid_cell)
{
    property_var(m_X)
        .Flags( property::flags::SHOW_READONLY)
,   property_var(m_Y)
        .Flags( property::flags::SHOW_READONLY)
}
property_end()
