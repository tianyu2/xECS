<img src="https://i.imgur.com/TyjrCTS.jpg" align="right" width="220px" /><br>
# [xECS](xecs.md) / [Prefabs](xecs_prefab.md) / Entity

<h3><details><summary><i><b>Related Topics </b>(Click to open)</i></summary>

* [Component Serialization](xecs_component_serialization.md)
* [Component Properties](xecs_component_properties.md)
* [Component Typedef](xecs_component_typedef.md)
* [Scene entity references](ecs_scene_entity_references.md)
* [Scene Ranges](xecs_scene_ranges.md)
* [Scene file format, details about entities](xecs_scene_serialization_entity.md)
</details></h3>

Entity Prefab is a Predefined Entity that contain a set of components with a specific set of values in them. One this Prefab Entity is defined the user can create clones of them in the editor call Entity Prefab Instances. These instances will be unique entities in their own right and the game is able to run any system on them. While the Original Entity Prefab has an exclusive tag **(xecs::prefab::tag)** which exclude it from most queries, and is left as a resource to create instances from it.

In the context of the game, Entity Prefab Instances have not additional information from any other entity. This is desirable because then these entities can share the same Archetype as any regular entity with the same components. 

In the context of the Editor, Entity Prefab Instances like any other entity from a Scene will have an additional component that will link back the to the Editor.DLL Entities. From there it will know if it is a prefab instance or not so it will know how to serialize properly.

Note that Entity Prefabs are not allowed to have children. If you add a child into a Entity Prefab you have convert it into a [Scene Prefab](xecs_prefab_scene.md)

## Entity Prefab vs Entity Prefab Instance

The Entity Prefab is an entity which is used to create the Entity Prefab Instances. Entity Prefabs are actual a kind of resource that must be loaded by the system. Like all resource they are assumed to be global entities so they don't belong to any Scene. 

Entity Prefab Instances are just like any other type of entity, in fact in the Game.DLL after it loads them or create them it won't know any difference at all. This allows entities to group together into larger Archetypes and be more efficient.

## Editor wise Prefab Instances 

Cloning prefabs is call creating a prefab-instance. In the [editor context Prefab-instances](xecs_editor_entities.md) are linked with the original Prefabs, and carry an extra component.

| State | Description |
|-------|----------|
| Added Component (X) | Added a new component to the prefab-instance. This component can be anything. This also  means that when the instance was created the component did not exited in the prefab. All properties for this component are assumed to be overriden. |
| Deleted Component (X) | Deleted a component from the prefab-instance. This component can be anything. This also means that when the instance was created the component did exist in the prefab. |
| Override Property (X,V) | Means that the component modified a value(V) of property (X). |

## Entity Prefab GUID vs Prefab EntityID

The entity prefab GUID **(xecs::prefab::guid)** is the official Identifier of a Prefab. This global identifier will also be used as its filename. The Prefab EntityID **(xecs::component::entity)** is the current ID of a prefab. Please note that every time a prefab is loaded the EntityID will be different as it could be remap to different slots every time. With the GUID you can get the Prefab's EntityID if needed. 

~~~cpp
struct xecs::prefab::guid
{
    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName            = "PrafabGUID"
    };

    xecs::guid::unit<64> m_Value;       // Why is this not a full resource guid???
}
~~~

The runtime will have a hash-map that will map between the xecs::prefab::guid to the actual EntityID.

## Creating an instance

Creation of Entity Prefab Instances are much faster than [Scene Prefabs](xecs_prefab_scene.md) because of the lower complexity that they deal with. The creation of a instance is done with the following steps:
1. Clones the entity (with add or removed component as requested by the user)
2. Calls the Callback 

## Editor.DLL Prefab

TODO: DO THIS?????


The prefab it


## Editor.DLL Prefab Instance Component

To keep track of which entities are prefab instances we have a component that the editor entities uses. It is able to track all the necessary changes.

~~~cpp
struct instance
{
    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName            = "PrafabInstance"
    };

    struct component
    {
        enum class type : std::uint8_t
        { OVERRIDES         // This component was part of the original archetype and I am overriding some properties
        , NEW               // New component so all properties are assumed to be overwritten 
        , REMOVE            // I removed this component from the original archetype
        };

        struct override
        {
            std::string             m_PropertyName;
            xcore::crc<32>          m_PropertyType;
        };

        xecs::component::type::guid m_ComponentTypeGuid;
        std::vector<override>       m_PropertyOverrides;
        type                        m_Type;
    };

    xecs::prefab::guid              m_Guid;                     // Guid of the prefab
    std::vector<component>          m_lComponents;
};
~~~

---
