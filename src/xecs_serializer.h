namespace xecs::serializer
{
    struct stream : xcore::textfile::stream
    {
        template< std::size_t N, typename... T_ARGS >
        xforceinline  xcore::err    Field(const char(&pFieldName)[N], T_ARGS&&... Args)                             noexcept;

        template< std::size_t N>
        xforceinline  xcore::err    Field( const char(&pFieldName)[N], xecs::component::entity& Entity ) noexcept;

        virtual void Remap( xecs::component::entity& Entity ) noexcept = 0;
    };
}
