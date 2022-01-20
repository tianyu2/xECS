struct position
{
    static xcore::err Serialize(xcore::textfile::stream& TextFile, bool, std::byte* pData) noexcept
    {
        auto& Position = *reinterpret_cast<position*>(pData);
        TextFile.Field("Value", Position.m_Value.m_X, Position.m_Value.m_Y).clear();
        return{};
    }

    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName = "Position"
//    ,   .m_pSerilizeFn = Serialize
    };

    xcore::vector2 m_Value;
};

property_begin(position)
{ property_var(m_Value)
}
property_end()

