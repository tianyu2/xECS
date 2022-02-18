struct bullet
{
    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName = "Bullet"
    };

    xcore::err Serialize( xecs::serializer::stream& TextFile, bool ) noexcept
    {
        return TextFile.Field( "ShipOwner", m_ShipOwner );
    }

    xecs::component::entity m_ShipOwner;
};

property_begin(bullet)
{
    property_var(m_ShipOwner)
}
property_end()
