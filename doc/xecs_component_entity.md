<img src="https://i.imgur.com/TyjrCTS.jpg" align="right" width="220px" />
# [xECS](xECS.md) / [Component](component_manager.md) / Entity

## Introduction
The Entity represents the concept of an object, and this objects are the atomic unit of xECS. Unlike C++ Object Oriented Objects which are identified by their type and instance pointer, and Entity does not have a type and stead of a pointer it contains a Unique Identifier. In the actual xECS the entity is represented in the following ways:

| Representation | Description |
|----------------|-------------|
| Component      | **xecs::component::type::entity** is the actual type in xECS C++<br>Here the entity is an actual component with a foot print of 64bits, That 64 bits is an ID with 2 parts:<br> 1. An Index to its global_info, which is the first part of the entity unique id.<br>2. A Validation structure, which contains the other part of the unique id, and a flag |
| global_info    | **xecs::component::type::entity::global_info** is the actual type in xECS C++ <br>There is a large array of global_info structures, the index of an entry represents part of the entities unique ID. The actual global_info structure contains: <br>1. Pointer to its pool so it knows how to locate its components <br>2. An index in the pool where all its components are<br>3. A Validation structure which should match the component::entity Validation

Like we mention before entities don't have types however the entire object representation of an entity (So entities and its components) can represent a dynamic type. This dynamic type is call an Archetype. So an Archetype is a unique collection of components. Entities are place inside of one of this Archetypes at run time, and as component are added or removed the entity will migrate to different Archetypes.

Ones you start dealing with Scenes you will realize that there are two kinds of entities:

* **Local-Entities** Entities that can not have references across different scenes.
* **Global-Entities** Entities that can be refer by any other entity independently of which scene you are. Please note that even if you have a reference to the global entity does not mean you can get access it because the scene may not be loaded.

To learn more about [Entity in Scenes check this]()

There is one more concept about Entities and it has to do with prefabs. Entities can either be created raw or they can be created via a prefab. If you create an entity using a prefab then what you have is a prefab instance which is also an entity. To [learn more about prefabs check this]()

## Entity Component Details

As explain above the entity structure has 2 main parts.
~~~cpp
union entity final                              // 64 bits size
{
    std::uint64_t       m_Value;                // Representation as a raw 64bits (This is actually the unique ID with an extra flag from the validation structure)

    struct                                      // This structure is 32bits
    {
        std::uint32_t   m_GlobalInfoIndex;      // Index to the global_info
        validation      m_Validation;           // Validation structure
    };
};
~~~

The validation part of the entity structure is as follows:

~~~cpp
union validation final                          // 32 bits size
{
    std::uint32_t       m_Value;                // Representation as a raw 32bits
    struct
    {
        std::uint32_t   m_Generation:31         // Used as a counter to avoid collision with older entity ids
        ,               m_bZombie:1;            // Indicates if the entity is a zombie (dead but not removed)
    };
};
~~~

Every entity in the game will have an entity component. The contents of the entity component is never changed by the user. It is assume to be private and should never be access directly. Most system use the entity component whenever they want to add or remove component, delete the particular entity, etc. So more like an ID that a real component.

## Entity global_info Details

The global info structure is quite simple;
~~~cpp
struct global_info final                                    // 16 bytes Size (Assuming 64bits build)
{
    xecs::pool::instance*           m_pPool         {};     // Pointer to the pool contains the entity components
    xecs::pool::index               m_PoolIndex     {};     // Index in the array of the pool where the actual components are
    validation                      m_Validation    {};     // The validation structure which should match with the entity component
};
~~~

The global_info is consider a detail for most users in xECS and really no one needs to care about it directly. As a concept is good to know as when doing more complex stuff (such global-entities) you should know the reasons why there are some limitations, however most people won't care.

There is an array of global_info this array is virtual because it uses virtual memory so it can be very large (up to 2^32 entries). However the actual break down in really is about ***~140 million entities*** reserved for local-entities for the runtime part, and the rest is for global-entities and their ranges. So xECS should be able to handle a large amount of entities. 



Global entities are a fascinated subject which You can learn more about [global_info ranges](xecs_scene_ranges.md)

