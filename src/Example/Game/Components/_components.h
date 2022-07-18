
#include "bullet_data.h"
#include "position_data.h"
#include "velocity_data.h"
#include "timer_data.h"
#include "grid_cell_share.h"
#include "Player.h"

using bullet_tuple = std::tuple<position, velocity, timer, bullet, grid_cell>;

using player_tuple = std::tuple<position, velocity, bullet, Player>;