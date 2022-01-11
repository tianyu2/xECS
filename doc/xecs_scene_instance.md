<img src="https://i.imgur.com/TyjrCTS.jpg" align="right" width="220px" />
# [xECS](xECS.md)/[Editor](editor.md)/[Scene](editor_scene_serialization.md)

## Introduction
Scenes are how xECS manages entity grouping for serialization and organization purposes. There are two kinds of scenes:

* **Global Scenes** These are scenes that are meant to be always loaded. Their purpose is to provide Local-Scenes access to common objects across them, things like the player for instance. So local-scenes can have references to Entities inside Global-Scenes.
* **Local Scenes** These type of scenes are used to separate different grouping of entities and are pretty much separated from each other. However you can load several Local-Scenes at the same time. This allows things like transitions from one area of the map to another simple. However the limitation is that they can not have references to other entities outside themselves.
* **Overlapped Scene** These is a type of scene that can have be overlap by an specific set of neighbors. This means that a given range could be overlap by other overlapped-Scenes as long as they are not their neighbors. This is a nice way to create infinite worlds by keep reusing ranges. A typical example of these type of scenes are world made out of a grid scenes. Each of those scenes will be an overlapped scene.  

There is one more concept worth mentioning and is the run-time. The run-time is the common space across all scenes. While there may be entities that have logic created in Local Scenes those entities won't ever be saved by the game. Those entities that must be saved by the game should be created by spawners from scenes in the run-time. A typical thing would be to span an entity in the run-time which contain a component with a global state, such pickup information (like a sword), then the actual Player Entity can pick that Object.

All the files related to the scenes will be located in the same directory. The range file will contain the ranges and could be consider the master file as it needs to be loaded first. This file is the one that should never be checkout unless you deleting or creating a new scene or you are requesting to increase the ranges of one scene.

~~~cpp

+--------------+      +--------------+
| Scene Folder |----->| Scene.ranges |  // This files contains all the ranges 
+--------------+      +--------------+
                      +--------------+
                      |              |
                      | Guid01.local |  // Example of local-scene file
                      |              |
                      +--------------+
                      +--------------+
                      |              |
                      | Guid02.global|  // Example of global scene file
                      |              |
                      +--------------+
                      +--------------+
                      |              |
                      |Guid03.overlap|  // Example of overlapped scene file
                      |              |
                      +--------------+
                      // ... etc
~~~

## Dependencies

A scene can have other scenes as dependencies. This means that ones a particular scene loads up, a bunch others may also load up. This is a simple way to create levels in a game, where a level is a set of scenes. Scenes can also be used to stream parts of the level when ever they are required. Global scenes usually are default dependencies of local-scenes, however some local-scenes could break such dependency. A typical case for this is for example you may have a Font-End menu in you game, and this could be in a global scene. You can also have a Pause menu in another global scene. However when you have the Font-End menu you do not need the Pause menu and vice versa. 

Each scene will save into its own file. This file can be assume to be a resource. However there could be two kinds of scene file formats:

* **Editor File Format** - This format optimizes for compatibility across different versions of your game. Find out more in [This Document](editor_scene_serialization.md).
* **Runtime File Format** - This format optimized for loading speed. (TODO: Would be nice to have a detail document about this...)

## Ranges

Scenes provide a unique id to entities, this unique id is an index to a structure call (**xecs::component::type::entity::global_info**). There for it is important that the id of one entity does not conflict with any other entity. The way the system a chive that is explained in [This document](xecs_component_mgr_global_entities.md). The important thing to know is that each scene has a set of index-ranges where it can create entities in a safe manner. However this ranges are limited inside and a scene may run out of them. When this happen the Scene can request for additional ranges to the Project. This however requires that the user do an *exclusive-checkout* of the project file so it can guarantee that the operation will be atomic across all users. 

## File formats

For the scene file formats you can find it in [This Document]()


---
