<img src="https://i.imgur.com/TyjrCTS.jpg" align="right" width="220px" />
# [xECS](xECS.md) / [Scene](editor.md) / [Serialization]() / Entity

## Introduction

Scenes knows which entities they need to save because they have a list of those entities. This is the same list that is used to map entity references. These are contexts cases when user want to save Scenes:

1. You are in the editor
2. You are building your own app (Casual app)

Loading scenes:

1. In the editor
2. Building your own app (Casual app)
3. Runtime

There are 2 main cases to save entities:

1. **Regular Entities** These types of entities are just typical case.
2. **Prefabs Instances** These are entities that have been created from an Prefab or a Prefab Variant. 

Across these two cases there are a few common blocks mainly to do with the archetype which the entities were when they were saved. 

## Share entities

Share components are components that are been share with other entities. That sharing can be narrow down by a particular "scope". We have the following 3 scopes for sharing components:

## Different types

* [Share-Entities]()
* [Raw-Entities]()
* [Prefab-Entities]()

### Global Shares
These are components that are going to be share across all entities independently of archetype or families. You can have multiple instances of this types of components in a scene. Entities share these components based on their value. An example of a global share component could be a mesh instance. You may have a mesh that is a box and another mesh that is a cat. Then when you create an entity you must specify which global component it should be associated with it. In xECS this association happens indirectly. xECS has the concept of a family. Ones you create an entity the entity will be put in one of these families. These families are identify base on the values of their associated shares. So from the example before we would have two different families one with a global share component that is a box, and another family which its share component is a cat.

***Questions:***

> Can the user change a share component directly?
In xECS that atomic unity is the Entity, so there is not way to change a share component directly. 

> What happens if I have an entity which has a share component with a box and I change it to a cat?
That entity will be moved from the family that has the share containing the box to the family with the share of a cat.

> What happens if after a change my entity from box to cat and that was the last entity that had a box?
What xECS should do is to move the entity to their new family and delete the previous family, since there are not more entities in there. By deleting those families the references to the share will be decrease and so shares which their references reach zero will be deleted. However we may add a flag to share component types so that it leaves a global share alone even when it reaches zero references.

> What happens if before loading a scene I delete a particular share type?
The Loader will do its base to recreate the scene assuming that this share never existed. 

There are 3 kinds of share entities:

1. **Global Shares** - These are components that are going to be share across all entities independenly of archetype or families.
2. **Anchetype Shares** - These are components that are going to be share per archetype. This means that if I made a change to this component only the entities for that given archetype will notice.
3. **Family Shares** - These are components that belong to a particular family and share with any entities inside that family. So when that component changes only the entities inside the family will know it. Note that the Key for this component is its value mix with the archetype and with the family as well.

:warning: **Note: that currently no share (family) is supported.**
:warning: **Note: There may be a bug in the xECS Family Guid, and possible with share (Archetype, or Family) since everytime one of these shares changes their guid must also be updated.**
:warning: **Note: There may be a bug in the xECS shares and their Guid, Because if the share changes their guid/key value also changes, which currently is been ignore in the system.**

Shares have a guid which is generated base on their type and value. However their hash look up should be:
1. **Global Shares** - Should be just their type guid.
2. **Archetype Shares** - Should be their type and the guid of the archetype.
3. **Family Shares** - Should be their type guid, their archetype, and their (<u>Family Guid</u>??)

Family Guids is base on their share values, and the archetype. However when a share changes so does the Family.

When an Entity Share changes, that entity must be moved to a new family. (This is something that xECS automatically takes care of)

## Archetype Info
This block servers to indicate that we have an archetype to process. The block will give the info require to know some of the details to process it.

***Example:***
~~~cpp
[ ArchetypeInfo ]
{ bPrefabI:c  nFamilies:d  nShareTypes:d }
//----------  -----------  -------------
     0             1             0      
~~~

* **bPrefabI** - Tells if this archetype is made of prefabs instances
* **nFamilies** - The number of families for this archetype. This is mainly used for share components other wise this would always be one. When we have share components the number of families is equal to the number of unique sets of share components values for entities inside this archetype.
* **nShareTypes** - The number of components types that are shared

## Archetype Types
Here we must let the system which component types this particular archetype needs.

***Example:***
~~~cpp
// TypeInfo Details: 
//   Guid:0000000000000000   Type:Data   Name:Entity
//   Guid:895F4D644EFB4528   Type:Data   Name:ShareFilter
//   Guid:9912A2744953624C   Type:Data   Name:ReferenceCount
//   Guid:B746E8DB5F35D46F   Type:Share   Name:GridCell
//   Guid:49F9A4F507C9056B   Type:Tag   Name:ExclusiveShareAsData
[ ArchetypeTypes : 5 ]
{ TypeGuid:G         SerializationMode:c }
//-----------------  -------------------
                 #0           1         
  #895F4D644EFB4528           0         
  ...
~~~

* **TypeGuid** - is the GUID of the component type info. This is very important that matches with the component.
* **SerializationMode** - When serializing components of a particular type there are 3 kinds of modes that we support. 

***SerializationModes:***

|Mode|Description|
|:--:|-----------|
| 0 | Means that the component type won't save any information. This is typically a component type Tag. Either that or some component had not information to serialize|
| 1 | Means that it will use the serialize function provided by the user. This is a nice fast way to serialize entities. It also supports future proving|
| 2 | Means that the component will be saved by its properties. This is the default mode, and easier mode to use. However it is not as fast as mode 1.|


***Questions:***

**What happens if the user deletes a component type from the scripts?**
What it will happen is that those components will be removed from those entities.

**What happens if the components change their properties?**
If they are saved by (mode 2, properties) then those won't get loaded unless the user provides some properties that will remap between the old and the new version. If they are saved by (mode 1, serialization function) Then if the user does not proving a remapping those properties will be ignore. But not matter what it should not crash. 

**Will adding new properties to components work?**
There should work fine. However their value will be whatever their constructor does. 

**If the user saved the component with the serialize function and then added properties will it matter?**
No, the serialization system always prioritizes the serialization function over properties. However you do need to keep the serialization function updated.

**If the user saved the component with properties and then added the serialization function will it matter?**
No, the loading of old scenes will be done with the properties systems but future serialization will be done with the serialization function.

**What about if I serialize with the serialize function and then added properties and removed the serialization function?**
This would be a problem because old scenes still depends on the serialization function. This will cause the component to be created but its values will set the the default constructed values.

---