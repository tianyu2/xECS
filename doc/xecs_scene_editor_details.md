<img src="https://i.imgur.com/TyjrCTS.jpg" align="right" width="220px" /> <br>
# [xECS](xecs.md) / [Scene](editor.md) / Editor Details

When adding or editing entities in the editor we will be running the editor instance of xECS. When the game runs the game will load all the entities from the Editor Instance. Since the Editor instance of xECS should be completely in sync with the Game-runtime it is possible for the Run-Time to copy over all the relevant entities from memory if we wanted to. This would allow to very fast start/stop editor states. Some Issues could exists:

* Knowing which scenes are loaded.
* Executing scripts after a scene is loaded

In the editor for every Entity is created we will have a mirror entity. The mirror entity will be the one that deal with organization issues for the editor such been put in a folder. Also having a name, as well as other editor side only components. Note that while those component are  

## Editor Entities

| Component Type | Component Name          | Description |
|----------------|-------------------------|-------------| 
| Share          | editor Scene            | this will be a nice way to group all the entities that belong to one scene |
| Data           | editor Name             | This will be use to help the user add a name to an entity |
| Data           | editor Tag              | Tags used for the editor |
| Data           | editor Reference        | A reference to the real entity |
| Data           | Children                | Will be used for folders |
| Data           | Parent                  | Will be used for folders |
| exclusive tag  | editor_tag              | Will be used to differentiate between editor and regular entities |
| tag            | editor_selected_tag     | Will be use to determine which object are selected |
| Data           | editor_scene            | Options and such about controlling the scene in the editor |

---
