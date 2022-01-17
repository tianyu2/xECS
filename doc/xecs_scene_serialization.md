<img src="https://i.imgur.com/TyjrCTS.jpg" align="right" width="220px" />
# [xECS](xECS.md) / [Scene](editor.md) / Serialization

## Introduction
The serialization of entities for the use of the editor requires us to make certain compromises. For instance future proving is more important than performance. Since things will be changing and we don't want to loose our work.

### <u>Introduction</u> - Challenges 
* Must survive the mutations/existence of properties
* Must survive the mutations/existence of components
* Must survive the mutations/existence of prefabs
  
---

## General information

### <u>General information</u> - Version
In order to know which version of the serialization we are we will save a stamp

***Example:***
~~~cpp
[ Version ]
{ Name:s            Number:f }
//---------------  -----------
  "xECS - Editor"     1.0
~~~

### <u>General information</u> - Map properties types to xcore::textfile types
xECS uses xcore::textfile to serialize its entities. Since properties are a typical way to serialize entities xECS will always create a user define block to map between the property types with the xcore::textfile types. Here is what you will see in to the of the file. Note that this may not look identical to you case as every application may have added a different set of properties.

***Example:***
~~~cpp
// New Types
< d:d b:c f:f s:s v2:ff entity:G C:C >
~~~

***Questions:***

**What would happen if a game that is trying to load the file has less propertie types?**
xcore::textfile and xcore::properties are able to handle this case just fine. However the entity's properties that had those types will be ignore.

**What would happens if we change the name of one of the property types?**
If the user still provides a mapping it should still work, other wise those properties will be ignore.

:warning: **TODO: We need to make this table as part of the user-settings**

### <u>General information</u> - Key information for the game manager

The GameMgr requires to know how many archetypes we are serializing. Remember that an archetype is a unique set of components, not matter of what type they are. Also note that prefabs/variants are saved separately using a different function. So xECS serialize will ignore any archetype with the xecs::prefab::tag in it.

* **nArchetypes** Indicates how many types of archetypes we are going to be serialize

***Example:***
~~~cpp
[ GameMgr ]
{ nArchetypes:d }
//-------------
        4      
~~~

### <u>General information</u> - Global Entities IDs
The component manager will serialize the validation part of the entities id. This help us to keep a true unique id for each of the entities. Note that the system will try to minimize the number of validations that it needs to save to keep the file as small as possible.
  
***Example:***
~~~cpp
[ GlobalEntities : 2348 ]
{ Validation:g }
//------------
       #0     
       #0 
       ...
~~~

* **Validation** Validation is part of the entityID. This part keeps track on how many versions have been created so far for a given index. In this case the index is explicit. 

***Question: can you give an example on why this is useful?***
If for whatever reason the user created an entity and copy and paste the EntityID in a C++ script and then later on tries to use that entity the system must be sure that the entity in question can not be confused by any other entity. By having that sequence been track not matter what, we can always guarantee it.

:warning: **The edge case that will cause the system to fail is if the validation rolls over and some older scripts was pointing and older entity**

:warning: **TODO: The future of Entities ID may change to allow global entities, and multi-scene entities to work**

:warning: **TODO: in the future we may may have virtual ranges for entity IDs, so each scene/layer may have a virtual range where it can create entities in. There for the concept of a "project" file which may be in charge of keeping the ranges of ids for each scene may be needed.**

---

# Saving Entities
There are 2 main cases to save entities:

1. **Regular Entities** These types of entities are just typical case.
2. **Prefabs Instances** These are entities that have been created from an Prefab or a Prefab Variant. 

Across these two cases there are a few common blocks mainly to do with the archetype which the entities were when they were saved. 

## <u>Saving Entities</u> - Share entities

Share components are components that are been share with other entities. That sharing can be narrow down by a particular "scope". We have the following 3 scopes for sharing components:

### Global Shares
These are components that are going to be share across all entities independently of archetype or families. You can have multiple instances of this types of components in a scene. Entities share these components based on their value. An example of a global share component could be a mesh instance. You may have a mesh that is a box and another mesh that is a cat. Then when you create an entity you must specify which global component it should be associated with it. In xECS this association happens indirectly. xECS has the concept of a family. Ones you create an entity the entity will be put in one of these families. These families are identify base on the values of their associated shares. So from the example before we would have two different families one with a global share component that is a box, and another family which its share component is a cat.

***Questions:***

**Can the user change a share component directly?**
In xECS that atomic unity is the Entity, so there is not way to change a share component directly. 

**What happens if I have an entity which has a share component with a box and I change it to a cat?**
That entity will be moved from the family that has the share containing the box to the family with the share of a cat.

**What happens if after a change my entity from box to cat and that was the last entity that had a box?**
What xECS should do is to move the entity to their new family and delete the previous family, since there are not more entities in there. By deleting those families the references to the share will be decrease and so shares which their references reach zero will be deleted. However we may add a flag to share component types so that it leaves a global share alone even when it reaches zero references.

**What happens if before loading a scene I delete a particular share type?**
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

### <u>Saving Entities</u> - Archetype Info
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

### <u>Saving Entities</u> - Archetype Types
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

### <u>Saving Entities</u> - (Regular Entities)
When saving regular Entities (meaning no prefab instances) the saving is a bit less abstract (meaning it is more better defined), there are for it the system can be faster at serialization. However there are plenty of challenges to overcome:

1. Removal of components
2. Changes of component data

***Questions:***

**What happens when a share-component changes its data?**
Since links to the share components are done via their entity this should be ok. 

