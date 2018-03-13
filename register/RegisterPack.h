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


    //! Register pack base implementation.
    /**
     */
    template <
        Address_t PackBase,
        Width_t RegWidth,
        std::uint32_t N
    > struct RegisterPack {

        //! Pack register type.
        using register_type = typename RegisterType<RegWidth>::type;

        //! Type alias for byte array.
        using mem_array_t =std::array<volatile register_type, N>;

        //! Base address.
        constexpr static const Address_t pack_base = PackBase;

        //! Pack size in bytes.
        constexpr static const std::uint32_t size_in_bytes =
            N * sizeof(register_type);

        //! Register width in the pack.
        constexpr static const Width_t register_width = RegWidth;

        //! Reference to the byte array.
        /**
         * This is explicitly defined below.
         */
        static mem_array_t& _mem_array;

    };

    //! RegisterPack byte array reference definition.
    template <
        Address_t PackBase,
        Width_t RegWidth,
        std::uint32_t N
    >
    typename RegisterPack<PackBase, RegWidth, N>::mem_array_t&
        RegisterPack<PackBase, RegWidth, N>::_mem_array =
        *(
            reinterpret_cast<
                RegisterPack<PackBase, RegWidth, N>::mem_array_t* const
                >(
                RegisterPack<PackBase, RegWidth, N>::pack_base
            )
        );


    //! Packed register implementation.
    /**
     *
     * This implementation is intended to be used when defining a register
     * that belongs to a type.
     */
    template <
        typename RegisterPack,
        std::uint32_t OffsetInPack,
        typename RegisterType<RegisterPack::register_width>::type ResetValue = 0x0,
        bool UseShadow = false
    >
    struct PackedRegister :
        Register<
            RegisterPack::pack_base + OffsetInPack,
            RegisterPack::register_width,
            ResetValue,
            UseShadow
                > {

        //! Register type.
        using base_reg = Register<
            RegisterPack::pack_base + OffsetInPack,
            RegisterPack::register_width,
            ResetValue,
            UseShadow
                                 >;

        //! Memory modifier.
        /**
         * @return A reference to the writable register memory.
         */
        static typename base_reg::MMIO_t& rw_mem_device() {
            return RegisterPack::_mem_array[OffsetInPack];
        };

        //! Memory accessor.
        /**
         * @return A reference to the read-only register memory.
         */
        static const typename base_reg::MMIO_t& ro_mem_device() {
            return RegisterPack::_mem_array[OffsetInPack];
        };

        // Safety check to detect if are overflowing the pack.
        static_assert((OffsetInPack + (RegisterPack::register_width / 8u)) <=
                      RegisterPack::size_in_bytes,
                      "packed register is overflowing the pack");

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
        template <std::size_t N>
        using regs = typename std::tuple_element<N, std::tuple<T...>>::type;
    };


    //! Template for loop implementation.
    template <std::size_t start, std::size_t end>
    struct for_loop {
        template <template <std::size_t> class Op, typename T = void>
        inline static void iterate(
            typename std::enable_if<start < end, T>::type* = nullptr
                                  ) {
            Op<start>()();
            if (start < end)
                for_loop<start + 1, end>::template iterate<Op>();
        };
        template <template <std::size_t> class Op, typename T = void>
        inline static void iterate(
            typename std::enable_if<start >= end, T>::type* = nullptr
                                  ) {};
    };



}


#endif  // CPPREG_REGISTERPACK_H
