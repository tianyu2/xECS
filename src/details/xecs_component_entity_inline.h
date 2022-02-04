namespace xecs::component
{
    inline
    xcore::err entity::Serialize(xecs::serializer::stream& TextFile, bool, std::byte* pData) noexcept
    {
        auto& Entity = *reinterpret_cast<entity*>(pData);
        return TextFile.Field( "Entity", Entity );
    }
}
