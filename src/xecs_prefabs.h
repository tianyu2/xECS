#ifndef XECS_PREFABS_H
#define XECS_PREFABS_H
#pragma once

namespace xecs::prefab
{
    struct tag
    {
        constexpr static auto typedef_v = xecs::component::type::exclusive_tag
        {
            .m_pName = "PrefabTag"
        };
    };

    //-------------------------------------------------------------------------------

    struct instance
    {
        constexpr static auto typedef_v = xecs::component::type::data
        {
            .m_pName = "PrafabInstance"
        };

        struct component
        {
            enum class type : std::uint8_t
            { OVERRIDES         // This component was part of the original archetype and I am overriding some properties
            , NEW               // New component so all properties are assumed to be overwritten 
            , REMOVE            // I removed this component from the original archetype
            };

            xecs::component::type::guid m_ComponentTypeGuid;
            std::vector<std::string>    m_PropertyOverrides;
            type                        m_Type;
        };

        xecs::component::entity     m_PrefabEntity;
        std::vector<component>      m_lComponents;
    };
}

#endif