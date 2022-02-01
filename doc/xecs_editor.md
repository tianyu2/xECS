<img src="https://i.imgur.com/TyjrCTS.jpg" align="right" width="220px" /><br>
# [xECS](xecs.md) / Editor

<h3><details><summary><i><b>Related Topics </b>(Click to open)</i></summary>

* [Component Serialization](xecs_component_serialization.md)
* [Component Properties](xecs_component_properties.md)
* [Component Typedef](xecs_component_typedef.md)
* [Scene entity references](ecs_scene_entity_references.md)
* [Scene Ranges](xecs_scene_ranges.md)
* [Scene file format, details about entities](xecs_scene_serialization_entity.md)
</details></h3>

There are many possible types of [Editor Architectures](xecs_editor_architectures.md) that xECS can support and it is up to the user to select the best one for it. However we will focus on a simple Layer One architecture to prioritize performance. 

~~~
                                  +------------------------+ 
                                  |                        | 
      +---------------+    +----->| Editor-DLL             |  
      |               |    |      |                        | 
      | Application   |----+      +------------------------+ 
      |               |    |                        
      +---------------+    |      +------------------------+ 
                           |      |                        | 
                           +----->| MiddleLayer-DLL        |  
                           |      |                        | 
                           |      +------------------------+
                           |
                           |      +------------------------+ 
                           |      |                        |  
                           +----->| Game-DLL               |  
                                  |                        | 
                                  +------------------------+
~~~

Since we have different DLLs we will have entities in all of them. However the [Editor Entities](xecs_editor_entities.md) are different because they specialize in the specific needs of the editor. This allows us to have add additional components which are useful in the editor context with out polluting the actual game run-time entity.


~~~cpp
struct link_back
{
    xecs::component::entity EditorEntity;
};
~~~


---