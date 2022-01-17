<img src="https://i.imgur.com/TyjrCTS.jpg" align="right" width="220px" />
# [xECS](xECS.md) / [Scene](editor.md) / [Serialization]() / [Entity]() / Prefab Instance

## Introduction

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

## PrefabInstance
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

## Components

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

## Relevant information for each of the components

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

---