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
    constexpr static auto typedef_v = xecs::component::type::data {};
    xcore::vector2 m_Value;
};

struct velocity
{
    constexpr static auto typedef_v = xecs::component::type::data {};
    xcore::vector2 m_Value;
};

struct timer
{
    constexpr static auto typedef_v = xecs::component::type::data {};

    float m_Value;
};

struct bullet
{
    constexpr static auto typedef_v = xecs::component::type::data {};

    xecs::component::entity m_ShipOwner;
};

struct grid_cell
{
    constexpr static auto typedef_v = xecs::component::type::share
    {
        .m_bBuildFilter = true
    };

    std::int16_t m_X;
    std::int16_t m_Y;
};

using bullet_tuple = std::tuple<position, velocity, timer, bullet, grid_cell>;

//---------------------------------------------------------------------------------------
// TOOLS
//--------------------------------------------------\/-----------------------------------
//                                                  //
//       +----+----+----+----+----+----+----+----+  //
//       |  -1,-1  |   0,-1  |   1,-1  |    :    |  //
//  +---------+---------+---------+---------+----+  //
//  |  -1,0   |   0,0   |   1,0   |   2,0   |       //
//  +----+----+----+----+----+----+----+----+----+  //
//       |  -1,1   |   0,1   |   1,1   |    :    |  //
//  +----+----+----+----+----+----+----+----+----+  //
//  |  -1,2   |   0,2   |   1,2   |   2,2   |       //
//  +----+----+----+----+----+----+----+----+----+  //
//       |  -1,3   |   0,3   |   1,3   |    :    |  //
//       +----+----+----+----+----+----+----+----+  //
//                                                  //
//--------------------------------------------------/\-----------------------------------

namespace grid
{
    constexpr int   cell_width_v               = 64; // Keep this divisible by 2
    constexpr int   cell_height_v              = 42; // Keep this divisible by 2
    constexpr int   max_resolution_width_v     = 1024;
    constexpr int   max_resolution_height_v    = 800;
    constexpr auto  cell_x_count               = static_cast<std::int16_t>(max_resolution_width_v /cell_width_v  + 1);
    constexpr auto  cell_y_count               = static_cast<std::int16_t>(max_resolution_height_v/cell_height_v + 1);

    //---------------------------------------------------------------------------------------

    template<typename T_FUNCTION>
    constexpr __inline
    bool Search
    ( xecs::system::instance&               System
    , const xecs::component::share_filter&  ShareFilter
    , const std::int16_t                    X
    , const std::int16_t                    Y
    , const xecs::query::instance&          Query
    , T_FUNCTION&&                          Function 
    ) noexcept
    {
        static constexpr auto Table = std::array< std::int16_t, 2*6 >
        { -1, 0
        , -1, 1
        , -1, 0
        ,  0, 1
        , -1, 1
        ,  0, 1
        };

        //
        // Search self first 
        //
        if( System.Foreach(ShareFilter, Query, std::forward<T_FUNCTION&&>(Function)) )
            return true;

        //
        // Search neighbors 
        //
        int i = (Y&1)*(2*3);
        for( std::int16_t y = std::max(0,Y-1), end_y = std::min(cell_y_count-1, Y+1); y != end_y; ++y )
        {
            if( auto pShareFilter = System.findShareFilter(grid_cell{ .m_X = X + Table[i + 0], .m_Y = y }); pShareFilter && System.Foreach
                    ( *pShareFilter
                    , Query
                    , std::forward<T_FUNCTION&&>(Function)
                    )
                )
                return true;

            if( auto pShareFilter = System.findShareFilter(grid_cell{ .m_X = X + Table[i + 1], .m_Y = y }); pShareFilter && System.Foreach
                    ( *pShareFilter
                    , Query
                    , std::forward<T_FUNCTION&&>(Function)
                    )
                )
                return true;

            i+=2;
        }
        return false;
    }

    //---------------------------------------------------------------------------------------

    __inline constexpr
    grid_cell ComputeGridCellFromWorldPosition( const xcore::vector2 Position ) noexcept
    {
        const auto X = static_cast<int>(Position.m_X / (cell_width_v /2.0f));
        const auto Y = std::max(0, std::min(static_cast<int>(Position.m_Y / cell_height_v), cell_y_count - 1));
        const auto x = 1 & ((X ^ Y) & Y);
        return
        { static_cast<std::int16_t>(std::max(0, std::min( 1 + ((X - x) >> 1), cell_x_count - 1 )))
        , static_cast<std::int16_t>(Y)
        };
    }
}

//---------------------------------------------------------------------------------------
// SYSTEMS
//---------------------------------------------------------------------------------------

