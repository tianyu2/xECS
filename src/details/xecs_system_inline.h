namespace xecs::system
{
    namespace details
    {
        template< typename T_USER_SYSTEM >
        requires( std::derived_from< T_USER_SYSTEM, xecs::system::instance > )
        struct compleated final : T_USER_SYSTEM
        {
            __inline
            compleated(xecs::game_mgr::instance& GameMgr) noexcept
            : T_USER_SYSTEM{ GameMgr }
            {}
            compleated( void ) noexcept = delete;

            __inline
            void Run( void ) noexcept
            {
                XCORE_PERF_ZONE_SCOPED_N(T_USER_SYSTEM::name_v)
                if constexpr (&T_USER_SYSTEM::OnUpdate != &instance::OnUpdate)
                {
                    T_USER_SYSTEM::OnUpdate();
                }
                else
                {
                    xecs::query::instance Query;
                    Query.AddQueryFromTuple(xcore::types::null_tuple_v< T_USER_SYSTEM::query >);
                    Query.AddQueryFromFunction(*this);
                    T_USER_SYSTEM::m_GameMgr.Foreach(T_USER_SYSTEM::m_GameMgr.Search(Query), *this);
                }
            }
        };
    }

    //-------------------------------------------------------------------------------------------

    instance::instance(xecs::game_mgr::instance& G) noexcept
    : m_GameMgr(G)
    {}

    //-------------------------------------------------------------------------------------------
    template
    < typename T_SYSTEM
    > requires( std::derived_from< T_SYSTEM, xecs::system::instance> )
    T_SYSTEM& mgr::RegisterSystem( xecs::game_mgr::instance& GameMgr ) noexcept
    {
        m_Systems.push_back
        (
            info
            {
                .m_System   = std::make_unique< details::compleated<T_SYSTEM> >(GameMgr)
            ,   .m_CallRun  = []( xecs::system::instance& System ) noexcept
                {
                    static_cast<details::compleated<T_SYSTEM>&>(System).Run();
                }
            ,   .m_pName    = T_SYSTEM::name_v
            }
        );

        return *static_cast<T_SYSTEM*>(m_Systems.back().m_System.get());
    }

    //---------------------------------------------------------------------------

    void mgr::Run( void ) noexcept
    {
        for( const auto& S : m_Systems )
            S.m_CallRun( *S.m_System.get() );
    }
}