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
        int             getPageFromIndex                    ( const component::info& Info
                                                            , int                    iEntity 
                                                            ) noexcept;
        inline
        void            Initialize                          ( std::span<const component::info* const > Span 
                                                            ) noexcept;
        inline
        void            Clear                               ( void 
                                                            ) noexcept;
        inline
        int             Append                              ( void 
                                                            ) noexcept;
        inline
        void            Delete                              ( int Index 
                                                            ) noexcept;
        constexpr
        int             Size                                ( void 
                                                            ) const noexcept;
        constexpr
        int             findIndexComponentFromUIDComponent  ( std::uint16_t UIDComponent 
                                                            ) const noexcept;
        template
        < typename T_COMPONENT
        >
        T_COMPONENT&    getComponent                        ( std::uint32_t EntityIndex 
                                                            ) const noexcept;

        std::span<const component::info* const >                            m_Infos         {};
        int                                                                 m_Size          {};
        std::array<std::byte*, xecs::settings::max_components_per_entity_v> m_pComponent    {};
    };
}
