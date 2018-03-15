//! Register pack implementation.
/**
 * @file      RegisterPack.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 *
 * This header provides the definitions related to register implementation.
 */


#include "Register.h"


#ifndef CPPREG_REGISTERPACK_H
#define CPPREG_REGISTERPACK_H


//! cppreg namespace.
namespace cppreg {


//! cppreg::internals namespace.
namespace internals {

    //! Memory map implementation.
    /**
     * @tparam address Memory base address.
     * @tparam n Memory size in bytes.
     * @tparam reg_size Register bit size for the array elements.
     *
     * This structure is used to map an array structure onto a memory region.
     * The width parameter will correspond to the register size.
     */
    template <Address_t address, std::uint32_t n, RegBitSize reg_size>
    struct memory_map {

        //! Array type.
        using mem_array_t = std::array<
            volatile typename TypeTraits<reg_size>::type,
            n / sizeof(typename TypeTraits<reg_size>::type)
                                      >;

        //! Static reference to memory array.
        /**
         * This is defined below.
         */
        static mem_array_t& array;

    };

    //! Memory array reference definition.
    template <Address_t address, std::uint32_t n, RegBitSize reg_size>
    typename memory_map<address, n, reg_size>::mem_array_t&
        memory_map<address, n, reg_size>::array = *(
            reinterpret_cast<typename memory_map<address, n, reg_size>
            ::mem_array_t*>(
                address
            )
        );


    //! is_aligned implementation.
    /**
     * @tparam address Address to be checked for alignment.
     * @tparam alignment Alignment constraint.
     */
    template <Address_t address, std::size_t alignment>
    struct is_aligned : std::integral_constant<
        bool,
        (address & (alignment - 1)) == 0
                                              > {
    };


}


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
     * @tparam BitOffset Offset in bits for the register with respect to base.
     * @tparam RegWidth Register width.
     * @tparam ResetValue Register reset value (0x0 if unknown).
     * @tparam UseShadow shadow Boolean flag to enable shadow value.
     *
     * This implementation is intended to be used when defining a register
     * that belongs to a peripheral group.
     */
    template <
        typename RegisterPack,
        std::uint32_t bit_offset,
        RegBitSize reg_size,
        typename TypeTraits<reg_size>::type reset_value = 0x0,
        bool use_shadow = false
    >
    struct PackedRegister :
        Register<
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
        static typename base_reg::MMIO_t& rw_mem_device() {
            return mem_map_t::array[bit_offset
                                    / TypeTraits<reg_size>::bit_size];
        };

        //! Memory accessor.
        /**
         * @return A reference to the read-only register memory.
         */
        static const typename base_reg::MMIO_t& ro_mem_device() {
            return mem_map_t::array[bit_offset
                                    / TypeTraits<reg_size>::bit_size];
        };

        // Safety check to detect if are overflowing the pack.
        static_assert((bit_offset / 8u) <= RegisterPack::size_in_bytes,
                      "packed register is overflowing the pack");

        // A packed register of width N bits requires:
        // - the pack address to be N-bits aligned (N/8 aligned),
        // - the pack address with offset to be N-bits aligned (N/8 aligned).
        static_assert(
            internals::is_aligned<
                RegisterPack::pack_base,
                TypeTraits<reg_size>::byte_size
                                 >::value
            &&
            internals::is_aligned<
                RegisterPack::pack_base + (bit_offset / 8),
                TypeTraits<reg_size>::byte_size
                                 >::value,
            "register is mis-aligned in the pack"
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
        using tuple_t = typename std::tuple<T...>;
        template <std::size_t N>
        using regs = typename std::tuple_element<N, tuple_t>::type;
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
        inline static void loop() {
            Func().template operator()<start>();
            if (start < end)
                for_loop<start + 1, end>::template loop<Func>();
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
        inline static void apply(Op& f) {
            if (start < end) {
                f(std::integral_constant<std::size_t, start>{});
                for_loop<start + 1, end>::apply(f);
            };
        };
#endif  // __cplusplus 201402L

    };
    template <std::size_t end>
    struct for_loop<end, end> {
        template <typename Func>
        inline static void loop() {};
#if __cplusplus >= 201402L
        template <typename Op>
        inline static void apply(Op& f) {};
#endif  // __cplusplus 201402L
    };


    //! Template range loop implementation.
    /**
     * @tparam IndexedPack Indexed pack type.
     */
    template <typename IndexedPack>
    struct range_loop : for_loop<
        0, std::tuple_size<typename IndexedPack::tuple_t>::value
                                > {};


}


#endif  // CPPREG_REGISTERPACK_H
