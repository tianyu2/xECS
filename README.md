# xECS -Lesson 03, Events and Maps
Go to the [main branch](https://github.com/LIONant-depot/xECS/tree/master) for the introduction.

We cover.
* New types of system
  * Updater 
  * Notifiers
  * Global Events delegates
  * System Events delegates
* Formalizing the event system
  * Systems have many kinds of events now
  * From overwriting the simple OnFrameStart
  * to Sending events
  * to reciving them
* Allot more heavy usage of maps in the code
  * Systems now can also cache references much more easily
* Updated the example
  * Shows new functinality
  * Added a simple spatial data base

## To build
1. Go to the build directory 
2. Click the GetDependencies.bat batch file
3. Wait untill it says that xECS is done
4. Go to the example project and load it, you should be able to run it at this point

## Dependencies
- [xCore](https://gitlab.com/LIONant/xcore) (for the profiler tracy, and few types )
- [CLUT](https://github.com/markkilgard/glut) (for the openGL example)

