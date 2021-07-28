struct position
{
    static xcore::err Serialize(xcore::textfile::stream& TextFile, bool, std::byte* pData) noexcept
    {
        auto& Position = *reinterpret_cast<position*>(pData);
        return TextFile.Field("Value", Position.m_Value.m_X, Position.m_Value.m_Y);
    }

    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName = "Position"
    ,   .m_pSerilizeFn = Serialize
    };

    xcore::vector2 m_Value;
};
