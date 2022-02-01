<img src="https://i.imgur.com/TyjrCTS.jpg" align="right" width="220px" /><br>
# [xECS](xecs.md) / Prefabs

<h3><details><summary><i><b>Related Topics </b>(Click to open)</i></summary>

* [Component Serialization](xecs_component_serialization.md)
* [Component Properties](xecs_component_properties.md)
* [Component Typedef](xecs_component_typedef.md)
* [Scene entity references](ecs_scene_entity_references.md)
* [Scene Ranges](xecs_scene_ranges.md)
* [Scene file format, details about entities](xecs_scene_serialization_entity.md)
</details></h3>

Prefabs also known in the game industry as (blue-prints, or archetypes) is a way to to define an Entity or Scene with a specific set of values in them for the shake of cloning them (know as prefabs instances) at runtime or in the editor. There are two kind of prefabs:

1. [Entity Prefabs](xecs_prefab_entity.md) - Which means we are dealing with a single entity
2. [Scene Prefabs](xecs_prefab_scene.md) - Which means that we are dealing with a group of entities 

There are two Context in which prefabs operate:

1. **Runtime** - The function calls will be made by systems. 
2. **Editor** - The user will be able to create prefabs and prefabs instance using the editor. This context provides extra functionality over the Runtime such property overrides and component overrides. 

## Prefab Instances 

Cloning prefabs is call creating a prefab-instance. In the [editor context Prefab-instances](xecs_editor_entities.md) are linked with the original Prefabs, and carry an extra component.

| State | Description |
|-------|----------|
| Added Component (X) | Added a new component to the prefab-instance. This component can be anything. This also  means that when the instance was created the component did not exited in the prefab. All properties for this component are assumed to be overriden. |
| Deleted Component (X) | Deleted a component from the prefab-instance. This component can be anything. This also means that when the instance was created the component did exist in the prefab. |
| Override Property (X,V) | Means that the component modified a value(V) of property (X). |

## Editor Prefab Instance

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

    xecs::component::entity         m_PrefabEntity;
    std::vector<component>          m_lComponents;
};
~~~

## Serializing Prefabs

Serializing prefabs is very similar to how a [scene serializes](xecs_scene_serialization.md). Scene groups entities base on where they were created, usually in the editor under the context of a particular scene. Prefabs don't belong to any scene, prefab instances do. Prefabs are thought to be entities that are globally accessible by any scene and always loaded, they are in the truer sense global objects. To know which entity is a prefab it uses a [exclusive_tag](xecs_component_type_tag.md) named ***xecs::prefab::tag***.