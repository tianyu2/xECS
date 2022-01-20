struct bullet
{
    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName = "Bullet"
    ,   .m_pSerilizeFn = xecs::component::entity::Serialize
    };

    xecs::component::entity m_ShipOwner;
};

property_begin(bullet)
{
    property_var(m_ShipOwner)
}
property_end()
