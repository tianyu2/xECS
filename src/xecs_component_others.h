#ifndef XECS_COMPONENT_OTHERS_H
#define XECS_COMPONENT_OTHERS_H
#pragma once

namespace xecs::component
{
    //
    // Reference Count Data Component
    //
    struct ref_count
    {
        static xcore::err Serialize(xcore::textfile::stream& TextFile, bool, std::byte* pData) noexcept
        {
            auto& RefCount = *reinterpret_cast<ref_count*>(pData);
            return TextFile.Field("GlobalIndex", RefCount.m_Value);
        }

        constexpr static auto typedef_v = xecs::component::type::data
        {
            .m_pName       = "ReferenceCount"
        ,   .m_pSerilizeFn = Serialize
        };

        int m_Value{ 1 };
    };

    //
    // Exclusive tag that tells Archetypes to treat share components as data components
    //
    struct share_as_data_exclusive_tag
    {
        constexpr static auto typedef_v = xecs::component::type::exclusive_tag
        {
            .m_pName = "ExclusiveShareAsData"
        };
    };

    //
    // Query Share Filter component
    //
    struct share_filter
    {
        constexpr static auto typedef_v = xecs::component::type::data
        {
            .m_pName = "ShareFilter"
        };

        struct entry
        {
            xecs::archetype::instance*          m_pArchetype;
            std::vector<xecs::pool::family*>    m_lFamilies;
        };

        std::vector<entry>  m_lEntries;
    };

    //
    // Parent component for hierarchical entities, used only for entities that have parents (not root entities).
    //
    struct parent
    {
        constexpr static auto typedef_v = xecs::component::type::data
        {
            .m_pName = "Parent"
        };

        xecs::component::entity m_Parent;
    };

    //
    // Parent component for hierarchical entities, this components is used by entities that can have children.
    //
    struct children
    {
        constexpr static auto typedef_v = xecs::component::type::data
        {
            .m_pName = "Children"
        };

        std::vector<xecs::component::entity> m_Children;
    };
}
#endif
