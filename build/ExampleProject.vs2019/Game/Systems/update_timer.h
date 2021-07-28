struct update_timer : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::update
    {
        .m_pName = "update_timer"
    };

    __inline constexpr
    void operator()( entity& Entity, timer& Timer ) const noexcept
    {
        Timer.m_Value -= 0.01f;
        if( Timer.m_Value <= 0 )
        {
            (void)AddOrRemoveComponents<std::tuple<>, std::tuple<timer>>(Entity);
        }
    }
};
