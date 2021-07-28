struct velocity
{
    static xcore::err Serialize(xcore::textfile::stream& TextFile, bool, std::byte* pData) noexcept
    {
        auto& Velocity = *reinterpret_cast<velocity*>(pData);
        return TextFile.Field("Value", Velocity.m_Value.m_X, Velocity.m_Value.m_Y);
    }

    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName = "Velocity"
    ,   .m_pSerilizeFn = Serialize
    };

    xcore::vector2 m_Value;
};
