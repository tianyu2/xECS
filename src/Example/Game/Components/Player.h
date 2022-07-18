struct Player
{
    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName = "Player"
    };

    xcore::err Serialize(xecs::serializer::stream& TextFile, bool) noexcept
    {
        return TextFile.Field("Player stats", playerPos, playerPos.m_Y, bullets);
    }

    xcore::vector2 playerPos;

    int bullets;
};

property_begin(Player)
{
    property_var(playerPos, bullets)
}
property_end()

