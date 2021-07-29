// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// This is C++ 20 don't forget
//
#define GLUT_STATIC_LIB
#include "GL/glut.h"
#include <random>
#include <array>
#include <filesystem>

#include "LiveCoding.h"

//---------------------------------------------------------------------------------------
// INPUT
//---------------------------------------------------------------------------------------

struct keys
{
    enum class type : std::uint8_t
    {
        IS    =   (1<<0)
    ,   UP    =   (1<<1)
    ,   DOWN  =   (1<<2)
    };

    using keys_array = std::array<std::uint8_t, 0xff + 1>;
    keys_array      m_Keys{};

    void setKeyState( char x, bool OnOff ) noexcept
    {
        auto& Key = m_Keys[static_cast<std::uint8_t>(x)];

        if( OnOff )
        {
            if (!(Key & static_cast<std::uint8_t>(type::IS)))
            {
                Key |= ( static_cast<std::uint8_t>(type::IS) | static_cast<std::uint8_t>(type::DOWN) );
            }
        }
        else
        {
            if (Key & static_cast<std::uint8_t>(type::IS))
            {
                Key &= ~(static_cast<std::uint8_t>(type::IS) | static_cast<std::uint8_t>(type::DOWN));
                Key |= static_cast<std::uint8_t>(type::UP);
            }
        }
    }

    bool getKeyUp( char x ) noexcept
    {
        return !!(m_Keys[static_cast<std::uint8_t>(x)] & static_cast<std::uint8_t>(type::UP));
    }

    bool getKeyDown ( char x ) noexcept
    {
        return !!(m_Keys[static_cast<std::uint8_t>(x)] & static_cast<std::uint8_t>(type::DOWN));
    }

    bool getKey( char x ) noexcept
    {
        return !!(m_Keys[static_cast<std::uint8_t>(x)] & static_cast<std::uint8_t>(type::IS));
    }

    void FrameUpdate( void ) noexcept
    {
        for( auto& K : m_Keys )
        {
            K = K & (~( static_cast<std::uint8_t>(type::UP) | static_cast<std::uint8_t>(type::DOWN)));
        }
    }
};

//---------------------------------------------------------------------------------------

static struct input_state
{
    int             m_MouseX{};
    int             m_MouseY{};
    bool            m_MouseLeft{};
    bool            m_MouseRight{};
    keys            m_Keys{};

} s_Input;

//---------------------------------------------------------------------------------------
// RENDER
//---------------------------------------------------------------------------------------

struct render_driver final : live::renderer
{
    int             m_W = 1024;
    int             m_H = 800;

    //---------------------------------------------------------------------------------------

    virtual void PageFlip( void ) noexcept override
    {
        //        glFlush();
        glutSwapBuffers();
    }

    //---------------------------------------------------------------------------------------

    virtual void PrintAt(int x, int y, const char* String, int Length) noexcept override
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, m_W, 0, m_H);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glRasterPos2i(x, m_H - (y + 20));
        for (int i = 0; i < Length; ++i)
        {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, String[i]);
        }
        glPopMatrix();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }

    //---------------------------------------------------------------------------------------

    virtual int getWidth(void) noexcept override
    {
        return m_W;
    }

    //---------------------------------------------------------------------------------------

    virtual int getHeight(void) noexcept override
    {
        return m_H;
    }

} s_Render;


//---------------------------------------------------------------------------------------
// GLUT TIMER
//---------------------------------------------------------------------------------------

void timer( int value ) noexcept
{
    // Post re-paint request to activate display()
    glutPostRedisplay();

    // next timer call milliseconds later
    glutTimerFunc(15, timer, 0);
}

//---------------------------------------------------------------------------------------
// MAIN
//---------------------------------------------------------------------------------------
live::plugin* pGamePlugin;
int main(int argc, char** argv)
{
    live::plugin GamePlugin;

    // Give access to clut functions
    pGamePlugin = &GamePlugin;

    // Initialize with the render
    GamePlugin.Initialize( s_Render );

    // Load the plugin
    GamePlugin.Load("Game/x64/Debug/Game.dll");

    //
    // Create the graphics and main loop
    //
    glutInitWindowSize(s_Render.m_W, s_Render.m_H);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE);
    glutCreateWindow("xECS Live Coding Example");

    glutDisplayFunc([]( void ) noexcept
    {
        pGamePlugin->Run();

        if(s_Input.m_Keys.getKeyUp('s') || s_Input.m_Keys.getKeyUp('x') )
        {
            pGamePlugin->SaveGameState( "x64/Test.bin" );
        }

        if (s_Input.m_Keys.getKeyUp('l') || s_Input.m_Keys.getKeyUp('x') )
        {
            pGamePlugin->LoadGameState( "x64/Test.bin" );
        }

        s_Input.m_Keys.FrameUpdate();
    });

    glutReshapeFunc([](int w, int h) noexcept
    {
        s_Render.m_W = w;
        s_Render.m_H = h;
    });
    glutTimerFunc( 0, timer, 0 );
    glutKeyboardFunc([](unsigned char Key, int MouseX, int MouseY) noexcept
    {
        s_Input.m_Keys.setKeyState(Key, true);
        s_Input.m_MouseX = MouseX;
        s_Input.m_MouseY = MouseY;
    });
    glutKeyboardUpFunc([](unsigned char Key, int MouseX, int MouseY) noexcept
    {
        s_Input.m_Keys.setKeyState(Key, false);
        s_Input.m_MouseX = MouseX;
        s_Input.m_MouseY = MouseY;
    });
    glutMouseFunc([](int Button, int State, int MouseX, int MouseY) noexcept
    {
        s_Input.m_MouseX = MouseX;
        s_Input.m_MouseY = MouseY;
             if (Button == GLUT_LEFT_BUTTON ) s_Input.m_MouseLeft  = (State == GLUT_DOWN);
        else if (Button == GLUT_RIGHT_BUTTON) s_Input.m_MouseRight = (State == GLUT_DOWN);
    });

    glutMainLoop();

    return 0;
}

