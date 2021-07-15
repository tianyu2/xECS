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
    constexpr static auto typedef_v = xecs::component::type::share {};

    std::uint16_t m_X;
    std::uint16_t m_Y;
};

using bullet_tuple = std::tuple<position, velocity, timer, bullet, grid_cell>;

//---------------------------------------------------------------------------------------
// TOOLS
//---------------------------------------------------------------------------------------

namespace grid
{
    constexpr static int cell_width_v               = 30;
    constexpr static int cell_height_v              = 30;
    constexpr static int max_resolution_width_v     = 1280;
    constexpr static int max_resolution_height_v    = 1024;
    constexpr static int cell_x_count               = max_resolution_width_v /cell_width_v  + 1;
    constexpr static int cell_y_count               = max_resolution_height_v/cell_height_v + 1;

    struct archetype_entry
    {
        xecs::archetype::instance*          m_pArchetype;
        std::vector<xecs::pool::family*>    m_ListOfFamilies;
    };

    using instance = std::array<std::array<std::vector<archetype_entry>,cell_x_count>,cell_y_count>;

    //---------------------------------------------------------------------------------------

    template<typename T_FUNCTION>
    constexpr __inline
    bool Foreach( instance& Grid, int X, int Y, const xecs::query::instance& Query, T_FUNCTION&& Function ) noexcept
    {
        xecs::query::iterator<T_FUNCTION> Iterator(*s_Game.m_GameMgr);
        for( auto& ArchetypeCell : Grid[Y][X] )
        {
            // Make sure this archetype matches are query
            if( Query.Compare(ArchetypeCell.m_pArchetype->getComponentBits()) == false )
                continue;

            Iterator.UpdateArchetype(*ArchetypeCell.m_pArchetype);

            for( auto F : ArchetypeCell.m_ListOfFamilies )
            {
                Iterator.UpdateFamilyPool( *F );
                for( auto p = &F->m_DefaultPool; p ; p = p->m_Next.get() )
                {
                    if( p->Size() )
                    {
                        Iterator.UpdatePool(*p);

                        // Do all entities
                        if constexpr (xecs::tools::function_return_v<T_FUNCTION, bool>)
                            return Iterator.ForeachEntity( std::forward<T_FUNCTION&&>(Function));
                        else
                            Iterator.ForeachEntity(std::forward<T_FUNCTION&&>(Function));
                    }
                }
            }
        }
        return false;
    }

    //---------------------------------------------------------------------------------------

    template<typename T_FUNCTION>
    constexpr __inline
    bool Search( instance& Grid, int X, int Y, xecs::query::instance& Query, T_FUNCTION&& Function ) noexcept
    {
        const auto XStart = std::max(0, X - 1);
        const auto XEnd   = std::min(cell_x_count - 1, X + 1);
        for( int y = std::max(0,Y-1), end_y = std::min(cell_y_count-1, Y+1); y != end_y; ++y )
            for (int x = XStart; x != XEnd; ++x)
            {
                if( Foreach( Grid, x,y, Query, std::forward<T_FUNCTION&&>(Function) ) ) 
                    return true;
            }
        return false;
    }

    //---------------------------------------------------------------------------------------

    constexpr __inline
    grid_cell ComputeGridCellFromWorldPosition( xcore::vector2 Position) noexcept
    {
        return
        { static_cast<std::uint16_t>(std::max(0.0f, std::min(Position.m_X / cell_width_v,  (float)cell_x_count - 1)))
        , static_cast<std::uint16_t>(std::max(0.0f, std::min(Position.m_Y / cell_height_v, (float)cell_y_count - 1)))
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

        // Update the grid cell base on our new position
        GridCell = grid::ComputeGridCellFromWorldPosition(Position.m_Value);
    }
};

//---------------------------------------------------------------------------------------

struct grid_system_pool_family_create : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::pool_family_create
    {
        .m_pName = "grid_system_pool_family_create"
    };

    using query = std::tuple
    <
        xecs::query::must<position, grid_cell>
    >;

    std::shared_ptr<grid::instance> m_Grid = std::make_shared<grid::instance>();

