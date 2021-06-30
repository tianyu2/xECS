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

    int m_nEntityThinkingDead = 0;
    int m_nEntityWaitingDead  = 0;

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

    float        m_Value;
};

struct bullet
{
    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName = "Bullet"
    };
    xecs::component::entity m_ShipOwner;
};

using bullet_tuple = std::tuple<position, velocity, timer, bullet>;

//---------------------------------------------------------------------------------------
// TOOLS
//---------------------------------------------------------------------------------------

struct grid
{
    struct info
    {
        xecs::component::entity m_Entity;
        xcore::vector2          m_Pos;
        bool                    m_isBullet;
    };

    struct cell
    {
        std::vector<info> m_EntityList;
    };

    constexpr static int cell_width_v  = 30;
    constexpr static int cell_height_v = 30;

    void Initialize()
    {
        m_CellXCount = s_Game.m_W / cell_width_v  + 1;
        m_CellYCount = s_Game.m_H / cell_height_v + 1;
        m_CellCount  = m_CellXCount * m_CellYCount;
        m_Cells = std::make_unique<cell[]>(m_CellCount);
    }

    void Clear()
    {
        for (auto& E : std::span{ m_Cells.get(), (std::size_t)m_CellCount }) 
            E.m_EntityList.clear();
    }

    std::tuple<int,int> ComputeFromWorldToGrid( xcore::vector2 Position ) noexcept
    {
        return
        { static_cast<int>(std::max(0.0f, std::min(Position.m_X / cell_width_v,  (float)m_CellXCount-1)))
        , static_cast<int>(std::max(0.0f, std::min(Position.m_Y / cell_height_v, (float)m_CellYCount-1)))
        };
    }

    void Insert( xecs::component::entity Entity, xcore::vector2 Position, bool isBullet ) noexcept
    {
        const auto[X,Y] = ComputeFromWorldToGrid(Position);
        const int Index = X + Y * m_CellXCount;
        assert(Index>=0);
        assert(Index < m_CellCount );
        assert(Entity.isZombie() == false);
        m_Cells[Index].m_EntityList.push_back( info
        {
            .m_Entity   = Entity
        ,   .m_Pos      = Position
        ,   .m_isBullet = isBullet
        });
    }

    std::tuple<int,int> findEntity(xecs::component::entity Entity, xcore::vector2 Position )
    {
        const auto [X, Y] = ComputeFromWorldToGrid(Position);
        const int Index = X + Y * m_CellXCount;
        assert(Index >= 0);
        assert(Index < m_CellCount);
        auto& Cell = m_Cells[Index].m_EntityList;
        for (int i = 0; i < Cell.size(); ++i)
        {
            auto& Entry = Cell[i];
            if (Entry.m_Entity.m_GlobalIndex == Entity.m_GlobalIndex)
            {
                return {Index, i};
            }
        }
        return { Index,-1};
    }

    void RemoveEntity( xecs::component::entity Entity, xcore::vector2 Position )
    {
        auto [Index,i] = findEntity(Entity, Position);
        assert(i>=0);
        auto& Cell = m_Cells[Index].m_EntityList;
        Cell[i] = Cell.back();
        Cell.pop_back();
    }

    void DrawGrid()
    {
        glBegin(GL_LINES);
        glColor3f(0.5, 0.5, 0.5);

        for(int y = 0; y<=m_CellYCount; ++y  )
        {
            glVertex2i( 0, y* cell_height_v);
            glVertex2i(m_CellXCount* cell_width_v, y* cell_height_v);
        }

        for (int x = 0; x <= m_CellXCount; x ++ )
        {
            glVertex2i(x* cell_width_v, 0);
            glVertex2i(x* cell_width_v, m_CellYCount * cell_height_v);
        }

        glEnd();
    }

    template<typename T_FUNCTION>
    void Search( xcore::vector2 Position, T_FUNCTION&& Function ) noexcept
    {
        const auto [X, Y] = ComputeFromWorldToGrid(Position);
        for (int y = std::max(0, Y - 1), endY = std::min(Y + 1,  m_CellYCount); y < endY; ++y)
            for (int x = std::max(0,X-1), endX = std::min(X + 1, m_CellXCount); x < endX; ++x)
            {
                int Index  = x + y * m_CellXCount;
                assert(Index< m_CellCount);
                assert(Index>=0);
                auto& Cell = m_Cells[Index].m_EntityList;
                for( int i=0; i< Cell.size(); ++i)
                {
                    auto& Entry = Cell[i];
                    bool  Break = Function(Entry);
                    if( Entry.m_Entity.isZombie() )
                    {
                        Entry = Cell.back();
                        Cell.pop_back();
                        --i;
                    }
                    if( Break ) return;
                }
            }
    }

