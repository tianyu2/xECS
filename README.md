# xECS -Lesson 02, Pulish the basics
Go to the [main branch](https://github.com/LIONant-depot/xECS/tree/master) for the introduction.

We cover.
* Components
  * Introduction of Global Unique Indentifiers (GUID) for the types
  * Introduction of different types of components (even though we dont do much with them)
* Improving the pools V2
  * No allocations when deleting std::vector is gone
  * Structural updates now more centralize in the pools (not completly yet)
    * Deleting is centralized here
    * Size depends on updating the structural changes
  * Adding and Removing Components
  * Components are shorted by their GUID which allows for faster access
  * Appending can now create N entities
* Game Mgr 
  * Adding and Removing Components to an entity
  * Creating N entities are faster handle by CreateEntities function
* Archetype
  * Adding and removing components
  * Components are shorted by their GUID for faster access
* Update the example to use new functionality

## To build
1. Go to the build directory 
2. Click the GetDependencies.bat batch file
3. Wait untill it says that xECS is done
4. Go to the example project and load it, you should be able to run it at this point

## Dependencies
- [xCore](https://gitlab.com/LIONant/xcore) (for the profiler tracy, and few types )
- [CLUT](https://github.com/markkilgard/glut) (for the openGL example)

