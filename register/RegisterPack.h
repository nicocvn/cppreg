//! Register pack implementation.
/**
 * @file      RegisterPack.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 *
 * This header provides the definitions related to register implementation.
 */


#include "Register.h"
#include "Internals.h"


#ifndef CPPREG_REGISTERPACK_H
#define CPPREG_REGISTERPACK_H


//! cppreg namespace.
namespace cppreg {


    //! Register pack base implementation.
    /**
     * @tparam base_address Pack base address.
     * @tparam pack_byte_size Pack size in bytes.
     */
    template <
        Address_t base_address,
        std::uint32_t pack_byte_size
    > struct RegisterPack {

        //! Base address.
        constexpr static const Address_t pack_base = base_address;

        //! Pack size in bytes.
        constexpr static const std::uint32_t size_in_bytes = pack_byte_size;

    };


    //! Packed register implementation.
    /**
     * @tparam RegisterPack Pack to which the register belongs.
     * @tparam reg_size Register size enum value.
     * @tparam bit_offset Offset in bits for the register with respect to base.
     * @tparam reset_value Register reset value (0x0 if unknown).
     * @tparam use_shadow Boolean flag to enable shadow value.
     *
     * This implementation is intended to be used when defining a register
     * that belongs to a peripheral group.
     */
    template <
        typename RegisterPack,
        RegBitSize reg_size,
        std::uint32_t bit_offset,
        typename TypeTraits<reg_size>::type reset_value = 0x0,
        bool use_shadow = false
    >
    struct PackedRegister : Register<
        RegisterPack::pack_base + (bit_offset / 8u),
        reg_size,
        reset_value,
        use_shadow
                                    > {

        //! Register type.
        using base_reg = Register<
            RegisterPack::pack_base + (bit_offset / 8u),
            reg_size,
            reset_value,
            use_shadow
                                 >;

        //! Memory map type.
        using mem_map_t = internals::memory_map<
            RegisterPack::pack_base,
            RegisterPack::size_in_bytes,
            reg_size
                                               >;

        //! Memory modifier.
        /**
         * @return A reference to the writable register memory.
         */
        inline static typename base_reg::MMIO_t& rw_mem_device() noexcept {
            return mem_map_t::array[bit_offset
                                    / TypeTraits<reg_size>::bit_size];
        };

        //! Memory accessor.
        /**
         * @return A reference to the read-only register memory.
         */
        inline static const typename base_reg::MMIO_t& ro_mem_device() noexcept {
            return mem_map_t::array[bit_offset
                                    / TypeTraits<reg_size>::bit_size];
        };

        // Safety check to detect if are overflowing the pack.
        static_assert(
            TypeTraits<reg_size>::byte_size + (bit_offset / 8u) <=
                RegisterPack::size_in_bytes,
            "PackRegister: packed register is overflowing the pack"
                     );

        // A packed register of width N bits requires:
        // - the pack address to be N-bits aligned (N/8 aligned),
        // - the pack address with offset to be N-bits aligned (N/8 aligned).
        static_assert(
            internals::is_aligned<
                RegisterPack::pack_base,
                TypeTraits<reg_size>::byte_size
                                 >::value,
            "PackedRegister: pack base address is mis-aligned for register type"
                     );
        static_assert(
            internals::is_aligned<
                RegisterPack::pack_base + (bit_offset / 8u),
                TypeTraits<reg_size>::byte_size
                                 >::value,
            "PackedRegister: offset address is mis-aligned for register type"
                     );


    };


    //! Pack indexing structure.
    /**
     * @tparam T List of types (registers or fields) to index.
     *
     * This can be used to conveniently map indices over packed registers.
     * The order in the variadic parameter pack will define the indexing
     * (starting at zero).
     */
    template <typename... T>
    struct PackIndexing {

        //! Tuple type.
        using tuple_t = typename std::tuple<T...>;

        //! Number of elements.
        constexpr static const std::size_t n_elems =
            std::tuple_size<tuple_t>::value;

        //! Element accessor.
        template <std::size_t N>
        using elem = typename std::tuple_element<N, tuple_t>::type;

    };


    //! Template for loop implementation.
    /**
     * @tparam start Start index value.
     * @tparam end End index value.
     */
    template <std::size_t start, std::size_t end>
    struct for_loop {

        //! Loop method.
        /**
         * @tparam Func Function to be called at each iteration.
         *
         * This will call Op for the range [start, end).
         */
        template <typename Func>
        inline static void apply() noexcept {
            Func().template operator()<start>();
            if (start < end)
                for_loop<start + 1ul, end>::template apply<Func>();
        };

#if __cplusplus >= 201402L
        //! Apply method.
        /**
         * @tparam Op Operator type to be called.
         *
         * This is only available with C++14 and up as this requires polymorphic
         * lambdas to be used in a somewhat useful manner.
         *
         * Typical example:
         * use lambda [](auto index) { index.value will be the loop index};
         */
        template <typename Op>
        inline static void apply(Op&& f) noexcept {
            if (start < end) {
                f(std::integral_constant<std::size_t, start>{});
                for_loop<start + 1ul, end>::apply(std::forward<Op>(f));
            };
        };
#endif  // __cplusplus 201402L

    };
    template <std::size_t end>
    struct for_loop<end, end> {
        template <typename Func>
        inline static void apply() noexcept {};
#if __cplusplus >= 201402L
        template <typename Op>
        inline static void apply(Op&& f) noexcept {};
#endif  // __cplusplus 201402L
    };


    //! Template range loop implementation.
    /**
     * @tparam IndexedPack Indexed pack type.
     */
    template <typename IndexedPack>
    struct pack_loop : for_loop<0, IndexedPack::n_elems> {};


}


#endif  // CPPREG_REGISTERPACK_H
