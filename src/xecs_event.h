namespace xecs::event
{
    template< typename...T_ARGS >
    struct instance final
    {
        struct info
        {
            using callback = void(void* pPtr, T_ARGS...);
            callback*   m_pCallback;
            void*       m_pClass;
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
}
