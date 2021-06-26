namespace xecs::archetype
{
    using guid = xcore::guid::unit<64, struct archetype_tag>;

    template< typename...T_TUPLES_OF_COMPONENTS_OR_COMPONENTS >
    constexpr static auto guid_v = []<typename...T>(std::tuple<T...>*) consteval
    {
        static_assert( ((xecs::component::type::is_valid_v<T>) && ... ) );
        return guid{ ((xecs::component::type::info_v<T>.m_Guid.m_Value) + ...) };
    }( xcore::types::null_tuple_v< xecs::tools::united_tuple<T_TUPLES_OF_COMPONENTS_OR_COMPONENTS...> > );


    struct instance final
    {
        struct events
        {
            xecs::event::instance<xecs::component::entity&>      m_OnEntityCreated;
            xecs::event::instance<xecs::component::entity&>      m_OnEntityDestroyed;
            xecs::event::instance<xecs::component::entity&>      m_OnEntityMovedIn;
            xecs::event::instance<xecs::component::entity&>      m_OnEntityMovedOut;
            std::array<xecs::event::instance<xecs::component::entity&>, xecs::settings::max_components_per_entity_v> m_OnComponentUpdated;
        };

                                instance                ( const instance& 
                                                        ) = delete;
        inline                  instance                ( xecs::game_mgr::instance& GameMgr 
                                                        ) noexcept;
        
        inline
        void                    Initialize              ( std::span<const xecs::component::type::info* const> Infos
                                                        , const tools::bits&                                  Bits 
                                                        ) noexcept;
        
        template
        < typename T_CALLBACK = xecs::tools::empty_lambda
        > requires
        ( xcore::function::is_callable_v<T_CALLBACK>
            && std::is_same_v<typename xcore::function::traits<T_CALLBACK>::return_type, void>
        ) __inline
        xecs::component::entity CreateEntity            ( T_CALLBACK&& Function = xecs::tools::empty_lambda{}
                                                        ) noexcept;

        template
        < typename T_CALLBACK = xecs::tools::empty_lambda
        > requires
        ( xcore::function::is_callable_v<T_CALLBACK>
            && std::is_same_v<typename xcore::function::traits<T_CALLBACK>::return_type, void>
        ) __inline
        void                    CreateEntities          ( int Count, T_CALLBACK&& Function = xecs::tools::empty_lambda{}
                                                        ) noexcept;

        inline
        void                    DestroyEntity           ( xecs::component::entity& Entity
                                                        ) noexcept;
        
        inline
        void                    UpdateStructuralChanges ( void 
                                                        ) noexcept;
        template
        < typename T_FUNCTION = xecs::tools::empty_lambda
        > [[nodiscard]] xecs::component::entity
                                MoveInEntity            ( xecs::component::entity&  Entity
                                                        , T_FUNCTION&&              Function = xecs::tools::empty_lambda{} 
                                                        ) noexcept;

        using info_array = std::array<const xecs::component::type::info*, xecs::settings::max_components_per_entity_v >;

        xecs::game_mgr::instance&           m_GameMgr;
        xecs::tools::bits                   m_ComponentBits     {};
        xecs::pool::instance                m_Pool              {};
        std::uint8_t                        m_nComponents       {};
        events                              m_Events            {};
        info_array                          m_InfoData          {};
    };
}
