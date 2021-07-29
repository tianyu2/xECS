# xECS -Lesson 07, Live Coding
Go to the [main branch](https://github.com/LIONant-depot/xECS/tree/master) for the introduction.

We cover.
* Simple project organization
  * Game into its own project
     * xCore
     * xECS
     * Components and System per file
  * Main framework app
     * Could be an editor or game, etc.
* Serializing the game state
  * Remove field errors 
* Live Coding
  * add new project dependency [CR.h](https://github.com/fungos/cr)
  * Create Plugin API
  * Hooking and the plugin with the game and framework
  * Try handle exceptions correctly
* CLUT WARNING
  * Seems like CLUT is corrupting memory
  * Used winDBG to find out, also can be seem allocating too much memory. 
        Note that could be a combo of OpenGL and CLUT...
  
## To build
1. Go to the build directory 
2. Click the GetDependencies.bat batch file
3. Wait untill it says that xECS is done
4. Go to the example project and load it, you should be able to run it at this point

## Dependencies
- [xCore](https://gitlab.com/LIONant/xcore) (for the profiler tracy, and few types )
- [CLUT](https://github.com/markkilgard/glut) (for the openGL example)

