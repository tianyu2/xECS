namespace xecs::event
{
    //---------------------------------------------------------------------------
    template
    < typename...   T_ARGS
    > template
    < auto          T_FUNCTION_PTR_V
    , typename      T
    > 
    void instance<T_ARGS...>::Register( T& Class ) noexcept
    {
        m_Delegates.push_back
        (
            info
            {
                .m_pCallback = [](void* pPtr, T_ARGS... Args)
                {
                    std::invoke(T_FUNCTION_PTR_V, reinterpret_cast<T*>(pPtr), std::forward<T_ARGS>(Args)...);
                }
            ,  .m_pClass = &Class
            }
        );
    }

    //---------------------------------------------------------------------------
    template
    < typename...T_ARGS
    > constexpr
    void instance<T_ARGS...>::NotifyAll(T_ARGS... Args) const noexcept
    {
        for (auto& E : m_Delegates)
        {
            E.m_pCallback(E.m_pClass, std::forward<T_ARGS>(Args)...);
        }
    }
}
