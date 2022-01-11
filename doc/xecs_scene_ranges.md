<img src="https://i.imgur.com/TyjrCTS.jpg" align="right" width="220px" />
# [xECS](xECS.md)/[Editor](editor.md)/[Scene Ranges](xecs_scene_ranges.md)

## Introduction

A Scene range is a particular allocation of **xecs::component::type::entity::global_info**. Since all the global_infos are in a continues virtual array the address of each one of them is unique. A range is therefor a discrete chunk of a fixed size from this array. A range size in terms of entities is 131,768, so whenever a scene runs out of entities it must allocate another range.  

~~~cpp
|----------------------------------------------------------------------------------------------------|
| Fact about Ranges                                                                                  |
|----------------------------------------------------------------------------------------------------|
| Addressable Entity Index  = 2^32                                                                   |
| Info Size                 = 16 bytes <= sizeof( xecs::component::type::entity::global_info )       |
| entity Size               = 1 info size <= ( Since there is one global_info per entity )           |
| Cache Line                = 64 bytes                                                               |
| Virtual Page Size         = 4096 bytes                                                             |
| Sub-Range Size (bytes)    = Virtual Page Size                                                      |
| Sub-Range Size (entities) = 256 entities <= Virtual Page Size / Info Size                          |
| Range Size (sub-ranges)   = 512 sub-ranges <= (( Chosen by the architect ))                        |
| Range Size (entities)     = 131,072 entities <= (Range(sub ranges)) * (Sub-Range Size(entities))   |
| Range Size (byte)         = 2.097,152 MB <= (Range Size (sub-ranges)) * (Sub-Range Size (bytes))   |
| Total Ranges              = 32,768 ranges <= (Addressable Entity Index) / (Range Size(entities))   |
| Max Entities              = 4,294,967,296 entities <= Total Ranges * (Range Size(entities))        |
| Max Memory Usage          = 68.719,476,736 GB <= Max Entities * Info Size                          |
|----------------------------------------------------------------------------------------------------|
~~~

## Runtime preallocation of ranges

There are a few ranges that are preallocated by the system for the runtime. This set represents the maximum size that the runtime can be. The runtime will allocate the ranges as needed, which means that it won't wasteful in terms of memory.

~~~cpp
|----------------------------------------------------------------------------------------------------|
| Preallocated Ranges = 1,024 (( Chosen by the architect ))                                          |
| Total Sub-Ranges    = 524,288 sub-ranges <= Preallocated Ranges * (Range Size (sub-ranges))        |
| Total Entities      = 134,217,728 entities <= Preallocated Ranges * Range Size (entities)          |
| Total Memory Usage  = 2.147,483,648 GB <= Total Entities * Info Size                               |
|----------------------------------------------------------------------------------------------------|
~~~

## Range structure and runtime requirements

Since the entire range needs to be continuous the runtime must allocate base on its needs and the needs of the scenes. In order to be smart no wasting too much virtual address space and memory for the editor case the system will first allocate the current maximum range required by the scenes plus 1,024 Additional ranges as buffer. If it runs out of ranges it would need to realloc the entire set.

The actual runtime and editor structure will follow...
~~~cpp
+------------+
| Full Range | Ptr           --> Virtual Unmapped Pre-Allocated of what is likely needed plus the buffer
|            | Free          --> Pointer to the next free... (Used only for the runtime)
+------------+ LastSub-Range --> Number which indicates which sub-range was last allocated so it knows when it runs out of memory which other sub-range to alloc
~~~

Scenes will look as follows...
~~~cpp
+---------+      std::vector (Sorted by the fuller Ranges)
| Scene 0 | ---> +---------+
+---------+      |         | Address       --> Which address starts
                 | Range 0 | Hashmap       --> (256/precision) entries hash map that index into the array below key is the value of the u8, used to quickly allocate to the most full sub-page
                 |         | Ptr           --> std::array<u8, 512>{ how many entities are allocated per sub-block }
                 |         | HashmapNextPtr--> std::array<s16, 512>{ Pointer to the next entry. (used by the Hashmap) }
                 |         | nEntities     --> Number of allocated entities
                 +---------+
                 |         | 
                 | Range 1 | 
                 |         |
                 +---------+
                 | ...     | 
                 +---------+
~~~

### Range Map To Scene

We will have a table that will map a particular range to a scene index. This way is easy to know which ranges are already been used as well as identifying which entities map to which scene.

~~~cpp
    auto TableRangeToScene = std::unique_ptr<u16[]>(Total Ranges); // Initialize to 0xffff which will mean that the range has not been allocated to any scene

    IndexOfRange        = static_cast<int>(static_cast<std::size_t>(BaseAddressOfGlobalInfo - AddressEntityGlobalInfo) / (# entity per range)) - (Preallocated Ranges)))
    SceneIndex          = ( IndexOfRange < 0 ) ? (0xffff-1)   // This means that is a Run Time Entity, there for not in a scene
                                               : TableRangeToScene[IndexOfRange];
~~~

We will have also a Scene mgr which keeps track of the scenes

~~~cpp
    auto                   SceneArray = std::array<std::unique_ptr<xecs::scene::instance>, max_scenes_v>
    int                    nScenes    = ...
~~~
