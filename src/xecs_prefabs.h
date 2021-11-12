#ifndef XECS_PREFABS_H
#define XECS_PREFABS_H
#pragma once

namespace xecs::prefab
{
    struct exclusive_tag
    {
        constexpr static auto typedef_v = xecs::component::type::exclusive_tag
        {
            .m_pName = "PrefabExclusiveTag"
        };
    };

    //-------------------------------------------------------------------------------

    struct tracker
    {
        constexpr static auto typedef_v = xecs::component::type::data
        {
            .m_pName = "PrafabTracker"
        };

        struct override
        {
            std::uint16_t           m_ComponentBitID;
            property::entry         m_Property;
        };

        xecs::component::entity     m_PrefabEntity;
        std::vector<override>       m_lProperties;
    };
}

#endif