namespace xecs::pool
{
    constexpr static std::uint32_t invalid_delete_global_index_v = 0xffffffffu >> 1;

    namespace type
    {
        using guid = xcore::guid::unit<64, struct pool_guid_tag>;
    }

    //------------------------------------------------------------------------------
    // POOL INSTANCE
    //------------------------------------------------------------------------------
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
        void            Initialize                          ( std::span<const component::type::info* const > DataComponentInfosSpan
                                                            ) noexcept;
        inline
        void            Clear                               ( void 
                                                            ) noexcept;
        inline
        index           Append                              ( int Count
                                                            ) noexcept;
        inline
        void            Delete                              ( index Index
                                                            ) noexcept;
        inline
        index           MoveInFromPool                      ( index             ToNewIndex
                                                            , index             FromIndexToMove
                                                            , pool::instance&   FromPool
                                                            ) noexcept;
        constexpr
        int             Size                                ( void 
                                                            ) const noexcept;
        constexpr
        int             findIndexComponentFromInfo          ( const xecs::component::type::info&
                                                            ) const noexcept;
        constexpr
        int             findIndexComponentFromInfoInSequence( const xecs::component::type::info&
                                                            , int&                        Sequence 
                                                            ) const noexcept;

        inline
        void            UpdateStructuralChanges             ( xecs::component::mgr& ComponentMgr
                                                            ) noexcept;
        constexpr
        int             getFreeSpace                        ( void
                                                            ) const noexcept;
        constexpr
        bool            isLastEntry                         ( index Index
                                                            ) const noexcept;
        template
        < typename T_COMPONENT
        > requires
        ( std::is_same_v<T_COMPONENT, std::decay_t<T_COMPONENT>>
        )
        T_COMPONENT&    getComponent                        ( index EntityIndex 
                                                            ) const noexcept;
        template
        < typename T_COMPONENT
        > requires
        ( std::is_same_v<T_COMPONENT, std::decay_t<T_COMPONENT>>
        )
        T_COMPONENT&    getComponentInSequence              ( index         EntityIndex
                                                            , int&          Sequence
                                                            ) const noexcept;
    private:
        inline
        bool            Free                                ( index Index
                                                            , bool  bCallDestructors
                                                            ) noexcept;
    public:

        std::span<const component::type::info* const >                      m_ComponentInfos        {};
        int                                                                 m_CurrentCount          {};
        int                                                                 m_Size                  {};
        std::uint32_t                                                       m_DeleteGlobalIndex     { invalid_delete_global_index_v };
        std::uint32_t                                                       m_DeleteMoveIndex       { invalid_delete_global_index_v };
        std::array<std::byte*, xecs::settings::max_components_per_entity_v> m_pComponent            {};
        std::int8_t                                                         m_ProcessReference      {};
        std::unique_ptr<instance>                                           m_Next                  {};
    };

    //------------------------------------------------------------------------------
    // POOL FAMILY INSTANCE
    //------------------------------------------------------------------------------
    struct family
    {
        using guid = xcore::guid::unit<64, struct family_pool_guid_tag>;

        constexpr
        static guid ComputeGuid(  xecs::archetype::guid                                 Guid
                                , std::span<const xecs::component::type::info* const>   Types
                                , std::span<const std::byte* const>                     Data
                                ) noexcept;

        void Initialize
        ( guid                                              Guid
        , std::span<xecs::component::entity>                ShareEntityList
        , std::span<xecs::component::type::share::key>      ShareKeyList
        , std::span<const xecs::component::type::info*>     ShareInfoList
        , std::span<const xecs::component::type::info*>     DataInfoList
        ) noexcept;

        template
        < typename T_FUNCTION
        >
        void AppendEntities( int Count, xecs::component::mgr& ComponentMgr, T_FUNCTION&& Function ) noexcept;

        struct share_details
        {
            xecs::component::entity             m_Entity;
            xecs::component::type::share::key   m_Key;
        };

        using share_details_array = std::array<share_details, xecs::settings::max_components_per_entity_v>;

        guid                                            m_Guid              {};
        instance                                        m_DefaultPool       {};
        std::span<const xecs::component::type::info*>   m_ShareInfos        {};
        std::unique_ptr<xecs::pool::family>             m_Next              {};
        xecs::pool::family*                             m_pPrev             {};
        share_details_array                             m_ShareDetails      {};
    };

    //------------------------------------------------------------------------------
    // ACCESS GUARD FOR POOLS INSTANCES
    //------------------------------------------------------------------------------
    struct access_guard
    {
        access_guard() = delete;
        access_guard( instance& Instance, xecs::component::mgr& ComponentMgr)
            : m_Instance{ Instance }
            , m_ComponentMgr{ ComponentMgr }
        {
            ++m_Instance.m_ProcessReference;
        }

        ~access_guard()
        {
            if( --m_Instance.m_ProcessReference == 0 )
                m_Instance.UpdateStructuralChanges(m_ComponentMgr);
        }

        instance&               m_Instance;
        xecs::component::mgr&   m_ComponentMgr;
    };
}