**When loading an entity will it be in the same pool?**
Pools may be compactify at loading time to minimize the wasted memory.
:warning: this remains to be define a bit more

### <u>Saving Entities</u> - (Regular Entities) - Family Info
Each archetype has a family of pools, this is mainly used for share components where each unique set of values of share components creates a new family. For non-share component Archetypes there will only be just one Family.

***Example:***
~~~cpp
[ FamilyInfo ]
{ nPools:d  nEntities:d }
//--------  -----------
     1           100     
~~~

* **nPools** - the number of pools that a family has.
* **nEntities** - the number of entities for this family. This field is really not needed but is use for debugging and such.

:warning: ***Note: xECS keeps the same number of pools when the file was serialized. This is done on purpose to minimize potential issues with anything.***

### <u>Saving Entities</u> - (Regular Entities) - Family Shares

This is the list of share entities that belong to the family. The reason why they are entities references is that they allow us to get access to them even if their keys have change. We also add the type of the share component so that is easier to filter out in case we no longer have this component. However the order of these entities should follow the order specified in the ArchetypeTypes block. So this is a bit overkill... (may be removed)

***Example:***
~~~cpp
[ FamilyShares:d ]
{ Entity:G  ShareTypeGuid:G }
//-----------------  -----------------
  #61F42A111DAF651C  #61F42A111DAF651C
~~~

* **Entity** - Entity which contains the Index of the global entity which contains the actual share component
* **ShareTypeGuid** - Share component guid for its type

In case (1) the references to the share-component will ultimately be zero and ultimately be removed. Also in this case the "family" concept may be already lost as for instance the prefab may not longer have any families. 

### <u>Saving Entities</u> - (Regular Entities) - Pool Info
We will have X of this headers where (X = [FamilyInfo]:nPools). This header tell us how many entities should be in this particular pool. The total number of entities in a Family can be found out from the ([FamilyInfo]:nEntities).

***Example:***
~~~cpp
[ PoolInfo ]
{ nEntities:d }
//-----------
       0     
~~~

* **nEntities** - The number of entities to be found in this pool.
  
### <u>Saving Entities</u> - (Regular Entities) - Components
We will serialize each of the component types together for better efficiency. These component types could be saved like we mention before in two ways, via a serialization function or via properties. 

***Example using a serialization function:***
~~~cpp
//Entity
[ 0000000000000000 : 330 ]
{ Entity:G }
//--------
     #0   
     #3   
     ...
~~~

***Example using the properties of a component:***
~~~cpp
[ Props : 1 ]
{ Name:s            Data:?                        }
//----------------  -----------------------------
  "velocity/Value"  ;v2 0.7647978663 0.6442690492
~~~

* **Name** - is the name of the property in string form
* **Data** - is the property type (;v2) and then the data associated with the type 0.74.., 0.62.. Note that v2 was one of the original type of the properties that we indicated at the top.

### <u>Saving Entities</u> - (Prefab Instances Entities)
When saving prefab instance entities we need to save the minimum information that we need to recreate the instance. To recreate the instance we need to follow these steps:

1. Identify the prefab
2. Create an instance of the prefab with a particular entityID (The entity ID of the entity we are trying to recreate)
3. Change the instance base on possible component changes (such added or moved components)
4. Change the values of our components to reflect the values of our instance

***Questions:***

**What happens if we are loading an prefab instance with share components?**
There are 3 cases:
1. The Prefab may have change/removed the share component (not longer related to the prefab)
2. The prefab may have updated the share component (change some of its values)
3. The prefab have not done anything

**what happens if the user deletes the prefab?**
The Entity won't be created

**What happens if the prefab has the same component as the prefab instance says is a new component?**
The component still completely overriden by the prefab instance.

**What happens if we delete a component that the prefab instance has overrides?**
The component is completely removed from the entity

**What happens if the prefab removes a component that the instance also removed?**
Nothing should work ok.

### <u>Saving Entities</u> - (Prefab Instances Entities) - PrefabInstance
This header indicates that we are entering the PrefabInstance context. It contains related information to this context.

***Example:***
~~~cpp
[ PrefabInstance ]
{ EntityID:G  PrefabID:G }
//----------  -----------
      #2          #0     
~~~

* **EntityID** - EntityID 
* **PrefabID** - Tells the system which prefab or variant does this entity instance was created from

### <u>Saving Entities</u> - (Prefab Instances Entities) - Components

***Example:***
~~~cpp
[ Components : 1 ]
{ ComponentGuid:G    Type:d }
//-----------------  ------
  #F8E0155B9ABC0972    0   
~~~

* **ComponentGuid** - Component type guid, found from xecs::component::type::info_v
* **Type** - Tells what type of overrides we have for this component.

|Type| Description |
|:--:|-------------|
| 0  | New component so all its properties go overriden. It won't care what the prefab says about this component. |
| 1  | Some properties from this component got overriden by the entity. |
| 2  | The component got deleted from the entity.|

### <u>Saving Entities</u> - (Prefab Instances Entities) - Relevant information for each of the components

Here is where either are properties serialize related to the the previous header. Note that components could be serialize in two modes. (Serialize function of via properties). The system will try to serialize in whatever mode is more efficient or has for that matter.

***Example:***
~~~cpp
[ Props : 1 ]
{ Name:s            Data:?                        }
//----------------  -----------------------------
  "velocity/Value"  ;v2 0.7647978663 0.6442690492
~~~

* **Name** - Name of the property
* **Data** - Data of the property