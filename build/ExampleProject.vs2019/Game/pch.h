// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include "xecs.h"
#include "../LiveCoding.h"
#include <random>


// Too hard to get the exit annotations correct for exit in recent Visual Studio compilers (2015 on) 
#pragma warning (disable:4244)  // Disable bogus VC++ 4.2 conversion warnings. 
#include <GL/gl.h>
#pragma comment (lib, "opengl32.lib") // link with Microsoft OpenGL lib

#endif
