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
    template< typename... T_COMPONENTS > constexpr
    void bits::AddFromComponents( void ) noexcept
    {
        ((setBit(xecs::component::info_v<T_COMPONENTS>.m_BitID)), ...);
    }
}
