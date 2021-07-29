struct timer
{
    static xcore::err Serialize(xcore::textfile::stream& TextFile, bool, std::byte* pData) noexcept
    {
        auto& Timer = *reinterpret_cast<timer*>(pData);
        TextFile.Field("Value", Timer.m_Value).clear();
        return {};
    }


    constexpr static auto typedef_v = xecs::component::type::data
    {
        .m_pName = "Timer"
    ,   .m_pSerilizeFn = Serialize
    };

    float m_Value;
};