struct update_movement : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::update
    {
        .m_pName = "update_movement"
    };

    __inline 
    void operator()( position& Position, velocity& Velocity, grid_cell& GridCell ) const noexcept
    {
        Position.m_Value += Velocity.m_Value;

        // Bounce on edges
        if (Position.m_Value.m_X < 0)
        {
            Position.m_Value.m_X = 0;
            Velocity.m_Value.m_X = -Velocity.m_Value.m_X;
        }
        else if (Position.m_Value.m_X >= grid::max_resolution_width_v )
        {
            Position.m_Value.m_X = grid::max_resolution_width_v - 1;
            Velocity.m_Value.m_X = -Velocity.m_Value.m_X;
        }

        if (Position.m_Value.m_Y < 0)
        {
            Position.m_Value.m_Y = 0;
            Velocity.m_Value.m_Y = -Velocity.m_Value.m_Y;
        }
        else if (Position.m_Value.m_Y >= grid::max_resolution_height_v )
        {
            Position.m_Value.m_Y = grid::max_resolution_height_v - 1;
            Velocity.m_Value.m_Y = -Velocity.m_Value.m_Y;
        }

        // Update the grid cell base on our new position
        GridCell = grid::ComputeGridCellFromWorldPosition(Position.m_Value);
    }
};

//---------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------

struct bullet_logic : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::update
    {
        .m_pName = "bullet_logic"
    };

    xecs::query::instance m_QueryBullets;
    xecs::query::instance m_QueryAny;

    using query = std::tuple
    <
        xecs::query::must<bullet>
    >;

    void OnGameStart( void ) noexcept
    {
        m_QueryBullets.AddQueryFromTuple<query>();
        m_QueryAny.m_Must.AddFromComponents<position>();
    }

    __inline 
    void OnUpdate( void ) noexcept
    {
        //
        // Update all the bullets
        //
        for( std::int16_t Y=0; Y<grid::cell_y_count; ++Y )
        for( std::int16_t X=0; X<grid::cell_x_count; ++X )
        {
            auto pShareFilter = findShareFilter(grid_cell{ .m_X = X, .m_Y = Y });
            if( pShareFilter == nullptr ) continue;

            Foreach( *pShareFilter, m_QueryBullets, [&]( entity& Entity, const position& Position, const bullet& Bullet ) constexpr noexcept
            {
                // If I am dead because some other bullet killed me then there is nothing for me to do...
                if (Entity.isZombie()) return;

                grid::Search( *this, *pShareFilter, X, Y, m_QueryAny, [&]( entity& E, const position& Pos )  constexpr noexcept
                {
                    if (E.isZombie()) return false;

                    // Our we checking against my self?
                    if ( Entity == E ) return false;

                    // Are we colliding with our own ship?
                    // If so lets just continue
                    if( Bullet.m_ShipOwner == E ) return false;

                    if (constexpr auto distance_v = 3; (Pos.m_Value - Position.m_Value).getLengthSquared() < distance_v * distance_v)
                    {
                        DeleteEntity(Entity);
                        DeleteEntity(E);
                        return true;
                    }

                    return false;
                });
            });
        }
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

    __inline
    void operator()( entity& Entity ) const noexcept
    {
        DeleteEntity(Entity);
    }
};

//---------------------------------------------------------------------------------------

