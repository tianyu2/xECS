struct position
{
    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName = "Position"
    };

    xcore::err Serialize( xecs::serializer::stream& TextFile, bool ) noexcept
    {
        TextFile.Field("Value", m_Value.m_X, m_Value.m_Y).clear();
        return{};
    }

    xcore::vector2 m_Value;
};

property_begin(position)
{ property_var(m_Value)
}
property_end()

