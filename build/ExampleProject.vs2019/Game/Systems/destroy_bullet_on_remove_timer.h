struct destroy_bullet_on_remove_timer : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::notify_moved_out
    {
        .m_pName = "destroy_bullet_on_timer_deletion"
    };

    using query = std::tuple
    <
        xecs::query::must<bullet, timer>
    >;

    __inline
    void operator()( entity& Entity ) const noexcept
    {
        DeleteEntity(Entity);
    }
};
