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

    void CreateDefaultScene( void ) noexcept
    {
        //
        // Initialize global elements
        //
        std::srand(101);

        //
        // Create a prefab
        //
        auto PrefabGuid = m_GameMgr->CreatePrefab<position, velocity, timer, grid_cell>([&](position& Position, velocity& Velocity, timer& Timer, grid_cell& Cell) noexcept
        {
            Position.m_Value = xcore::vector2
            { static_cast<float>(std::rand() % renderer::s_pLiveRenderer->getWidth())
            , static_cast<float>(std::rand() % renderer::s_pLiveRenderer->getHeight())
            };

            Cell = grid::ComputeGridCellFromWorldPosition(Position.m_Value);

            Velocity.m_Value.m_X = std::rand() / static_cast<float>(RAND_MAX) - 0.5f;
            Velocity.m_Value.m_Y = std::rand() / static_cast<float>(RAND_MAX) - 0.5f;
            Velocity.m_Value.Normalize();

            Timer.m_Value = 0;
        });

        //
        // Create a Variant
        //
        auto PrefabVarientGuid = m_GameMgr->CreatePrefabVariant( PrefabGuid, [&]( timer& Timer ) noexcept
        {
            Timer.m_Value = std::rand() / static_cast<float>(RAND_MAX) * 8;
        });

        //
        // Create a prefab instance
        //
        m_GameMgr->CreatePrefabInstance( 2000, PrefabVarientGuid, [&]( position& Position, velocity & Velocity, timer& Timer, grid_cell& Cell) noexcept
        {
            Position.m_Value = xcore::vector2{ static_cast<float>(std::rand() % renderer::s_pLiveRenderer->getWidth())
                                             , static_cast<float>(std::rand() % renderer::s_pLiveRenderer->getHeight())
            };

            Cell = grid::ComputeGridCellFromWorldPosition(Position.m_Value);

            Velocity.m_Value.m_X = std::rand() / static_cast<float>(RAND_MAX) - 0.5f;
            Velocity.m_Value.m_Y = std::rand() / static_cast<float>(RAND_MAX) - 0.5f;
            Velocity.m_Value.Normalize();

            Timer.m_Value = std::rand() / static_cast<float>(RAND_MAX) * 8;
        });

        int b = 0;
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

    inline static constexpr auto save_in_binary_v = false;

    //-----------------------------------------------------------------------------

    virtual const char* SaveGameState(const char* pFileName) noexcept override
    {
        // Serialize first the prefabs/variants
        auto Error = m_GameMgr->SerializePrefabs("x64/Prefabs", false, false);
        assert(!Error);

        // Serialize the game state
        Error = m_GameMgr->SerializeGameState(pFileName, false, save_in_binary_v );
        assert(!Error);

        // Return error
        return Error ? Error.getCodeAndClear().m_pString : nullptr;
    }

    //-----------------------------------------------------------------------------

    virtual const char* LoadGameState(const char* pFileName) noexcept override
    {
        m_GameMgr.reset();
        xecs::component::mgr::resetRegistrations();
        m_GameMgr = std::make_unique<xecs::game_mgr::instance>();
        RegisterElements();

        auto Error = m_GameMgr->SerializeGameState(pFileName, true, save_in_binary_v );
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

void printStack(const char* pTempPath, CONTEXT* ctx);

//-----------------------------------------------------------------------------

DWORD handler( cr_plugin& ctx, DWORD code, _EXCEPTION_POINTERS* ex )
{
    printf("\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ EXCEPTION HAPPEN @@@@@@@@@@@@@@@@@@@@@@\n");
    printStack("", ex->ContextRecord);

    // If we loaded the default dll and failed then we are dead...
    if (ctx.version == 1) 
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    // If we can rewind lets try it
    ctx.version = ctx.last_working_version;

    return EXCEPTION_EXECUTE_HANDLER;
}

//-----------------------------------------------------------------------------

CR_EXPORT int cr_main(struct cr_plugin* ctx, enum cr_op operation)
{
    assert(ctx);
    switch (operation)
    {
        case CR_LOAD:
        {
            printf("\nLoading Game DLL Version (( %d ))\n", ctx->version );

            xcore::Init("ECS Example");

            auto& LiveInfo = *static_cast<live::info*>(ctx->userdata);
            auto& MyGame   = [&]() -> auto&
            {
                auto Game  = std::make_unique<my_game>();
                auto pGame = Game.get();
                LiveInfo.m_Game = std::move(Game);
                return *pGame;
            }();

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

            break;
        }
        case CR_UNLOAD:
        {
            printf("\nUnloading Game DLL Version { %d }\n", ctx->version);

            auto& LiveInfo = *static_cast<live::info*>(ctx->userdata);
            auto& MyGame   = *static_cast<my_game*>(LiveInfo.m_Game.get());
            auto  Error    = MyGame.SaveGameState("x64/Test.bin");
               
            LiveInfo.m_Game.reset();

            xcore::Kill();

            if(Error) return -1;
            break;
        }
        case CR_CLOSE:
        {
            printf("\nClosing Game DLL\n");
            auto& LiveInfo = *static_cast<live::info*>(ctx->userdata);
            LiveInfo.m_Game.reset();

            xcore::Kill();
            break;
        }
        case CR_STEP:
        {
            auto& LiveInfo = *static_cast<live::info*>(ctx->userdata);
            auto& MyGame   = *static_cast<my_game*>(LiveInfo.m_Game.get());

            __try
            {
                MyGame.m_GameMgr->Run();
            }
            __except (handler( *ctx, GetExceptionCode(), GetExceptionInformation() ))
            {
                return -1;
            }
            break;
        }
    }

    return 0;
}
