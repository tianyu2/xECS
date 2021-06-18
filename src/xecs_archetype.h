namespace xecs::archetype
{
    struct instance final
    {
                                instance                ( const instance& 
                                                        ) = delete;
        inline                  instance                ( xecs::game_mgr::instance& GameMgr 
                                                        ) noexcept;
        
        inline
        void                    Initialize              ( std::span<const xecs::component::info* const> Infos
                                                        , const tools::bits&                            Bits 
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
        < typename T_FUNCTION
        > requires
        ( xcore::function::is_callable_v<T_FUNCTION>
            && std::is_same_v<typename xcore::function::traits<T_FUNCTION>::return_type, void>
            && xcore::function::traits<T_FUNCTION>::arg_count_v == 0
        ) __inline
        void                    AccessGuard             ( T_FUNCTION&& Function
                                                        ) noexcept;


        using info_array = std::array<const xecs::component::info*, xecs::settings::max_components_per_entity_v >;
        constexpr static std::uint32_t invalid_delete_global_index_v = 0xffffffffu >> 1;

        xecs::game_mgr::instance&            m_GameMgr;
        xecs::tools::bits                    m_ComponentBits     {};
        xecs::pool::instance                 m_Pool              {};
        std::uint32_t                        m_DeleteGlobalIndex { invalid_delete_global_index_v };
        std::int8_t                          m_ProcessReference  { 0 };
        info_array                           m_InfoData          {};
    };
}