    std::unique_ptr<cell[]> m_Cells{};
    int                     m_CellXCount;
    int                     m_CellYCount;
    int                     m_CellCount;
};

//---------------------------------------------------------------------------------------
// SYSTEMS
//---------------------------------------------------------------------------------------

struct update_movement : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::update
    {
        .m_pName = "update_movement"
    };

    grid            m_Grid;

    void OnGameStart()
    {
        m_Grid.Initialize();
    }

    void OnFrameStart()
    {
        m_Grid.Clear();
        m_Grid.DrawGrid();
    }

    void operator()( entity* pEntity, position& Position, velocity& Velocity, bullet* pBullet ) noexcept
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

        m_Grid.Insert( *pEntity, Position.m_Value, !!pBullet );
    }
};

//---------------------------------------------------------------------------------------

struct update_timer : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::update
    {
        .m_pName = "update_timer"
    };

    void operator()( entity& Entity, timer& Timer ) noexcept
    {
        Timer.m_Value -= 0.01f;
        if( Timer.m_Value <= 0 )
        {
            (void)m_GameMgr.AddOrRemoveComponents<std::tuple<>, std::tuple<timer>>(Entity);
        }
    }
};

//---------------------------------------------------------------------------------------

struct bullet_logic : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::update
    {
        .m_pName = "bullet_logic"
    };

    grid* m_pGrid{};

    void OnGameStart()
    {
        m_pGrid = &m_GameMgr.getSystem<update_movement>().m_Grid;
    }

    void operator()( entity& Entity, position& Position, bullet& Bullet ) const noexcept
    {
        // If I am dead because some other bullet killed me then there is nothing for me to do...
        if (Entity.isZombie()) return;

        // Check for collisions
        m_pGrid->Search( Position.m_Value, [&]( auto& CellEntry ) -> bool
        {
            assert(CellEntry.m_Entity.isZombie() == false );

            // Our we checking against my self?
            if ( Entity == CellEntry.m_Entity ) return false;

            // Are we colliding with our own ship?
            // If so lets just continue
            if( Bullet.m_ShipOwner.m_Value == CellEntry.m_Entity.m_Value ) return false;

            constexpr auto distance_v = 3;
            if ((CellEntry.m_Pos - Position.m_Value).getLengthSquared() < distance_v * distance_v)
            {
                m_GameMgr.DeleteEntity(Entity);
                m_GameMgr.DeleteEntity(CellEntry.m_Entity);
                return true;
            }

            return false;
        });

        if(Entity.isZombie()) m_pGrid->RemoveEntity(Entity, Position.m_Value);
    }
};

//---------------------------------------------------------------------------------------

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

    void operator()(entity& Entity) noexcept
    {
        m_GameMgr.DeleteEntity(Entity);
    }
};

//---------------------------------------------------------------------------------------

