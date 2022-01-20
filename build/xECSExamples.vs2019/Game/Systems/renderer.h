struct renderer : xecs::system::instance
{
    inline static live::renderer* s_pLiveRenderer = nullptr;

    constexpr static auto typedef_v = xecs::system::type::update
    {
        .m_pName = "renderer"
    };

    using update = xecs::event::instance<>;

    using events = std::tuple
    < update
    >;

    __inline
    void OnUpdate( void ) noexcept
    {
        //
        // Begin of the rendering
        //
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        glViewport(0, 0, s_pLiveRenderer->getWidth(), s_pLiveRenderer->getHeight());
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, s_pLiveRenderer->getWidth(), 0, s_pLiveRenderer->getHeight(), -1, 1);
        glScalef(1, -1, 1);
        glTranslatef(0, -s_pLiveRenderer->getHeight(), 0);

        //
        // Let all the system that depends on me
        //
        SendEventFrom<update>(this);

        //
        // Page Flip
        //
        renderer::s_pLiveRenderer->PageFlip();
    }
};
