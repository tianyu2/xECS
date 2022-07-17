<img src="https://i.imgur.com/TyjrCTS.jpg" align="right" width="220px" /><br>
# [xECS](xecs.md) / Scene



## Type of Entities 

A scene has type types of entities:

|  Type           | Description |
|-----------------|-------------|
| Global Entities | Entities that can be access by other Scenes entities and Scripts (using the entity component). In away these entities are similar to global variables in programming terms. This means that ***xecs::component::entity.m_GlobalIndex*** must remain static both at runtime and in the scene itself (no remapping allowed). To solve keeping this global index unique without been overriden by another entity in some other scene we assign [ranges](xecs_scene_ranges.md). |
| Local Entities  | These are the common type of entities scoped to the scene. Which means no other entity outside this scene can have references to them. However this entities could have references to ***Global Entities***. When the scene loads these entity types their ids will be remapped, so you can not hardcore the entity-id in scripts as each time they load they will be different. If you really want a reference to the entity-id you must make those entities global. <br>The remap usually happens when the scene gets loaded this means that xecs can solve it via the serialization functions provided by the user or by the property system, however remapping happens for other reasons as well, such in the case of prefab-instances, xecs can still solve the remap via properties for this case also but to keep things more efficient the user can provide a fast remap function in the components **void mycomponent::ReportReferences( xcore::func<void(xecs::component::entity&)> Callback )** |

Note: Remember the concept of **Overlapped Scene**, which it will try to minimize global-info ranges by knowing which other scenes are its neighbors.

There are two kinds of Scene files that are used for scenes:

## Creation of entities

The best way to think of a scene is as a glorified spawner of entities, A scene is a container of entities. There are two main types of context when we talk about an entity:

| Create Entity Context  | Description |
|------------------------|-------------|
| Scene                  | Entities are grouped by a Scene object which is invisible to most of xecs but allows to load/unload, save, etc as a group. |
| non-Scene              | Entities that are no in a Scene. These entities don't participate on the features offer by the scene object. |

## Scene File Formats

|  Scene File Formats      | Description |
|--------------------------|-------------|
| Asset Scene              | The scene file is meant to keep a nice workflow via layers and to maximize compatibility with changes in the scripts (Future proving the file). To accomplish this we need to keep the file format flexible. Unreal editor as well as Ubisoft keeps their entities saved in individual files, we are choosing to disregard this in favor of layers to speed up the loading/saving of scenes. |
| Resource Scene           | The scene file has been optimized for performance saving/loading of the scene data, this format is used when the game is released or when it is saves before the game runs, and reloaded afterwards. |

All the files related to the scenes will be located in the same directory. The range file will contain the ranges and could be consider the master file as it needs to be loaded first. This file is the one that should never be checkout unless you deleting or creating a new scene or you are requesting to increase the ranges of one scene.

## Folder organization

### Editor Asset Scene

When using the editor scenes are saved in a particular folders structure.

~~~cpp
+- Project Files
    +- Settings
    |   |- Project.ranges       // Ranges File
    |
    +- Editor Scene Assets      // Editor Scenes
    |   +- Scenes 
    |       +- GUID.scene       // Scene Folder
    |       |    |- GUID.layer  // Layer file
    |       |    |- GUID.layer  // Layer file
    |       |
    |       +- GUID.scene       // Scene Folder
    |            |- GUID.layer  // Layer file
    |            |- GUID.layer  // Layer file
    |
    +- Game Scene Assets        // Game Scenes
        +- Scenes               // All the scenes
        |   +- GUID.scene       // Scene Folder
        |   |    |- GUID.layer  // Layer file
        |   |    |- GUID.layer  // Layer file
        |   |
        |   +- GUID.scene       // Scene Folder
        |        |- GUID.layer  // Layer file
        |        |- GUID.layer  // Layer file
        |
        +- Prefabs              // All the prefabs
            |- GUID.prefab      // Prefab File
            |- GUID.prefab      // Prefab Files
~~~

### Final Resource Scene

When the game has been exported the scene files will be organized in a different way, and it won't be any layers any more.

~~~cpp
+ Resources                     // Resource folders
    + Scene                     // Scenes folders
    |   |- Guid01.scene         // A scene with all the exported layers
    |   |- Guid02.scene         // A scene with all the exported layers
    |
    + Prefabs                   // Prefab folders
        |- Guid01.prefab        // Prefab file
        |- Guid02.prefab        // Prefab file
~~~

File details: 
* [Scene Serialization File](xecs_scene_serialization.md)
* [Scene Global Ranges Serialization File](xecs_scene_ranges_serialization.md)

## Working with multiple users

You need to have a way to exclusively check-out file or else you will have problems...


## Dependencies

A scene can have other scenes as dependencies. This means that ones a particular scene loads up, a bunch others may also load up. This is a simple way to create levels in a game, where a level is a set of scenes. Scenes can also be used to stream parts of the level when ever they are required. Global scenes usually are default dependencies of local-scenes, however some local-scenes could break such dependency. A typical case for this is for example you may have a Font-End menu in you game, and this could be in a global scene. You can also have a Pause menu in another global scene. However when you have the Font-End menu you do not need the Pause menu and vice versa. 

Each scene will save into its own file. This file can be assume to be a resource. However there could be two kinds of scene file formats:

* **Editor File Format** - This format optimizes for compatibility across different versions of your game. Find out more in [This Document](editor_scene_serialization.md).
* **Runtime File Format** - This format optimized for loading speed. (TODO: Would be nice to have a detail document about this...)

## Ranges

Scenes provide a unique id to entities, this unique id is an index to a structure call (**xecs::component::type::entity::global_info**). There for it is important that the id of one entity does not conflict with any other entity. The way the system a chive that is explained in [This document](xecs_component_entity.md). The important thing to know is that each scene has a set of index-ranges where it can create entities in a safe manner. However this ranges are limited inside and a scene may run out of them. When this happen the Scene can request for additional ranges to the Project. This however requires that the user do an *exclusive-checkout* of the project file so it can guarantee that the operation will be atomic across all users. 


---
