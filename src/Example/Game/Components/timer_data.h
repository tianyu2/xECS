struct timer
{
    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName = "Timer"
    };

    float m_Value;
};

property_begin(timer)
{
    property_var(m_Value)
}
property_end()

