//---------------------------------------------------------------------------------------
// TOOLS
//--------------------------------------------------\/-----------------------------------
//                                                  //
//       +----+----+----+----+----+----+----+----+  //
//       |  -1,-1  |   0,-1  |   1,-1  |    :    |  //
//  +---------+---------+---------+---------+----+  //
//  |  -1,0   |   0,0   |   1,0   |   2,0   |       //
//  +----+----+----+----+----+----+----+----+----+  //
//       |  -1,1   |   0,1   |   1,1   |    :    |  //
//  +----+----+----+----+----+----+----+----+----+  //
//  |  -1,2   |   0,2   |   1,2   |   2,2   |       //
//  +----+----+----+----+----+----+----+----+----+  //
//       |  -1,3   |   0,3   |   1,3   |    :    |  //
//       +----+----+----+----+----+----+----+----+  //
//                                                  //
//--------------------------------------------------/\-----------------------------------
namespace grid
{
    constexpr int   cell_width_v               = 64; // Keep this divisible by 2
    constexpr int   cell_height_v              = 42; // Keep this divisible by 2
    constexpr int   max_resolution_width_v     = 1024;
    constexpr int   max_resolution_height_v    = 800;
    constexpr auto  cell_x_count               = static_cast<std::int16_t>(max_resolution_width_v /cell_width_v  + 1);
    constexpr auto  cell_y_count               = static_cast<std::int16_t>(max_resolution_height_v/cell_height_v + 1);

    //---------------------------------------------------------------------------------------

    template<typename T_FUNCTION>
    constexpr __inline
    bool Search
    ( xecs::system::instance&               System
    , const xecs::component::share_filter&  ShareFilter
    , const std::int16_t                    X
    , const std::int16_t                    Y
    , const xecs::query::instance&          Query
    , T_FUNCTION&&                          Function 
    ) noexcept
    {
        static constexpr auto Table = std::array< std::int16_t, 2*6 >
        { -1, 0
        , -1, 1
        , -1, 0
        ,  0, 1
        , -1, 1
        ,  0, 1
        };

        //
        // Search self first 
        //
        if( System.Foreach(ShareFilter, Query, std::forward<T_FUNCTION&&>(Function)) )
            return true;

        //
        // Search neighbors 
        //
        int i = (Y&1)*(2*3);
        for( std::int16_t y = std::max(0,Y-1), end_y = std::min(cell_y_count-1, Y+1); y != end_y; ++y )
        {
            if( auto pShareFilter = System.findShareFilter(grid_cell{ .m_X = X + Table[i + 0], .m_Y = y }); 
                     pShareFilter && System.Foreach
                    ( *pShareFilter
                    , Query
                    , std::forward<T_FUNCTION&&>(Function)
                    )
                )
                return true;

            if( auto pShareFilter = System.findShareFilter(grid_cell{ .m_X = X + Table[i + 1], .m_Y = y }); 
                    pShareFilter && System.Foreach
                    ( *pShareFilter
                    , Query
                    , std::forward<T_FUNCTION&&>(Function)
                    )
                )
                return true;

            i+=2;
        }
        return false;
    }

    //---------------------------------------------------------------------------------------
    // This function will map between word space to the grid space. However it uses an imaginary grid
    // to help map between both. The imaginary grid is just a regular grid nothing special but not real.
    // Not real in the sense it does not get use for anything just for mapping reasons. The imaginary
    // Grid is twice the resolution in the X direction. Given that then we just need to find a formula to
    // Map between this imaginary yet regular grid to the Actual Tiled grid.
    //
    // Imaginary Grid--> |0 |1 |2 |3 |4 |5 |6 |7 |8 |9 |10|11|12|13|14|15|16|17|18|19|20|21|
    // RealGrid          +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    //                   +  0  +  1  +  2  +  3  +  4  +  5  +  6  +  7  +  8  +  9  +  10 +
    //                +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    //                + -1  +  0  +  1  +  2  +  3  +  4  +  5  +  6  +  7  +  8  +  9  +
    //                +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    //
    // To map between the Imaginary grid we use the following formulas:
    //
    //  For Odd  Values of Y:  X_Real = (X_img-(1-(X_img&1)))/2
    //  For Even Values of Y:  X_Real = (X_img/2)
    //
    // To remove the condition we can use the following trick
    //
    // X_Real = (X_img - (1 & ((X_img ^ Y_img) & Y_img)) / 2
    //
    // In the code below we add a 1 to the final result to avoid the coordinates to be negative
    // However in the normal case we would remove that "1 + " from our equation.
    //---------------------------------------------------------------------------------------
    __inline constexpr
    grid_cell ComputeGridCellFromWorldPosition( const xcore::vector2 Position ) noexcept
    {
        const auto X = static_cast<int>(Position.m_X / (cell_width_v /2.0f));
        const auto Y = std::max(0, std::min(static_cast<int>(Position.m_Y / cell_height_v), cell_y_count - 1));
        const auto x = 1 & ((X ^ Y) & Y);
        return
        { static_cast<std::int16_t>(std::max(0, std::min( 1 + ((X - x) >> 1), cell_x_count - 1 )))
        , static_cast<std::int16_t>(Y)
        };
    }
}
