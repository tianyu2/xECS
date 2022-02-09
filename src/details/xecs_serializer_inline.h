namespace xecs::serializer
{
    template< std::size_t N, typename... T_ARGS >
    xcore::err stream::Field( const char(&pFieldName)[N], T_ARGS&&... Args ) noexcept
    {
        static_assert( ((std::is_same_v< xcore::types::decay_full_t<T_ARGS>, xecs::component::entity > == false) && ... ) );
        return xcore::textfile::stream::Field(pFieldName, std::forward<T_ARGS&&>(Args)... );
    }

    //-----------------------------------------------------------------------------------

    template< std::size_t N>
    xcore::err stream::Field( const char(&pFieldName)[N], xecs::component::entity& Entity ) noexcept
    {
        if (auto Err = xcore::textfile::stream::Field(pFieldName, Entity.m_Value); Err) return Err;
        if( isReading() ) Remap(Entity);
        return {};
    }
}