struct update_movement : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::update
    {
        .m_pName = "update_movement"
    };

    __inline 
    void operator()( position& Position, velocity& Velocity, grid_cell& GridCell ) const noexcept
    {
        Position.m_Value += Velocity.m_Value;

        // Bounce on edges
        if (Position.m_Value.m_X < 0)
        {
            Position.m_Value.m_X = 0;
            Velocity.m_Value.m_X = -Velocity.m_Value.m_X;
        }
        else if (Position.m_Value.m_X >= grid::max_resolution_width_v )
        {
            Position.m_Value.m_X = grid::max_resolution_width_v - 1;
            Velocity.m_Value.m_X = -Velocity.m_Value.m_X;
        }

        if (Position.m_Value.m_Y < 0)
        {
            Position.m_Value.m_Y = 0;
            Velocity.m_Value.m_Y = -Velocity.m_Value.m_Y;
        }
        else if (Position.m_Value.m_Y >= grid::max_resolution_height_v )
        {
            Position.m_Value.m_Y = grid::max_resolution_height_v - 1;
            Velocity.m_Value.m_Y = -Velocity.m_Value.m_Y;
        }

        // Update the grid cell base on our new position
        GridCell = grid::ComputeGridCellFromWorldPosition(Position.m_Value);
    }
};
