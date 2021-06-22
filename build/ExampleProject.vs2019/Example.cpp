// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// This is C++ 20 don't forget
//
#include "xecs.h"
#define GLUT_STATIC_LIB
#include "GL/glut.h"
#include <random>

//---------------------------------------------------------------------------------------
// GAME
//---------------------------------------------------------------------------------------

static struct game
{
    std::unique_ptr<xecs::game_mgr::instance> m_GameMgr = std::make_unique<xecs::game_mgr::instance>();
    int m_W = 1024;
    int m_H = 800;

} s_Game;

//---------------------------------------------------------------------------------------
// COMPONENTS
//---------------------------------------------------------------------------------------

struct position 
{
    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName = "Position"
    };
    xcore::vector2 m_Value;
};

struct velocity
{
    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName = "Velocity"
    };
    xcore::vector2 m_Value;
};

struct timer
{
    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName = "Timer"
    };

    using fncallback = void( xecs::system::instance&, xecs::component::entity&) noexcept;

    float        m_Value{};
    fncallback*  m_pCallback{};
};

struct bullet
{
    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName = "Bullet"
    };
    xecs::component::entity m_ShipOwner;
};

//---------------------------------------------------------------------------------------
// SYSTEM
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------

struct update_movement : xecs::system::instance
{
    constexpr static auto   name_v = "update_movement";

    void operator()(position& Position, velocity& Velocity) const noexcept
    {
        Position.m_Value += Velocity.m_Value;

        // Bounce on edges
        if (Position.m_Value.m_X < 0)
        {
            Position.m_Value.m_X = 0;
            Velocity.m_Value.m_X = -Velocity.m_Value.m_X;
        }
        else if (Position.m_Value.m_X >= s_Game.m_W)
        {
            Position.m_Value.m_X = s_Game.m_W - 1;
            Velocity.m_Value.m_X = -Velocity.m_Value.m_X;
        }

        if (Position.m_Value.m_Y < 0)
        {
            Position.m_Value.m_Y = 0;
            Velocity.m_Value.m_Y = -Velocity.m_Value.m_Y;
        }
        else if (Position.m_Value.m_Y >= s_Game.m_H)
        {
            Position.m_Value.m_Y = s_Game.m_H - 1;
            Velocity.m_Value.m_Y = -Velocity.m_Value.m_Y;
        }
    }
};

//---------------------------------------------------------------------------------------

struct update_timer : xecs::system::instance
{
    constexpr static auto   name_v = "update_timer";

    void operator()( entity& Entity, timer& Timer ) noexcept
    {
        Timer.m_Value -= 0.01f;
        if( Timer.m_Value <= 0 )
        {
            if( Timer.m_pCallback ) Timer.m_pCallback( *this, Entity );
            if( Entity.isZombie() == false )
            {
                (void)m_GameMgr.AddOrRemoveComponents<std::tuple<>, std::tuple<timer>>(Entity);
            }
        }
    }
};

//---------------------------------------------------------------------------------------

struct bullet_logic : xecs::system::instance
{
    constexpr static auto   name_v = "bullet_logic";

    void operator()( entity& Entity, position& Position, bullet& Bullet ) const noexcept
    {
        // If I am dead because some other bullet killed me then there is nothing for me to do...
        if (Entity.isZombie()) return;

        // Check for collisions
        xecs::query::instance Query;
        Query.m_Must.AddFromComponents<position>();
        m_GameMgr.Foreach
        (   m_GameMgr.Search(Query)
        ,   [&]( entity& E, position& Pos ) noexcept -> bool
        {
            assert( Entity.isZombie() == false );

            // Our we checking against my self?
            if( &Entity == &E ) return false;

            // Is this bullet or ship already dead?
            if( E.isZombie() ) return false;

            // Are we colliding with our own ship?
            // If so lets just continue
            if( Bullet.m_ShipOwner.m_Value == E.m_Value ) return false;

            constexpr auto distance_v = 3;
            if ((Pos.m_Value - Position.m_Value).getLengthSquared() < distance_v * distance_v)
            {
                m_GameMgr.DeleteEntity(Entity);
                m_GameMgr.DeleteEntity(E);
                return true;
            }

            return false;
        });
    }
};

//---------------------------------------------------------------------------------------

struct space_ship_logic : xecs::system::instance
{
    constexpr static auto   name_v = "space_ship_logic";

    using query = std::tuple
    <
        xecs::query::none_of<bullet, timer>
    >;

    void operator()( entity& Entity, position& Position ) const noexcept
    {
        xecs::query::instance    Query;
        Query.m_NoneOf.AddFromComponents<bullet>();
        m_GameMgr.Foreach
        ( m_GameMgr.Search(Query)
        , [&](position& Pos ) noexcept -> bool
        {
            // Don't shoot myself
            if( &Pos == &Position ) return false;

            auto        Direction        = Pos.m_Value - Position.m_Value;
            const auto  DistanceSquare   = Direction.getLengthSquared();

            // Shoot a bullet if close enough
            constexpr auto min_distance_v = 30;
            if( DistanceSquare < min_distance_v*min_distance_v )
            {
                auto NewEntity = m_GameMgr.AddOrRemoveComponents<std::tuple<timer>>( Entity, [&]( timer& Timer )
                {
                    Timer.m_Value = 8;
                });
                assert( NewEntity.m_GlobalIndex             == Entity.m_GlobalIndex          );     // This still remains the same but old Entity should not longer be used
                assert( NewEntity.m_Validation              != Entity.m_Validation           );     // Destroy because of the link-list
                assert( NewEntity.m_Validation.m_bZombie    != Entity.m_Validation.m_bZombie );     // Also the entity is marked as deleted from the pool

                m_GameMgr.getOrCreateArchetype<position, velocity, timer, bullet>()
                    .CreateEntity([&]( position& Pos, velocity& Vel, bullet& Bullet, timer& Timer )
                    {
                        Direction  /= std::sqrt(DistanceSquare);
                        Vel.m_Value = Direction * 2.0f;
                        Pos.m_Value = Position.m_Value + Vel.m_Value;

                        Bullet.m_ShipOwner = NewEntity;

                        Timer.m_Value      = 10;
                        Timer.m_pCallback  = []( xecs::system::instance& System, xecs::component::entity& Entity ) noexcept
                        {
                            // Ones the timer is up then just delete ourselves
                            System.m_GameMgr.DeleteEntity(Entity);
                        };
                    });
                return true;
            }
            return false;
        });
    }
};

