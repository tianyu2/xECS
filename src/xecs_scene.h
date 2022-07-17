namespace xecs::scene
{
    using guid = xcore::guid::unit<64, struct scene_tag>;

    enum class type : std::uint8_t
    { LOCAL
    , GLOBAL
    , OVERLAPPED
    };

    constexpr std::int32_t range_size_v             = 512;
    constexpr std::int32_t sub_range_size_v         = 256;

    struct range
    {
        static constexpr auto sub_range_precision_v    = 4;
        static constexpr auto hash_size_v              = sub_range_size_v / sub_range_precision_v;

        std::size_t                              m_Address;
        int                                      m_nEntities;
        std::array<std::uint8_t, hash_size_v >   m_HashEmpty;
        std::array<std::uint8_t, range_size_v >  m_BlockEntityCount;
        std::array<std::int16_t, range_size_v >  m_HashNextSubRange;
    };

    struct instance
    {
        guid                    m_Guid;
        xcore::cstring          m_Name;
        type                    m_Type;
        std::vector<range>      m_lGlobalRanges;

        std::unordered_map<std::uint64_t, std::uint64_t> m_LoadRemappedList;
    };

    struct component
    {
        constexpr static auto typedef_v = xecs::component::type::share
        { .m_pName          = "Scene" 
        , .m_bBuildFilter   = true
        , .m_ReferenceMode  = xecs::component::type::reference_mode::NO_REFERENCES
        };

        __inline constexpr xecs::component::type::share::key ComputeShareKey( void ) noexcept
        {
            return {m_SceneGuid.m_Value};
        }
    
        guid m_SceneGuid;
    };

    struct mgr
    {
        std::vector<std::unique_ptr<instance>>   m_SceneInstances;
    };
}