struct space_ship_logic : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::update
    {
        .m_pName = "space_ship_logic"
    };

    xecs::archetype::instance*  m_pBulletArchetype          {};
    xecs::query::instance       m_QueryThinkingShipsOnly    {};
    xecs::query::instance       m_QueryAnyShips             {};

    using query = std::tuple
    <
        xecs::query::must<position>
    ,   xecs::query::none_of<bullet, timer>
    >;

    void OnGameStart( void ) noexcept
    {
        m_pBulletArchetype  = &getOrCreateArchetype<bullet_tuple>();
        m_QueryThinkingShipsOnly.AddQueryFromTuple<query>();

        m_QueryAnyShips.m_Must.AddFromComponents<position>();
        m_QueryAnyShips.m_NoneOf.AddFromComponents<bullet>();
    }

    __inline
    void OnUpdate( void ) noexcept
    {
        for( std::int16_t Y=0; Y<grid::cell_y_count; ++Y )
        for( std::int16_t X=0; X<grid::cell_x_count; ++X )
        {
            auto pShareFilter = findShareFilter(grid_cell{ .m_X = X, .m_Y = Y });
            if( nullptr == pShareFilter ) continue;

            Foreach( *pShareFilter, m_QueryThinkingShipsOnly, [&]( entity& Entity, const position& Position ) constexpr noexcept
            {
                grid::Search( *this, *pShareFilter, X, Y, m_QueryAnyShips, [&]( const entity& E, const position& Pos ) constexpr noexcept
                {
                    // Don't shoot myself
                    if ( Entity == E ) return false;

                    auto        Direction        = Pos.m_Value - Position.m_Value;
                    const auto  DistanceSquare   = Direction.getLengthSquared();

                    // Shoot a bullet if close enough
                    if(constexpr auto min_distance_v = 60; DistanceSquare < min_distance_v*min_distance_v )
                    {
                        auto NewEntity = AddOrRemoveComponents<std::tuple<timer>>( Entity, [&]( timer& Timer )
                        {
                            Timer.m_Value = 8;
                        });

                        // After moving the entity all access to its components via the function existing parameters is consider a bug
                        // Since the entity has moved to a different archetype
                        assert( Entity.isZombie() );

                        // Hopefully there is not system that intersects me and kills me
                        assert( !NewEntity.isZombie() );

                        m_pBulletArchetype->CreateEntity( [&]( position& Pos, velocity& Vel, bullet& Bullet, timer& Timer, grid_cell& Cell) noexcept
                        {
                            Direction  /= std::sqrt(DistanceSquare);
                            Vel.m_Value = Direction * 2.0f;
                            Pos.m_Value = Position.m_Value + Vel.m_Value;

                            Bullet.m_ShipOwner = NewEntity;

                            Cell = grid::ComputeGridCellFromWorldPosition(Pos.m_Value);

                            Timer.m_Value      = 10;
                        });

                        return true;
                    }

                    return false;
                });
            });
        }
    }
};

//---------------------------------------------------------------------------------------

struct renderer : xecs::system::instance
{
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

        //
        // Let all the system that depends on me
        //
        SendEventFrom<update>(this);

        //
        // Page Flip
        //
//        glFlush();
        glutSwapBuffers();
    }
};

//---------------------------------------------------------------------------------------

template< typename... T_ARGS>
void GlutPrint( const int x, const int y, const char* const pFmt, T_ARGS&&... Args) noexcept
{
    std::array<char, 256> FinalString;
    const auto len = sprintf_s(FinalString.data(), FinalString.size(), pFmt, Args...);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, s_Game.m_W, 0, s_Game.m_H);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glRasterPos2i(x, s_Game.m_H - (y + 20));
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

//---------------------------------------------------------------------------------------

struct render_ships : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::child_update<renderer, renderer::update>
    {
        .m_pName = "render_ships"
    };

    using query = std::tuple
    <
        xecs::query::none_of<bullet>
    ,   xecs::query::one_of<entity>
    >;

    void OnPreUpdate( void ) noexcept
    {
        glBegin(GL_QUADS);
    }

    void OnPostUpdate( void ) noexcept
    {
        glEnd();
    }

    __inline
    void operator()( const position& Position, const timer* pTimer ) const noexcept
    {
        constexpr auto Size = 3;
        if(pTimer) glColor3f(1.0, 1.0, 1.0);
        else       glColor3f(0.5, 1.0, 0.5);
        glVertex2i(Position.m_Value.m_X - Size, Position.m_Value.m_Y - Size);
        glVertex2i(Position.m_Value.m_X - Size, Position.m_Value.m_Y + Size);
        glVertex2i(Position.m_Value.m_X + Size, Position.m_Value.m_Y + Size);
        glVertex2i(Position.m_Value.m_X + Size, Position.m_Value.m_Y - Size);
    }
};

//---------------------------------------------------------------------------------------

