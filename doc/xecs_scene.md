<img src="https://i.imgur.com/TyjrCTS.jpg" align="right" width="220px" /><br>
# [xECS](xecs.md) / Scene


The best way to think of a scene is as a glorified spawner of entities, A scene is a container of entities. There are two main types of context when we talk about an entity:

* **Runtime Context** which is when the entity is in the actual game. 
* **Scene Context** which is when the entity is just in a scene and has nothing to do with runtime.

A scene has different ways to store entities:

* **Global Entities** The main concept of storing entities this way is to allow any other entity to have access to them. This is useful when creating references to these entities. So in away these entities are similar to global variables in programming terms. This means that ***xecs::component::entity.m_GlobalIndex*** must remain static both at runtime and in the scene itself. To solve keeping this global index unique without been overriden by another entity in some other scene we assign [ranges](xecs_scene_ranges.md).

* **Local Entities** They should be the more common type. They are meant to hold entities that will be spawn into the runtime when the scenes are loaded. Entities store in this type of scenes can not have references to other Scene Local-Entities, only to other Scenes Global-Entities. This means that when the system loads the local-entities their ids will be remapped. This is why any component that has reference to other entities has to provide a function helper in its xecs::component::typedef/(xecs::component::type::info).

Note: Remember the concept of **Overlapped Scene**, which it will try to minimize global-info ranges by knowing which other scenes are its neighbors.

All the files related to the scenes will be located in the same directory. The range file will contain the ranges and could be consider the master file as it needs to be loaded first. This file is the one that should never be checkout unless you deleting or creating a new scene or you are requesting to increase the ranges of one scene.

~~~cpp

+--------------+      +--------------+
| Scene Folder |----->| Scene.ranges |  // This files contains all the ranges 
+--------------+      +--------------+
                      +--------------+
                      |              |
                      | Guid01.scene |  // Example of a scene file
                      |              |
                      +--------------+
                      +--------------+
                      |              |
                      | Guid02.scene |
                      |              |
                      +--------------+
                      +--------------+
                      |              |
                      | ...          | 
                      |              |
                      +--------------+
~~~

Details about: 
* [Scene Serialization File](xecs_scene_serialization.md)
* [Scene Range Serialization File](xecs_scene_range_serialization.md)


## Editor perspective

From an editor perspective there are two contexts:

* **Editor Context** an Entity which has a reference to a scene-entity.
* **Runtime Context**

This means that a single 

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
