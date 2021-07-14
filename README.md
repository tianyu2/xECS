# xECS -Lesson 04, More component types
Go to the [main branch](https://github.com/LIONant-depot/xECS/tree/master) for the introduction.

We cover.
* New types of components
  * Tags
  * Exclusive Tags
  * Share components
* New concepts in pools
  * Families
  * New way to handle Archetectural changes
  * You can have unlimited entities
* Archetypes
  * Shift to deal with types bits rather than in type::info*
  * Handle families for both shared and non-share
  * New way to compute the guid
* System
  * New child_update system
  * More types of events
    * OnPostStructuralChanges
    * OnGamePause
  * Clean up
* Queries
  * Added the iterator
* Tools::bits
  * Added more helpers
* Settings
  * More details settings
* Generally 
  * Major rework in organization Game_Mgr becomes little by little a container for other Managers
  * Major rework in the archetypes as PoolFamily are handle due to share components
  * Major clean up on giving more responsability to Managers
* Example updated to show new changes

## To build
1. Go to the build directory 
2. Click the GetDependencies.bat batch file
3. Wait untill it says that xECS is done
4. Go to the example project and load it, you should be able to run it at this point

## Dependencies
- [xCore](https://gitlab.com/LIONant/xcore) (for the profiler tracy, and few types )
- [CLUT](https://github.com/markkilgard/glut) (for the openGL example)

