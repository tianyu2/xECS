struct bullet_logic : xecs::system::instance
{
    constexpr static auto typedef_v = xecs::system::type::update
    {
        .m_pName = "bullet_logic"
    };

    xecs::query::instance m_QueryBullets;
    xecs::query::instance m_QueryAny;

    using query = std::tuple
    <
        xecs::query::must<bullet>
    >;

    void OnGameStart( void ) noexcept
    {
        m_QueryBullets.AddQueryFromTuple<query>();
        m_QueryAny.m_Must.AddFromComponents<position>();
    }

    __inline 
    void OnUpdate( void ) noexcept
    {
        //
        // Update all the bullets
        //
        for( std::int16_t Y=0; Y<grid::cell_y_count; ++Y )
        for( std::int16_t X=0; X<grid::cell_x_count; ++X )
        {
            auto pShareFilter = findShareFilter(grid_cell{ .m_X = X, .m_Y = Y });
            if( pShareFilter == nullptr ) continue;

            Foreach( *pShareFilter, m_QueryBullets, [&]( entity& Entity, const position& Position, const bullet& Bullet ) constexpr noexcept
            {
                // If I am dead because some other bullet killed me then there is nothing for me to do...
                if (Entity.isZombie()) return;

                grid::Search( *this, *pShareFilter, X, Y, m_QueryAny, [&]( entity& E, const position& Pos )  constexpr noexcept
                {
                    if (E.isZombie()) return false;

                    // Our we checking against my self?
                    if ( Entity == E ) return false;

                    // Are we colliding with our own ship?
                    // If so lets just continue
                    if( Bullet.m_ShipOwner == E ) return false;

                    if (constexpr auto distance_v = 3; (Pos.m_Value - Position.m_Value).getLengthSquared() < distance_v * distance_v)
                    {
                        DeleteEntity(Entity);
                        DeleteEntity(E);
                        return true;
                    }

                    return false;
                });
            });
        }
    }
};
