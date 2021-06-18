namespace xecs::pool
{
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
        constexpr
        int             Size                                ( void 
                                                            ) const noexcept;
        constexpr
        int             findIndexComponentFromGUID          ( xecs::component::info::guid Guid
                                                            ) const noexcept;
        constexpr
        int             findIndexComponentFromGUIDInSequence( xecs::component::info::guid Guid
                                                            , int&          Sequence 
                                                            ) const noexcept;
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

        std::span<const component::info* const >                            m_Infos         {};
        int                                                                 m_Size          {};
        std::array<std::byte*, xecs::settings::max_components_per_entity_v> m_pComponent    {};
    };
}
