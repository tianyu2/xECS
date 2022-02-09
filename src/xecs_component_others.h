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
        inline static xcore::err Serialize(xecs::serializer::stream&, bool, std::byte* ) noexcept;

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
            .m_pName            = "ShareFilter"
        ,   .m_SerializeMode    = xecs::component::type::serialize_mode::DONT_SERIALIZE
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
        inline static xcore::err Serialize          ( xecs::serializer::stream&, bool, std::byte* ) noexcept;
        inline static void       ReportReferences   ( std::vector<xecs::component::entity*>&, std::byte* ) noexcept;

        constexpr static auto typedef_v = xecs::component::type::data
        {
            .m_pName                = "Parent"
        ,   .m_pReportReferencesFn  = ReportReferences
        ,   .m_pSerilizeFn          = Serialize
        };

        xecs::component::entity m_Value;
    };

    //
    // Parent component for hierarchical entities, this components is used by entities that can have children.
    //
    struct children
    {
        inline static xcore::err FullSerialize      ( xecs::serializer::stream&, bool, std::byte*, int&  ) noexcept;
        inline static void       ReportReferences   ( std::vector<xecs::component::entity*>&, std::byte* ) noexcept;

        constexpr static auto typedef_v = xecs::component::type::data
        {
            .m_pName                = "Children"
        ,   .m_pReportReferencesFn  = ReportReferences
        ,   .m_pFullSerializeFn     = FullSerialize
        };

        std::vector<xecs::component::entity> m_List;
    };
}

property_begin_name( xecs::component::parent, xecs::component::parent::typedef_v.m_pName )
{
    property_var(m_Value).Name("Parent")
}
property_end()


property_begin_name( xecs::component::children, xecs::component::children::typedef_v.m_pName )
{
    property_var(m_List)
}
property_end()


#endif
