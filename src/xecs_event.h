namespace xecs::event
{
    namespace type
    {
        using guid = xcore::guid::unit<64, struct event_tag>;

        struct info
        {
            type::guid  m_Guid;
            const char* m_pName;
        };

        namespace details
        {
            template< typename T >
            consteval type::info CreateInfo(void) noexcept;

            template< typename T >
            static constexpr auto info_v = CreateInfo<T>();
        }

        template< typename T_SYSTEM >
        constexpr static auto& info_v = details::info_v<T_SYSTEM>;

        struct global
        {
            const char* m_pName = "Unnamed Notified Move Out System";
            guid        m_Guid{};
        };
    }

    struct overrides
    {
        constexpr static auto   typedef_v = type::global{};
    };

    template< typename...T_ARGS >
    struct instance : overrides
    {
        struct info
        {
            using callback = void(void* pPtr, T_ARGS...);

            callback*       m_pCallback;
            void*           m_pClass;
            const char*     m_pName;
        };

                            instance                ( const instance&
                                                    ) = delete;
                            instance                ( void 
                                                    ) noexcept = default;
        template
        < auto      T_FUNCTION_PTR_V
        , typename  T_CLASS
        > __inline
        void                Register                ( T_CLASS& Class
                                                    ) noexcept;
        constexpr __inline
        void                NotifyAll               ( T_ARGS... Args 
                                                    ) const noexcept;
        template
        < typename T_CLASS
        >
        void                RemoveDelegate          ( T_CLASS& Class
                                                    ) noexcept;
        std::vector<info>   m_Delegates{};
    };

    struct mgr
    {
        template< typename T_EVENT, typename T_CLASS >
        void RegisterClass( T_CLASS&& Class ) noexcept
        {
            auto It = m_GlobalEventsMap.find( type::info_v<T_EVENT>.m_Guid );
            assert( It != m_GlobalEventsMap.end() );
            reinterpret_cast<T_EVENT*>(It->second)->Register<&T_CLASS::OnEvent>(Class);
        }

        template< typename T_EVENT >
        T_EVENT& getEvent( void ) const noexcept
        {
            auto It = m_GlobalEventsMap.find(type::info_v<T_EVENT>.m_Guid);
            assert(It != m_GlobalEventsMap.end());
            return *reinterpret_cast<T_EVENT*>(It->second);
        }

        template< typename T_EVENT >
        void Register( void ) noexcept
        {
            m_GlobalEvents.push_back( std::make_unique<T_EVENT>() );
            m_GlobalEventsMap.emplace( type::info_v<T_EVENT>.m_Guid, m_GlobalEvents.back().get() );
        }

        std::unordered_map<type::guid, overrides*>  m_GlobalEventsMap;
        std::vector<std::unique_ptr<overrides>>     m_GlobalEvents;
    };
}
