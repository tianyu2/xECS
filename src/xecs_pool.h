namespace xecs::pool
{
    constexpr static std::uint32_t invalid_delete_global_index_v = 0xffffffffu >> 1;

    struct instance final
    {
                        instance                            ( void 
                                                            ) noexcept = default;
                        instance                            ( const instance& 
                                                            ) noexcept = delete;
        inline
                       ~instance                            ( void 
                                                            ) noexcept;
        inline
        void            Initialize                          ( std::span<const component::info* const > Span 
                                                            ) noexcept;
        inline
        void            Clear                               ( void 
                                                            ) noexcept;
        inline
        int             Append                              ( int Count 
                                                            ) noexcept;
        inline
        void            Delete                              ( int Index
                                                            ) noexcept;
        inline
        void            MoveDelete                          ( int Index
                                                            ) noexcept;
        inline
        void            Free                                ( int   Index
                                                            , bool  bCallDestructors
                                                            ) noexcept;
        constexpr
        int             Size                                ( void 
                                                            ) const noexcept;
        constexpr
        int             findIndexComponentFromGUID          ( xecs::component::type::guid Guid
                                                            ) const noexcept;
        constexpr
        int             findIndexComponentFromGUIDInSequence( xecs::component::type::guid Guid
                                                            , int&                        Sequence 
                                                            ) const noexcept;
        inline
        void            UpdateStructuralChanges             ( xecs::game_mgr::instance& GameMgr
                                                            ) noexcept;
        constexpr
        bool            isLastEntry                         ( int Index 
                                                            ) const noexcept;
        inline
        int             MoveInFromPool                      ( int               IndexToMove
                                                            , pool::instance&   Pool
                                                            ) noexcept;
        template
        < typename T_COMPONENT
        > requires
        ( std::is_same_v<T_COMPONENT, std::decay_t<T_COMPONENT>>
        )
        T_COMPONENT&    getComponent                        ( std::uint32_t EntityIndex 
                                                            ) const noexcept;
        template
        < typename T_COMPONENT
        > requires
        ( std::is_same_v<T_COMPONENT, std::decay_t<T_COMPONENT>>
        )
        T_COMPONENT&    getComponentInSequence              ( std::uint32_t EntityIndex
                                                            , int&          Sequence
                                                            ) const noexcept;

        std::span<const component::info* const >                            m_Infos             {};
        int                                                                 m_CurrentCount      {};
        int                                                                 m_Size              {};
        std::uint32_t                                                       m_DeleteGlobalIndex { invalid_delete_global_index_v };
        std::uint32_t                                                       m_DeleteMoveIndex   { invalid_delete_global_index_v };
        std::array<std::byte*, xecs::settings::max_components_per_entity_v> m_pComponent        {};
    };
}
