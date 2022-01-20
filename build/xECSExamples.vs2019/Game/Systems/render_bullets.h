struct render_bullets : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::child_update<renderer, renderer::update>
    {
        .m_pName = "render_bullets"
    };

    using query = std::tuple
    <
        xecs::query::must<bullet>
    >;

    void OnPreUpdate( void ) noexcept
    {
        glBegin(GL_TRIANGLES);
    }

    void OnPostUpdate( void ) noexcept
    {
        glEnd();
    }

    __inline
    void operator()( const position& Position, const velocity& Velocity ) const noexcept
    {
        constexpr auto SizeX = 1;
        constexpr auto SizeY = SizeX*3;
        glColor3f(1.0, 0.5, 0.0);
        glVertex2i(Position.m_Value.m_X + Velocity.m_Value.m_X * SizeY, Position.m_Value.m_Y + Velocity.m_Value.m_Y * SizeY);
        glVertex2i(Position.m_Value.m_X + Velocity.m_Value.m_Y * SizeX, Position.m_Value.m_Y - Velocity.m_Value.m_X * SizeX);
        glVertex2i(Position.m_Value.m_X - Velocity.m_Value.m_Y * SizeX, Position.m_Value.m_Y + Velocity.m_Value.m_X * SizeX);
    }
};
