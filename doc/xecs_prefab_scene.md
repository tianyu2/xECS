<img src="https://i.imgur.com/TyjrCTS.jpg" align="right" width="220px" /><br>
# [xECS](xecs.md) / [Prefabs](xecs_prefab.md) / Scene

<h3><details><summary><i><b>Related Topics </b>(Click to open)</i></summary>

* [Component Serialization](xecs_component_serialization.md)
* [Component Properties](xecs_component_properties.md)
* [Component Typedef](xecs_component_typedef.md)
* [Scene entity references](ecs_scene_entity_references.md)
* [Scene Ranges](xecs_scene_ranges.md)
* [Scene file format, details about entities](xecs_scene_serialization_entity.md)
</details></h3>

Please make sure that [Entity Prefabs](xecs_prefab_entity.md) make sense before understanding what this is. Scene Prefabs are similar in concept to an [Entity Prefab](xecs_prefab_entity.md) in concept except stead of having one Entity you can have Many. When you have many entities in a way you are creating a mini Scene. One of the key differences between a regular Entity Prefab is that here we need a pivot point.

Similar to what happens when you select multiple entities in a editor you need a point that can be consider the pivot/origin of the Scene. Where you can rotate/scale/position the scene base on this point.

This Scene can have both regular entities and Prefab Instances (which include both Scene Prefab Instances and Entity Prefab Instances). Unlike real scenes they must have the pivot point and they can not have global entities. 

## Pivot point

The pivot point is an Entity which has a transform component. The user can choose if ones it creates the instance the pivot point is retained as a parent entity or not. The default is that is not kept as having hierarchies creates complexities and slows down performance. 

## Creating an instance