struct render_grid : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::child_update<renderer, renderer::update>
    {
        .m_pName = "render_grid"
    };

    __inline constexpr
    void OnUpdate( void ) noexcept
    {
        for( std::int16_t y=0; y<grid::cell_y_count; y++ )
        for( std::int16_t x=0; x<grid::cell_x_count; x++ )
        {
            auto pGridCell = findShareFilter( grid_cell{ .m_X = x, .m_Y = y } );
            if(nullptr == pGridCell) continue;

            // If we don't have any archetypes then move on
            int Count = static_cast<int>(pGridCell->m_lEntries.size());
            if( 0 == Count) continue;

            // Hide nodes where there are not entities
            if constexpr (false)
            {
                int nEntities = 0;
                for (auto& ArchetypeCell : pGridCell->m_lEntries)
                    for (auto& Family : ArchetypeCell.m_lFamilies)
                        nEntities += static_cast<int>(Family->m_DefaultPool.Size());
                if(nEntities == 0 ) continue;
            }
            
            const float X = ((x-1) + 0.5f + (y & 1) * 0.5f) * grid::cell_width_v;
            const float Y = (y + 0.5f                     ) * grid::cell_height_v;
            constexpr auto SizeX = grid::cell_width_v  / 2.0f - 1;
            constexpr auto SizeY = grid::cell_height_v / 2.0f - 1;
            
            glBegin(GL_QUADS);
            glColor3f(0.25f, 0.25f, 0.25f);
            glVertex2i(X - SizeX, Y - SizeY);
            glVertex2i(X - SizeX, Y + SizeY);
            glVertex2i(X + SizeX, Y + SizeY);
            glVertex2i(X + SizeX, Y - SizeY);
            glEnd();

            enum print
            {
                NONE
            ,   FAMILIES
            ,   ARCHETYPES
            ,   ENTITIES
            ,   GRIDCELL_XY
            };

            // What are we printing?
            switch( print::NONE)
            {
                case print::ARCHETYPES: 
                {
                    glColor3f(1.0f, 1.0f, 1.0f);
                    GlutPrint(X, Y - 15, "%d", Count);
                    break;
                }
                case print::FAMILIES:
                {
                    int nFamilies = 0;
                    for (auto& ArchetypeCell : pGridCell->m_lEntries)
                        nFamilies += static_cast<int>(ArchetypeCell.m_lFamilies.size());

                    glColor3f(1.0f, 1.0f, 1.0f);
                    GlutPrint(X, Y - 15, "%d", nFamilies);
                    break;
                }
                case print::ENTITIES:
                {
                    int nEntities = 0;
                    for (auto& ArchetypeCell : pGridCell->m_lEntries)
                        for (auto& Family : ArchetypeCell.m_lFamilies)
                            nEntities += static_cast<int>(Family->m_DefaultPool.Size());

                    glColor3f(1.0f, 1.0f, 1.0f);
                    GlutPrint(X, Y - 15, "%d", nEntities);
                    break;
                }
                case print::GRIDCELL_XY:
                {
                    auto& C = pGridCell->m_lEntries[0].m_pArchetype->getShareComponent<grid_cell>(*pGridCell->m_lEntries[0].m_lFamilies[0]);
                    glColor3f(1.0f, 1.0f, 1.0f);
                    GlutPrint(X-23, Y - 15, "%d,%d", C.m_X, C.m_Y );
                    break;
                }
            }
        }

        //
        // Print how many archetypes we have so far
        //
        if constexpr (false)
        {
            glColor3f(1.0f, 1.0f, 1.0f);
            GlutPrint(0, 0, "#Archetypes: %d", s_Game.m_GameMgr->m_ArchetypeMgr.m_lArchetype.size());
        }
    }
};

//---------------------------------------------------------------------------------------

void InitializeGame( void ) noexcept
{
    //
    // Initialize global elements
    //
    std::srand(101);

    //
    // Register Components (They should always be first)
    //
    s_Game.m_GameMgr->RegisterComponents
    <   position
    ,   velocity
    ,   timer
    ,   bullet
    ,   grid_cell
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
    ,   renderer                // Structural: No
    ,       render_grid         // Structural: No
    ,       render_ships        // Structural: No
    ,       render_bullets      // Structural: No
    >();


    // Register Reactive Systems. Note that the order for this system are not related with the update systems
    // Since these are event base they can happen at any time. But if many of them are going to get the
    // same message they will get them in the order that are here.
    // This is why I create a separate section for these even thought they still are systems.
    s_Game.m_GameMgr->RegisterSystems
    <   destroy_bullet_on_remove_timer  // Structural: Yes (Deletes bullets entities when timer is removed)
    >();

    //
    // Generate a few random ships
    //
    s_Game.m_GameMgr->getOrCreateArchetype< position, velocity, timer, grid_cell>()
        .CreateEntities( 20, [&]( position& Position, velocity& Velocity, timer& Timer, grid_cell& Cell ) noexcept
        {
            Position.m_Value     = xcore::vector2{ static_cast<float>(std::rand() % s_Game.m_W)
                                                 , static_cast<float>(std::rand() % s_Game.m_H)
                                                 };

            Cell = grid::ComputeGridCellFromWorldPosition(Position.m_Value);

            Velocity.m_Value.m_X = std::rand() / static_cast<float>(RAND_MAX) - 0.5f;
            Velocity.m_Value.m_Y = std::rand() / static_cast<float>(RAND_MAX) - 0.5f;
            Velocity.m_Value.Normalize();

            Timer.m_Value        = std::rand() / static_cast<float>(RAND_MAX) * 8;
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
    glDisable(GL_DEPTH_TEST);
    glutMainLoop();

    xcore::Kill();
    return 0;
}

