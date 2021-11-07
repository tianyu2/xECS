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

    struct override_tracker
    {
        constexpr static auto typedef_v = xecs::component::type::data
        {
            .m_pName = "PrafabOverrideTracker"
        };

        struct override
        {
            std::uint16_t           m_ComponentBitID;
            property::entry         m_Property;
        };

        xecs::component::entity     m_PrefabEntity;
        std::vector<override>       m_lProperties;
    };

/*
    //-------------------------------------------------------------------------------

    template
    < typename T_FUNCTION = xecs::tools::empty_lambda
    > requires
    ( xcore::function::is_callable_v<T_FUNCTION>
    ) [[nodiscard]] xecs::component::entity AddOrRemoveComponents( xecs::game_mgr::instance&                            GameMgr
                                                                 , xecs::component::entity                              Entity
                                                                 , std::span<const xecs::component::type::info* const>  Add
                                                                 , std::span<const xecs::component::type::info* const>  Sub
                                                                 , T_FUNCTION&&                                         Function = xecs::tools::empty_lambda{}
                                                                 ) noexcept;

    //-------------------------------------------------------------------------------

    template
    <   typename T_TUPLE_ADD
    ,   typename T_TUPLE_SUBTRACT   = std::tuple<>
    ,   typename T_FUNCTION         = xecs::tools::empty_lambda
    > requires
    ( xcore::function::is_callable_v<T_FUNCTION>
    && xcore::types::is_specialized_v<std::tuple, T_TUPLE_ADD>
    && xcore::types::is_specialized_v<std::tuple, T_TUPLE_SUBTRACT>
    ) [[nodiscard]] xecs::component::entity AddOrRemoveComponents( xecs::game_mgr::instance&                            GameMgr
                                                                 , xecs::component::entity                              Entity
                                                                 , T_FUNCTION&&                                         Function = xecs::tools::empty_lambda{}
                                                                 ) noexcept;

    //-------------------------------------------------------------------------------

    template
    < typename T_FUNCTION = xecs::tools::empty_lambda
    > requires
    ( xcore::function::is_callable_v<T_FUNCTION>
    ) [[nodiscard]] xecs::component::entity AddOrRemoveComponents( xecs::game_mgr::instance&                            GameMgr
                                                                 , xecs::component::entity                              Entity
                                                                 , const xecs::tools::bits&                             Add
                                                                 , const xecs::tools::bits&                             Sub
                                                                 , T_FUNCTION&&                                         Function = xecs::tools::empty_lambda{}
                                                                 ) noexcept;
*/

}

#endif