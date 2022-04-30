<img src="https://i.imgur.com/TyjrCTS.jpg" align="right" width="220px" /><br>
# [xECS](xecs.md) / [Editor](xecs_editor.md) / Serialization

The organization of the saved project is as follows:

Folder Architecture:

~~~cpp
+- Project Files
|   +- Editor
|   |   |> 
|   |   +- Scenes
|   |   |   |> GUID.scene   // Scene File
|   |   |   |> GUID.scene   // Scene File
|   |   |
|   |   +- Prefabs
|   |       |> GUID.prefab  // Prefab File
|   |       |> GUID.prefab  // Prefab Files
|   |
|   +- Game
|   |   +- Scenes 
|   |   |   |> GUID.scene   // Scene File
|   |   |   |> GUID.scene   // Scene File
|   |   |
|   |   +- Prefabs
|   |       |> GUID.prefab  // Prefab File
|   |       |> GUID.prefab  // Prefab Files
|   |
~~~
