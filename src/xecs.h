#ifndef XECS_H
#define XECS_H
#pragma once

//--------------------------------------------------------------
// HEADER FILES
//--------------------------------------------------------------

//
// SYSTEM
//
#define NOMINMAX
#include "Windows.h"

#include <iostream>
#include <vector>
#include <array>
#include <functional>
#include <bit>
#include <bitset>

//
// EXTERNAL DEPENDENCIES
//
#include "../dependencies/xcore/src/xcore.h"

//--------------------------------------------------------------
// PREDEFINITIONS
//--------------------------------------------------------------
namespace xecs::game_mgr
{
    struct instance;
}

namespace xecs::system
{
    struct instance;
    struct mgr;

    namespace details
    {
        template< typename T_USER_SYSTEM >
        requires( std::derived_from< T_USER_SYSTEM, xecs::system::instance > )
        struct compleated;
    }
}

namespace xecs::archetype
{
    struct instance;
    struct mgr;
    using guid = xcore::guid::unit<64, struct archetype_tag>;
}

namespace xecs::pool
{
    struct instance;
    struct family;
    struct index
    {
        int     m_Value;
        friend auto operator <=> (const index&, const index&) = default;
    };
}

namespace xecs::query
{
    template< typename T_FUNCTION, auto T_MODE_V >
    struct iterator;
}

//--------------------------------------------------------------
// FILES
//--------------------------------------------------------------
#include "xecs_settings.h"
#include "xecs_event.h"
#include "xecs_event_mgr.h"
#include "xecs_component.h"
#include "xecs_tools.h"
#include "xecs_tools_bits.h"
#include "xecs_component_mgr.h"
#include "xecs_pool.h"
#include "xecs_archetype.h"
#include "xecs_archetype_mgr.h"
#include "xecs_query.h"
#include "xecs_query_iterator.h"
#include "xecs_system.h"
#include "xecs_system_mgr.h"
#include "xecs_game_mgr.h"

//--------------------------------------------------------------
// INLINE FILES
//--------------------------------------------------------------
#include "details/xecs_component_inline.h"
#include "details/xecs_tools_inline.h"
#include "details/xecs_tools_bits_inline.h"
#include "details/xecs_component_mgr_inline.h"
#include "details/xecs_pool_inline.h"
#include "details/xecs_archetype_inline.h"
#include "details/xecs_archetype_mgr_inline.h"
#include "details/xecs_system_inline.h"
#include "details/xecs_system_mgr_inline.h"
#include "details/xecs_query_iterator_inline.h"
#include "details/xecs_game_mgr_inline.h"
#include "details/xecs_query_inline.h"
#include "details/xecs_event_inline.h"
#include "details/xecs_event_mgr_inline.h"

#endif