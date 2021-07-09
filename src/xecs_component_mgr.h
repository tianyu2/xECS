namespace xecs::component
{
    // https://graphics.stanford.edu/~seander/bithacks.html
    //
    // Count the consecutive zero bits(trailing) on the right with multiplyand lookup
    //     unsigned int v;  // find the number of trailing zeros in 32-bit v 
    // int r;           // result goes here
    // static const int MultiplyDeBruijnBitPosition[32] =
    // {
    //   0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
    //   31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    // };
    // r = MultiplyDeBruijnBitPosition[((uint32_t)((v & -v) * 0x077CB531U)) >> 27];
    // Converting bit vectors to indices of set bits is an example use for this.It requires one more operation than the earlier one involving modulus division, but the multiply may be faster.The expression(v & -v) extracts the least significant 1 bit from v.The constant 0x077CB531UL is a de Bruijn sequence, which produces a unique pattern of bits into the high 5 bits for each possible bit position that it is multiplied against.When there are no bits set, it returns 0. More information can be found by reading the paper Using de Bruijn Sequences to Index 1 in a Computer Word by Charles E.Leiserson, Harald Prokof, and Keith H.Randall.
    //     On October 8, 2005 Andrew Shapira suggested I add this.Dustin Spicuzza asked me on April 14, 2009 to cast the result of the multiply to a 32 - bit type so it would work when compiled with 64 - bit ints.
    //
    // Can use intrinsic: https://docs.microsoft.com/en-us/cpp/intrinsics/bitscanreverse-bitscanreverse64?view=msvc-160
    //
    // std::countr_zero
    
    //
    // MGR
    //
    struct mgr final
    {
        inline
                                            mgr                     ( void 
                                                                    ) noexcept;
        template
        < typename T_COMPONENT
        > requires
        ( xecs::component::type::is_valid_v<T_COMPONENT>
        )
        void                                RegisterComponent       ( void
                                                                    ) noexcept;
        inline
        const entity::global_info&          getEntityDetails        ( xecs::component::entity Entity 
                                                                    ) const noexcept;
        inline
        void                                DeleteGlobalEntity      ( std::uint32_t              GlobalIndex
                                                                    , xecs::component::entity&   SwappedEntity 
                                                                    ) noexcept;
        inline
        void                                DeleteGlobalEntity      ( std::uint32_t GlobalIndex
                                                                    ) noexcept;
        inline
        void                                MovedGlobalEntity       ( xecs::pool::index         PoolIndex
                                                                    , xecs::component::entity&  SwappedEntity
                                                                    ) noexcept;
        inline 
        entity                              AllocNewEntity          ( pool::index                   PoolIndex
                                                                    , xecs::archetype::instance&    Archetype
                                                                    , xecs::pool::instance&         Pool 
                                                                    ) noexcept;

        using bits_to_info_array = std::array<const xecs::component::type::info*, xecs::settings::max_component_types_v>;

        std::unique_ptr<entity::global_info[]>              m_lEntities         = std::make_unique<entity::global_info[]>(xecs::settings::max_entities_v);
        int                                                 m_EmptyHead         = 0;

        inline static xecs::tools::bits                     s_ShareBits         {};
        inline static xecs::tools::bits                     s_DataBits          {};
        inline static xecs::tools::bits                     s_TagsBits          {};
        inline static xecs::tools::bits                     s_ExclusiveTagsBits {};
        inline static int                                   s_UniqueID          = 0;
        inline static bits_to_info_array                    s_BitsToInfo        {};
        inline static int                                   s_nTypes            = 0;
    };
}