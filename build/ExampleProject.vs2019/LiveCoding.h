
#ifndef _WINDLL
    #define CR_HOST CR_UNSAFE // try to best manage static states
#endif

#include "cr.h"

namespace live
{
    struct renderer
    {
        virtual                ~renderer            ()                                                           = default;
        virtual void            PageFlip            ( void )                                            noexcept = 0;
        virtual void            PrintAt             ( int X, int Y, const char* String, int Length )    noexcept = 0;
        virtual int             getWidth            ( void )                                            noexcept = 0;
        virtual int             getHeight           ( void )                                            noexcept = 0;
    };

    //----------------------------------------------------------------------------

    struct game
    {
        virtual                ~game               ()                                   = default;
        virtual void            CreateDefaultScene ( void )                    noexcept = 0;
        virtual const char*     SaveGameState      ( const char* pFileName )   noexcept = 0;
        virtual const char*     LoadGameState      ( const char* pFileName )   noexcept = 0;
    };

    //----------------------------------------------------------------------------

    struct info
    {
        renderer*               m_pRenderer {nullptr};
        std::unique_ptr<game>   m_Game      {};
    };

    //----------------------------------------------------------------------------
#ifdef CR_HOST

    struct plugin
    {
        cr_plugin       m_CRInstance;
        live::info      m_LiveInfo;
        std::string     m_FullPath;

        ~plugin(void) noexcept
        {
            cr_plugin_close(m_CRInstance);
        }

        void Initialize ( renderer& Render ) noexcept
        {
            m_LiveInfo.m_pRenderer = &Render;
        }

        bool Load( const char* pString ) noexcept
        {
            assert(m_LiveInfo.m_pRenderer);

            m_FullPath = std::filesystem::canonical(pString).string();

            if( false == cr_plugin_open( m_CRInstance, m_FullPath.c_str() ) )
            {
                if( cr_plugin_rollback(m_CRInstance) )
                {
                    assert(false);
                    return false;
                }
            }

            m_CRInstance.userdata  = &m_LiveInfo;
            return true;
        }

        bool LoadGameState( const char* pFileName ) noexcept
        {
            if( const char* pError = m_LiveInfo.m_Game->LoadGameState("x64/Test.bin"); pError )
            {
                assert(false);
                return false;
            }
            return true;
        }

        bool SaveGameState( const char* pFileName ) noexcept
        {
            if( const char* pError = m_LiveInfo.m_Game->SaveGameState("x64/Test.bin"); pError )
            {
                assert(false);
                return false;
            }
            return true;
        }

        bool Run( void ) noexcept
        {
            if( cr_plugin_update( m_CRInstance ) )
            {
                if (cr_plugin_rollback(m_CRInstance))
                {
                    assert(false);
                    return false;
                }
            }
            return true;
        }
    };

#endif

}