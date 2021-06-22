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
}

//--------------------------------------------------------------
// FILES
//--------------------------------------------------------------
#include "xecs_settings.h"
#include "xecs_event.h"
#include "xecs_component.h"
#include "xecs_tools.h"
#include "xecs_pool.h"
#include "xecs_archetype.h"
#include "xecs_query.h"
#include "xecs_system.h"
#include "xecs_game_mgr.h"

//--------------------------------------------------------------
// INLINE FILES
//--------------------------------------------------------------
#include "details/xecs_component_inline.h"
#include "details/xecs_system_inline.h"
#include "details/xecs_pool_inline.h"
#include "details/xecs_archetype_inline.h"
#include "details/xecs_game_mgr_inline.h"
#include "details/xecs_query_inline.h"
#include "details/xecs_tools_inline.h"
#include "details/xecs_event_inline.h"

#endif