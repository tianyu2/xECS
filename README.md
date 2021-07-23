# xECS -Lesson 05, Const and Improvements
Go to the [main branch](https://github.com/LIONant-depot/xECS/tree/master) for the introduction.

We cover.
* Share component filters
  * Can specify if you want your share component to have a filter or not
  * findShareFilter 
* Const 
  * Adding const to arguments of function create speed ups
  * Faster Share component reading, etc.
* System
  * Exclusive functions for systems
  * System understand what share filters are
  * Clean up
* Generally 
  * Factor out more mgrs 
* Example
  * updated to show new changes
  * Added a more efficient type of grid

## To build
1. Go to the build directory 
2. Click the GetDependencies.bat batch file
3. Wait untill it says that xECS is done
4. Go to the example project and load it, you should be able to run it at this point

## Dependencies
- [xCore](https://gitlab.com/LIONant/xcore) (for the profiler tracy, and few types )
- [CLUT](https://github.com/markkilgard/glut) (for the openGL example)

