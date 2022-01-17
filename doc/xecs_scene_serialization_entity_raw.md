<img src="https://i.imgur.com/TyjrCTS.jpg" align="right" width="220px" />

# [xECS](xECS.md) / [Scene](editor.md) / [Serialization]() / [Entity]() / Raw

When saving regular Entities (meaning no prefab instances) the saving is a bit less abstract (meaning it is more better defined), there are for it the system can be faster at serialization. However there are plenty of challenges to overcome:

1. Removal of components
2. Changes of component data

***Questions:***

> What happens when a share-component changes its data?
Since links to the share components are done via their entity this should be ok. 

> When loading an entity will it be in the same pool?
Pools may be compactify at loading time to minimize the wasted memory.

:warning: this remains to be define a bit more

## Family Info
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

## Family Shares

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

## Pool Info
We will have X of this headers where (X = [FamilyInfo]:nPools). This header tell us how many entities should be in this particular pool. The total number of entities in a Family can be found out from the ([FamilyInfo]:nEntities).

***Example:***
~~~cpp
[ PoolInfo ]
{ nEntities:d }
//-----------
       0     
~~~

* **nEntities** - The number of entities to be found in this pool.
  
## Components
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

