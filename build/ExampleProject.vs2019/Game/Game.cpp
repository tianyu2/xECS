#include "pch.h"
#include "../cr.h"

#include "Components/_components.h"
#include "Tools/_tools.h"
#include "Systems/_systems.h"

struct my_game final : live::game
{
    std::unique_ptr<xecs::game_mgr::instance>  m_GameMgr{};
    bool                                       m_SkipFrames = true;

    //-----------------------------------------------------------------------------

    void Initialize ( live::renderer& Renderer ) noexcept
    {
        renderer::s_pLiveRenderer = &Renderer;
        m_GameMgr                 = std::make_unique<xecs::game_mgr::instance>();

        RegisterElements();
    }

    //-----------------------------------------------------------------------------

    virtual ~my_game() noexcept
    {
        m_GameMgr.reset();
    }

    //-----------------------------------------------------------------------------

    virtual void CreateDefaultScene( void ) noexcept override
    {
        //
        // Initialize global elements
        //
        std::srand(101);

        //
        // Generate a few random ships
        //
        m_GameMgr->getOrCreateArchetype< position, velocity, timer, grid_cell>()
            .CreateEntities( 20000, [&]( position& Position, velocity& Velocity, timer& Timer, grid_cell& Cell ) noexcept
            {
                Position.m_Value     = xcore::vector2{ static_cast<float>(std::rand() % renderer::s_pLiveRenderer->getWidth() )
                                                     , static_cast<float>(std::rand() % renderer::s_pLiveRenderer->getHeight())
                                                     };

                Cell = grid::ComputeGridCellFromWorldPosition(Position.m_Value);

                Velocity.m_Value.m_X = std::rand() / static_cast<float>(RAND_MAX) - 0.5f;
                Velocity.m_Value.m_Y = std::rand() / static_cast<float>(RAND_MAX) - 0.5f;
                Velocity.m_Value.Normalize();

                Timer.m_Value        = std::rand() / static_cast<float>(RAND_MAX) * 8;
            });
        
    }

    //-----------------------------------------------------------------------------

    void RegisterElements( void ) noexcept
    {
        //
        // Register Components (They should always be first)
        //
        m_GameMgr->RegisterComponents
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
        m_GameMgr->RegisterSystems
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
        m_GameMgr->RegisterSystems
        <   destroy_bullet_on_remove_timer  // Structural: Yes (Deletes bullets entities when timer is removed)
        >();
    }

    //-----------------------------------------------------------------------------

    virtual const char* SaveGameState(const char* pFileName) noexcept override
    {
        auto Error = m_GameMgr->SerializeGameState(pFileName, false, true);
        assert(!Error);
        return Error ? Error.getCodeAndClear().m_pString : nullptr;
    }

    //-----------------------------------------------------------------------------

    virtual const char* LoadGameState(const char* pFileName) noexcept override
    {
        m_GameMgr.reset();
        xecs::component::mgr::resetRegistrations();
        m_GameMgr = std::make_unique<xecs::game_mgr::instance>();
        RegisterElements();

        auto Error = m_GameMgr->SerializeGameState(pFileName, true, true);
        assert(!Error);
        return Error ? Error.getCodeAndClear().m_pString : nullptr;
    }
};

//-----------------------------------------------------------------------------

template< typename... T_ARGS>
void GlutPrint(const int x, const int y, const char* const pFmt, T_ARGS&&... Args) noexcept
{
    std::array<char, 256> FinalString;
    const auto len = sprintf_s(FinalString.data(), FinalString.size(), pFmt, Args...);
    renderer::s_pLiveRenderer->PrintAt( x, y, FinalString.data(), len );
}

//-----------------------------------------------------------------------------

CR_EXPORT int cr_main(struct cr_plugin* ctx, enum cr_op operation)
{
    assert(ctx);
    switch (operation)
    {
        case CR_LOAD:
        {
            xcore::Init("ECS Example");

            auto& LiveInfo = *static_cast<live::info*>(ctx->userdata);
            assert( nullptr == LiveInfo.m_pGame );

            auto& MyGame = *new my_game();
            MyGame.Initialize( *LiveInfo.m_pRenderer );

            if( ctx->version > 1 ) 
            {
                if( auto Error = MyGame.LoadGameState("x64/Test.bin"); Error )
                    return -1;
            }
            else
            {
                MyGame.CreateDefaultScene();
            }

            LiveInfo.m_pGame = &MyGame;

            break;
        }
        case CR_UNLOAD:
        {
            auto& LiveInfo = *static_cast<live::info*>(ctx->userdata);
            auto& MyGame   = *static_cast<my_game*>(LiveInfo.m_pGame);

            auto Error = MyGame.SaveGameState("x64/Test.bin");
               
            delete LiveInfo.m_pGame;
            LiveInfo.m_pGame = nullptr;

            xcore::Kill();

            if(Error) return -1;
            break;
        }
        case CR_CLOSE:
        {
            auto& LiveInfo = *static_cast<live::info*>(ctx->userdata);
            delete LiveInfo.m_pGame;
            LiveInfo.m_pGame = nullptr;

            xcore::Kill();
            break;
        }
        case CR_STEP:
        {
            auto& LiveInfo = *static_cast<live::info*>(ctx->userdata);
            auto& MyGame   = *static_cast<my_game*>(LiveInfo.m_pGame);

            MyGame.m_GameMgr->Run();

            break;
        }
    }

    return 0;
}