//---------------------------------------------------------------------------------------

struct render_bullets : xecs::system::instance
{
    constexpr static auto   name_v = "render_bullets";

    using query = std::tuple
    <
        xecs::query::must<bullet>
    >;

    void operator()( position& Position, velocity& Velocity ) const noexcept
    {
        constexpr auto SizeX = 1;
        constexpr auto SizeY = SizeX*3;
        glBegin(GL_TRIANGLES);
        glColor3f(1.0, 0.5, 0.0);
        glVertex2i(Position.m_Value.m_X + Velocity.m_Value.m_X * SizeY, Position.m_Value.m_Y + Velocity.m_Value.m_Y * SizeY);
        glVertex2i(Position.m_Value.m_X + Velocity.m_Value.m_Y * SizeX, Position.m_Value.m_Y - Velocity.m_Value.m_X * SizeX);
        glVertex2i(Position.m_Value.m_X - Velocity.m_Value.m_Y * SizeX, Position.m_Value.m_Y + Velocity.m_Value.m_X * SizeX);
        glEnd();
    }
};

//---------------------------------------------------------------------------------------

struct render_ships : xecs::system::instance
{
    constexpr static auto   name_v = "render_ships";

    using query = std::tuple
    <
        xecs::query::none_of<bullet>
    ,   xecs::query::one_of<entity>
    >;

    void operator()( position& Position, timer* pTimer ) const noexcept
    {
        constexpr auto Size = 3;
        glBegin(GL_QUADS);
        if(pTimer) glColor3f(1.0, 1.0, 1.0);
        else       glColor3f(0.5, 1.0, 0.5);
        glVertex2i(Position.m_Value.m_X - Size, Position.m_Value.m_Y - Size);
        glVertex2i(Position.m_Value.m_X - Size, Position.m_Value.m_Y + Size);
        glVertex2i(Position.m_Value.m_X + Size, Position.m_Value.m_Y + Size);
        glVertex2i(Position.m_Value.m_X + Size, Position.m_Value.m_Y - Size);
        glEnd();                                         
    }
};

//---------------------------------------------------------------------------------------

struct page_flip : xecs::system::instance
{
    constexpr static auto   name_v = "page_flip";

    __inline
    void OnUpdate( void ) noexcept
    {
        glutSwapBuffers();
        glClear(GL_COLOR_BUFFER_BIT);
    }
};

//---------------------------------------------------------------------------------------

void InitializeGame( void ) noexcept
{
    //
    // Initialize global elements
    //
    std::srand(100);

    //
    // Register all the elements of the game
    //
    s_Game.m_GameMgr->RegisterComponents
    <   position
    ,   velocity
    ,   timer
    ,   bullet
    >();

    s_Game.m_GameMgr->RegisterSystems
    <   update_timer            // Structural: Yes, AddOrRemoveComponent(timer), User defined... (Destroy Bullets)
    ,   update_movement         // Structural: No
    ,   space_ship_logic        // Structural: Yes, AddOrRemoveComponent(timer), Create(Bullets)
    ,   bullet_logic            // Structural: Yes, Destroy(Bullets || Ships)
    ,   render_ships            // Structural: No
    ,   render_bullets          // Structural: No
    ,   page_flip               // Structural: No
    >();

    //
    // Generate a few random ships
    //
    s_Game.m_GameMgr->getOrCreateArchetype< position, velocity, timer >()
        .CreateEntities( 10000, [&]( position& Position, velocity& Velocity, timer& Timer )
        {
            Position.m_Value.m_X = std::rand() % s_Game.m_W;
            Position.m_Value.m_Y = std::rand() % s_Game.m_H;

            Velocity.m_Value.m_X = (std::rand() / (float)RAND_MAX) - 0.5f;
            Velocity.m_Value.m_Y = (std::rand() / (float)RAND_MAX) - 0.5f;
            Velocity.m_Value.Normalize();

            Timer.m_Value        = (std::rand() / (float)RAND_MAX) * 8;
        });
}

//---------------------------------------------------------------------------------------

void timer(int value)
{
    // Post re-paint request to activate display()
    glutPostRedisplay();

    // next timer call milliseconds later
    glutTimerFunc(15, timer, 0); 
}

//---------------------------------------------------------------------------------------
// MAIN
//---------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    xcore::Init("ECS Example");

    //
    // Initialize the game
    //
    InitializeGame();

    //
    // Create the graphics and main loop
    //
    glutInitWindowSize(s_Game.m_W, s_Game.m_H);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE);
    glutCreateWindow(xcore::get().m_pAppName);
    glutDisplayFunc([]
    {
        s_Game.m_GameMgr->Run();
    });
    glutReshapeFunc([](int w, int h)
    {
        s_Game.m_W = w;
        s_Game.m_H = h;
        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, w, 0, h, -1, 1);
        glScalef(1, -1, 1);
        glTranslatef(0, -h, 0);
    });
    glutTimerFunc(0, timer, 0);
    glutMainLoop();

    xcore::Kill();
    return 0;
}

