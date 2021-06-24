# xECS -Lesson 02, Pulish the basics
Go to the [main branch](https://github.com/LIONant-depot/xECS/tree/master) for the introduction.

We cover.
* New types of system
  * Updater 
  * Notifiers
  * Global Events delegates
  * System Events delegates
* Formalizing the event system
  * Systems have many kinds of events now
  * From overritting the simple OnFrameStart
  * to Sending events
  * to reciving them
* Allot more heavy usage of maps in the code
  * System now can also cache references much more easily
* Update the example to use new functionality

## To build
1. Go to the build directory 
2. Click the GetDependencies.bat batch file
3. Wait untill it says that xECS is done
4. Go to the example project and load it, you should be able to run it at this point

## Dependencies
- [xCore](https://gitlab.com/LIONant/xcore) (for the profiler tracy, and few types )
- [CLUT](https://github.com/markkilgard/glut) (for the openGL example)

