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
        std::vector<range>      m_lRanges;

        std::unordered_map<std::uint64_t, std::uint64_t> m_EntityList;
    };

    struct mgr
    {
        std::vector<std::unique_ptr<instance>>   m_SceneInstances;
    };
}