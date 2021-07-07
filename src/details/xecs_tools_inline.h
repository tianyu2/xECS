namespace xecs::tools
{
    //------------------------------------------------------------------------------------
    constexpr
    void bits::setBit(int Bit) noexcept
    {
        int x = Bit / 64;
        int y = Bit % 64;
        m_Bits[x] |= (1ull << y);
    }

    //------------------------------------------------------------------------------------
    constexpr
    void bits::clearBit(int Bit) noexcept
    {
        int x = Bit / 64;
        int y = Bit % 64;
        m_Bits[x] &= ~(1ull << y);
    }

    //------------------------------------------------------------------------------------
    constexpr
    bool bits::getBit(int Bit) const noexcept
    {
        int x = Bit / 64;
        int y = Bit % 64;
        return m_Bits[x] & (1ull << y);
    }

    //------------------------------------------------------------------------------------
    constexpr
    bool bits::Superset(const bits& B) const noexcept
    {
        for (int i = 0, size = static_cast<int>(m_Bits.size()); i < size; ++i)
        {
            if ((m_Bits[i] & B.m_Bits[i]) != B.m_Bits[i])
                return false;
        }

        return true;
    }

    //------------------------------------------------------------------------------------
    constexpr
    bool bits::Subset(const bits& B) const noexcept
    {
        return B.Superset(*this);
    }

    //------------------------------------------------------------------------------------
    constexpr
    bool bits::Equals(const bits& B) const noexcept
    {
        for (int i = 0, size = static_cast<int>(m_Bits.size()); i < size; ++i)
        {
            if (m_Bits[i] != B.m_Bits[i])
                return false;
        }

        return true;
    }

    //------------------------------------------------------------------------------------
    template< typename... T_COMPONENTS >
    requires( assert_valid_tuple_components_v < std::tuple<T_COMPONENTS...> > )
    constexpr
    void bits::AddFromComponents( void ) noexcept
    {
        ((setBit(xecs::component::type::info_v<T_COMPONENTS>.m_BitID)), ...);
    }

    //------------------------------------------------------------------------------------

    std::uint64_t bits::GenerateUniqueID( void ) const noexcept
    {
        std::hash<std::uint64_t> Hasher;
        const auto               Hashes = std::span{ reinterpret_cast<const std::uint64_t*>(this), sizeof(*this) / sizeof(std::uint64_t) };

        std::uint64_t Hash = Hasher(Hashes[0]);
        for (int i = 1; i < Hashes.size(); ++i)
        {
            Hash ^= Hasher(Hashes[i]) + 0x9e3779b9u + (Hash << 6) + (Hash >> 2);
        }

        return Hash;
    }

    //------------------------------------------------------------------------------------
    inline
    void bits::setupAnd( const bits& A, const bits& B ) noexcept
    {
        for( auto i = 0, size = static_cast<const int>(m_Bits.size()); i < size; ++i)
        {
            m_Bits[i] = A.m_Bits[i] & B.m_Bits[i];
        }
    }

}
