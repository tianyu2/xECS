struct grid_cell
{
    constexpr static auto typedef_v = xecs::component::type::share
    {
        .m_pName        = "GridCell"
    ,   .m_bBuildFilter = true
    };

    xcore::err Serialize(xecs::serializer::stream& TextFile, bool) noexcept
    {
        TextFile.Field( "X", m_X ).clear();
        TextFile.Field( "Y", m_Y ).clear();
        return {};
    }

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