struct space_ship_logic : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::update
    {
        .m_pName = "space_ship_logic"
    };

    grid*                       m_pGrid             {};
    xecs::archetype::instance*  m_pBulletArchetype  {};

    void OnGameStart()
    {
        m_pGrid             = &m_GameMgr.getSystem<update_movement>().m_Grid;
        m_pBulletArchetype  = &m_GameMgr.getOrCreateArchetype<bullet_tuple>();
    }

    using query = std::tuple
    <
        xecs::query::none_of<bullet, timer>
    >;

    void operator()( entity& Entity, position& Position ) const noexcept
    {
        // Check for collisions
        m_pGrid->Search( Position.m_Value, [&]( auto& CellEntry ) -> bool
        {
            // Don't shoot myself, or try to shoot another bullet
            if ( Entity == CellEntry.m_Entity || CellEntry.m_isBullet ) return false;

            auto        Direction        = CellEntry.m_Pos - Position.m_Value;
            const auto  DistanceSquare   = Direction.getLengthSquared();

            // Shoot a bullet if close enough
            constexpr auto min_distance_v = 60;
            if( DistanceSquare < min_distance_v*min_distance_v )
            {
                auto NewEntity = m_GameMgr.AddOrRemoveComponents<std::tuple<timer>>( Entity, [&]( timer& Timer )
                {
                    Timer.m_Value = 8;
                });
                // After moving the entity all access to its components via the function existing parameters is consider a bug
                // Since the entity has moved to a different archetype
                assert( Entity.isZombie() );

                // Hopefully there is not system that intersects me and kills me
                assert( !NewEntity.isZombie() );


                m_pBulletArchetype->CreateEntity([&]( position& Pos, velocity& Vel, bullet& Bullet, timer& Timer ) noexcept
                    {
                        Direction  /= std::sqrt(DistanceSquare);
                        Vel.m_Value = Direction * 2.0f;
                        Pos.m_Value = Position.m_Value + Vel.m_Value;

                        Bullet.m_ShipOwner = NewEntity;

                        Timer.m_Value      = 10;
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
    constexpr static auto typedef_v = xecs::system::type::update
    {
        .m_pName = "render_bullets"
    };

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
    constexpr static auto typedef_v = xecs::system::type::update
    {
        .m_pName = "render_ships"
    };

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

template< typename... T_ARGS>
void GlutPrint(int x, int y, const char* pFmt, T_ARGS&&... Args) noexcept
{
    std::array<char, 256> FinalString;
    const auto len = sprintf_s(FinalString.data(), FinalString.size(), pFmt, Args... );

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, s_Game.m_W, 0, s_Game.m_H);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glRasterPos2i(x, s_Game.m_H - (y + 1) * 20);
    for (int i = 0; i < len; ++i)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, FinalString[i]);
    }
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

//---------------------------------------------------------------------------------------

struct on_destroy_count_dead_ships : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::notify_destroy
    {
        .m_pName = "on_destroy_count_dead_ships"
    };

    using query = std::tuple
    <
        xecs::query::none_of<bullet>
    ,   xecs::query::one_of<entity>
    ,   xecs::query::must<position>
    >;

    void operator()( timer* pTimer ) noexcept
    {
        if(pTimer) s_Game.m_nEntityWaitingDead++;
        else       s_Game.m_nEntityThinkingDead++;
    }
};

//---------------------------------------------------------------------------------------

struct page_flip : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::update
    {
        .m_pName = "page_flip"
    };

    __inline
    void OnUpdate( void ) noexcept
    {
        glFlush();
        glutSwapBuffers();
        glClear(GL_COLOR_BUFFER_BIT);
    }
};

//---------------------------------------------------------------------------------------

struct example
{
    constexpr static auto typedef_v = xecs::component::type::share
    {
        .m_pName        = "Example of a share component"
    };

    int m_Int;
};

//---------------------------------------------------------------------------------------

void InitializeGame( void ) noexcept
{
    //
    // Initialize global elements
    //
    std::srand(100);

    //
    // Register Components (They should always be first)
    //
    s_Game.m_GameMgr->RegisterComponents
    <   position
    ,   velocity
    ,   timer
    ,   bullet
    >();

    //
    // Register Systems
    //

    // Register updated systems (the update system should be before the delegate systems)
    s_Game.m_GameMgr->RegisterSystems
    <   update_timer            // Structural: Yes, RemoveComponent(Timer)
    ,   update_movement         // Structural: No
    ,   bullet_logic            // Structural: Yes, Destroy(Bullets || Ships)
    ,   space_ship_logic        // Structural: Yes, AddShipComponent(Timer), Create(Bullets)
    ,   render_ships            // Structural: No
    ,   render_bullets          // Structural: No
    ,   page_flip               // Structural: No
    >();

    // Register notifiers. Note that the order for this system are not related with the update systems
    // Since these are event base they can happen at any time. But if many of them are going to get the
    // same message they will get them in the order that are here.
    // This is why I create a separate section for these even thought they still are systems.
    s_Game.m_GameMgr->RegisterSystems
    <   on_destroy_count_dead_ships     // Structural: No
    ,   destroy_bullet_on_remove_timer  // Structural: Yes (Deletes bullets entities when timer is removed)
    >();

    //
    // Generate a few random ships
    //
    static_assert( false == xcore::types::tuple_has_duplicates_v<std::tuple<position& , velocity& , timer& >>);

    s_Game.m_GameMgr->getOrCreateArchetype< position, velocity, timer >()
        .CreateEntities( 10000, [&]( position& Position, velocity& Velocity, timer& Timer ) noexcept
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

