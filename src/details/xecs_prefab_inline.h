namespace xecs::prefab
{
    xcore::err root::Serialize(xecs::serializer::stream& TextFile, bool) noexcept
    {
        xcore::err Error;

        (   ( Error = TextFile.Field("Guid", m_Guid.m_Instance.m_Value) )
        ||  ( Error = TextFile.Field( "ParentGuid", m_ParentPrefabGuid.m_Instance.m_Value ) )
        );

        return Error;
    }
}