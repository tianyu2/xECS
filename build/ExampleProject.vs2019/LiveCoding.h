
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
        renderer*   m_pRenderer {nullptr};
        game*       m_pGame     {nullptr};
    };
}