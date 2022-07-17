struct velocity
{
    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName = "Velocity"
    };

    xcore::vector2 m_Value;
};

property_begin(velocity)
{
    property_var(m_Value)
}
property_end()

