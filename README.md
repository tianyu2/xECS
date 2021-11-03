# xECS -Lesson 08, Properties
Go to the [main branch](https://github.com/LIONant-depot/xECS/tree/master) for the introduction.

We cover.
* How to add the xcore::properties to xECS for saving and loading. This mixes together with the normal serialization.
  Typical use: First used properties (for serialization and development), Step 2 pecialized key component serialization
  by adding the official serialization functions to make things go faster.
* Still uses the [CR.h](https://github.com/fungos/cr) but now the project is built from the main solution
  
## To build
1. Go to the build directory 
2. Click the GetDependencies.bat batch file
3. Wait untill it says that xECS is done
4. Go to the example project and load it, you should be able to run it at this point

## Dependencies
- [xCore](https://gitlab.com/LIONant/xcore) (for the profiler tracy, and few types )
- [CLUT](https://github.com/markkilgard/glut) (for the openGL example)