    __inline
    void OnPoolFamily( xecs::archetype::instance& Archetype, xecs::pool::family& PoolFamily ) const noexcept
    {
        assert(PoolFamily.m_ShareInfos.size() == 1);

        auto& Cell      = Archetype.getShareComponent<grid_cell>(PoolFamily);
        auto& GridCell  = (*m_Grid)[Cell.m_Y][Cell.m_X];
        for( auto& E : GridCell )
        {
            if(E.m_pArchetype == &Archetype)
            {
                E.m_ListOfFamilies.push_back(&PoolFamily);
                return;
            }
        }
        
        // We made it here so it means we did not find the archetype
        // So lets create a new entry for our new archetype
        GridCell.emplace_back();
        GridCell.back().m_pArchetype = &Archetype;
        GridCell.back().m_ListOfFamilies.push_back(&PoolFamily);
    }
};

//---------------------------------------------------------------------------------------

struct update_timer : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::update
    {
        .m_pName = "update_timer"
    };

    __inline
    void operator()( entity& Entity, timer& Timer ) const noexcept
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

    grid::instance* m_pGrid;

    void OnGameStart()
    {
        m_pGrid = m_GameMgr.getSystem<grid_system_pool_family_create>().m_Grid.get();
    }

    using query = std::tuple
    <
        xecs::query::must<bullet>
    >;

    __inline
    void OnUpdate() noexcept
    {
        xecs::query::instance QueryBullets;
        xecs::query::instance QueryAny;

        QueryBullets.AddQueryFromTuple<query>();
        QueryAny.m_Must.AddFromComponents<position>();

        //
        // Update all the bullets
        //
        for( int Y=0; Y<grid::cell_y_count; ++Y )
        for( int X=0; X<grid::cell_x_count; ++X )
        {
            grid::Foreach( *m_pGrid, X, Y, QueryBullets, [&]( entity& Entity, position& Position, bullet& Bullet ) constexpr noexcept
            {
                // If I am dead because some other bullet killed me then there is nothing for me to do...
                if (Entity.isZombie()) return;

                grid::Search( *m_pGrid, X, Y, QueryAny, [&]( entity& E, position& Pos )  constexpr noexcept
                {
                    if (E.isZombie()) return false;

                    // Our we checking against my self?
                    if ( Entity == E ) return false;

                    // Are we colliding with our own ship?
                    // If so lets just continue
                    if( Bullet.m_ShipOwner == E ) return false;

                    constexpr auto distance_v = 3;
                    if ((Pos.m_Value - Position.m_Value).getLengthSquared() < distance_v * distance_v)
                    {
                        m_GameMgr.DeleteEntity(Entity);
                        m_GameMgr.DeleteEntity(E);
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
    void operator()(entity& Entity) const noexcept
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

    xecs::archetype::instance*  m_pBulletArchetype  {};
    grid::instance*             m_pGrid             {};

    void OnGameStart()
    {
        m_pBulletArchetype  = &m_GameMgr.getOrCreateArchetype<bullet_tuple>();
        m_pGrid             = m_GameMgr.getSystem<grid_system_pool_family_create>().m_Grid.get();
    }

    using query = std::tuple
    <
        xecs::query::must<position>
    ,   xecs::query::none_of<bullet, timer>
    >;

    __inline
    void OnUpdate() noexcept
    {
        xecs::query::instance QueryThinkingShipsOnly;
        xecs::query::instance QueryAnyShips;

        QueryThinkingShipsOnly.AddQueryFromTuple<query>();

        QueryAnyShips.m_Must.AddFromComponents<position>();
        QueryAnyShips.m_NoneOf.AddFromComponents<bullet>();

        for( int Y=0; Y<grid::cell_y_count; ++Y )
        for( int X=0; X<grid::cell_x_count; ++X )
        {
            grid::Foreach( *m_pGrid, X, Y, QueryThinkingShipsOnly, [&]( entity& Entity, position& Position ) constexpr noexcept
            {
                grid::Search( *m_pGrid, X, Y, QueryAnyShips, [&]( entity& E, position& Pos ) constexpr noexcept
                {
                    // Don't shoot myself
                    if ( Entity == E ) return false;

                    auto        Direction        = Pos.m_Value - Position.m_Value;
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

                        m_pBulletArchetype->CreateEntity( [&]( position& Pos, velocity& Vel, bullet& Bullet, timer& Timer, grid_cell& Cell) noexcept
                        {
                            Direction  /= std::sqrt(DistanceSquare);
                            Vel.m_Value = Direction * 2.0f;
                            Pos.m_Value = Position.m_Value + Vel.m_Value;

                            Bullet.m_ShipOwner = NewEntity;

                            Cell = grid::ComputeGridCellFromWorldPosition(Position.m_Value);

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

struct on_destroy_count_dead_ships : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::notify_destroy
    {
        .m_pName = "on_destroy_count_dead_ships"
    };

    using query = std::tuple
    < xecs::query::none_of<bullet>
    , xecs::query::one_of<entity>
    , xecs::query::must<position>
    >;

    __inline
    void operator()(timer* pTimer) const noexcept
    {
        if (pTimer) s_Game.m_nEntityWaitingDead++;
        else        s_Game.m_nEntityThinkingDead++;
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
    void OnUpdate() noexcept
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
        glFlush();
        glutSwapBuffers();
    }
};

//---------------------------------------------------------------------------------------

template< typename... T_ARGS>
void GlutPrint(int x, int y, const char* pFmt, T_ARGS&&... Args) noexcept
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

    __inline
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
    constexpr static auto typedef_v = xecs::system::type::child_update<renderer, renderer::update>
    {
        .m_pName = "render_ships"
    };

    using query = std::tuple
    <
        xecs::query::none_of<bullet>
    ,   xecs::query::one_of<entity>
    >;

    __inline
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

struct render_grid : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::child_update<renderer, renderer::update>
    {
        .m_pName = "render_grid"
    };

    grid::instance* m_pGrid;

    void OnGameStart() noexcept
    {
        m_pGrid = m_GameMgr.getSystem<grid_system_pool_family_create>().m_Grid.get();
    }

    __inline
    void OnUpdate() noexcept
    {
        for( int y=0; y<grid::cell_y_count; y++ )
        for( int x=0; x<grid::cell_x_count; x++ )
        {
            auto& GridCell = (*m_pGrid)[y][x];

            // If we don't have any archetypes then move on
            int Count = (int)(GridCell.size());
            if( 0 == Count) continue;

            float X = (x + 0.5f) * grid::cell_width_v;
            float Y = (y + 0.5f) * grid::cell_width_v;
            constexpr auto SizeX = grid::cell_width_v/2.0f - 1;
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
            };

            // What are we printing?
            switch( print::NONE )
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
                    for (auto& ArchetypeCell : GridCell)
                        nFamilies += (int)ArchetypeCell.m_ListOfFamilies.size();

                    glColor3f(1.0f, 1.0f, 1.0f);
                    GlutPrint(X, Y - 15, "%d", nFamilies);
                    break;
                }
                case print::ENTITIES:
                {
                    int nEntities = 0;
                    for (auto& ArchetypeCell : GridCell)
                        for (auto& Family : ArchetypeCell.m_ListOfFamilies)
                            nEntities += (int)Family->m_DefaultPool.Size();

                    glColor3f(1.0f, 1.0f, 1.0f);
                    GlutPrint(X, Y - 15, "%d", nEntities);
                    break;
                }
            }
        }

        //
        // Print how many archetypes we have so far
        //
        if(false)
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
    std::srand(100);

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
    <   on_destroy_count_dead_ships     // Structural: No
    ,   destroy_bullet_on_remove_timer  // Structural: Yes (Deletes bullets entities when timer is removed)
    ,   grid_system_pool_family_create  // Structutal: No
    >();

    //
    // Generate a few random ships
    //
    s_Game.m_GameMgr->getOrCreateArchetype< position, velocity, timer, grid_cell>()
        .CreateEntities( 20000, [&]( position& Position, velocity& Velocity, timer& Timer, grid_cell& Cell ) noexcept
        {
            Position.m_Value     = xcore::vector2{ static_cast<float>(std::rand() % s_Game.m_W)
                                                 , static_cast<float>(std::rand() % s_Game.m_H)
                                                 };

            Cell = grid::ComputeGridCellFromWorldPosition(Position.m_Value);

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
    glDisable(GL_DEPTH_TEST);
    glutMainLoop();

    xcore::Kill();
    return 0;
}

