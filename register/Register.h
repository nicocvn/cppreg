//! Register type implementation.
/**
 * @file      Register.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 *
 * This header provides the definitions related to register implementation.
 */


#ifndef CPPREG_REGISTER_H
#define CPPREG_REGISTER_H


#include "Traits.h"
#include "MergeWrite.h"
#include "ShadowValue.h"


//! cppreg namespace.
namespace cppreg {


    //! Register data structure.
    /**
     * @tparam address Register address.
     * @tparam width Register total width (i.e., size).
     * @tparam reset Register reset value (0x0 if unknown).
     * @tparam shadow Boolean flag to enable shadow value (enabled if `true`).
     *
     * This data structure will act as a container for fields and is
     * therefore limited to a strict minimum. It only carries information
     * about the register base address, size, and reset value.
     * In practice, register are implemented by deriving from this class to
     * create custom types.
     */
    template <
        Address_t RegAddress,
        Width_t RegWidth,
        typename RegisterType<RegWidth>::type ResetValue = 0x0,
        bool UseShadow = false
    >
    struct Register {

        //! Register base type.
        using type = typename RegisterType<RegWidth>::type;

        //! MMIO pointer type.
        using MMIO_t = volatile type;

        //! Register base address.
        constexpr static const Address_t base_address = RegAddress;

        //! Register total width.
        constexpr static const Width_t size = RegWidth;

        //! Register reset value.
        constexpr static const type reset = ResetValue;

        //! Boolean flag for shadow value management.
        using shadow = Shadow<Register, UseShadow>;

        //! Memory modifier.
        /**
         * @return A reference to the writable register memory.
         */
        static MMIO_t& rw_mem_device() {
            return *(reinterpret_cast<MMIO_t* const>(base_address));
        };

        //! Memory accessor.
        /**
         * @return A reference to the read-only register memory.
         */
        static const MMIO_t& ro_mem_device() {
            return *(reinterpret_cast<const MMIO_t* const>(base_address));
        };

        //! Merge write start function.
        /**
         * @tparam F Field on which to perform the first write operation.
         * @param value Value to be written to the field.
         * @return A merge write data structure to chain further writes.
         */
        template <typename F>
        inline static MergeWrite<typename F::parent_register, F::mask>
        merge_write(const typename F::type value) noexcept {
            return
                MergeWrite<typename F::parent_register, F::mask>
                ::make(((value << F::offset) & F::mask));
        };

        //! Merge write start function for constant value.
        /**
         * @tparam F Field on which to perform the first write operation.
         * @tparam value Value to be written to the field.
         * @return A merge write data structure to chain further writes.
         */
        template <
            typename F,
            type value,
            typename T = MergeWrite_tmpl<
                typename F::parent_register,
                F::mask,
                F::offset,
                value
                                        >
        >
        inline static
        typename std::enable_if<
            internals::check_overflow<
                size, value, (F::mask >> F::offset)
                                     >::result::value,
            T
                               >::type&&
        merge_write() noexcept {
            return std::move(T::make());
        };

        // Sanity check.
        static_assert(RegWidth != 0u,
                      "defining a Register type of width 0u is not allowed");

    };


    //! Register pack base implementation.
    /**
     * @tparam PackBase Base address to use for the pack memory.
     * @tparam PackByteSize Size in bytes of the memory region for the pack.
     */
    template <
        Address_t PackBase,
        std::uint32_t PackByteSize
    > struct RegisterPack {

        //! Type alias for byte array.
        using mem_array_t = std::array<volatile std::uint8_t, PackByteSize>;

        //! Base address.
        constexpr static const Address_t pack_base = PackBase;

        //! Pack size in bytes.
        constexpr static const std::uint32_t size_in_bytes = PackByteSize;

        //! Reference to the byte array.
        /**
         * This is explicitly defined below.
         */
        static mem_array_t& _mem_array;

    };

    //! RegisterPack byte array reference definition.
    template <
        Address_t PackBase,
        std::uint32_t PackByteSize
    >
    typename RegisterPack<PackBase, PackByteSize>::mem_array_t&
        RegisterPack<PackBase, PackByteSize>::_mem_array =
        *(
            reinterpret_cast<
                RegisterPack<PackBase, PackByteSize>::mem_array_t* const
                >(RegisterPack<PackBase, PackByteSize>::pack_base)
        );


    //! Packed register implementation.
    /**
     * @tparam RegisterPack Pack to which the register belongs.
     * @tparam RegWidth Register total width in bits.
     * @tparam OffsetInPack Register offset in the pack in bytes.
     * @tparam reset Register reset value (0x0 if unknown).
     * @tparam shadow Boolean flag to enable shadow value (enabled if `true`).
     *
     * This implementation is intended to be used when defining a register
     * that belongs to a type.
     */
    template <
        typename RegisterPack,
        Width_t RegWidth,
        std::uint32_t OffsetInPack,
        typename RegisterType<RegWidth>::type ResetValue = 0x0,
        bool UseShadow = false
    >
    struct PackedRegister :
    Register<
        RegisterPack::pack_base + OffsetInPack * 8,
        RegWidth,
        ResetValue,
        UseShadow
            > {

        //! Register type.
        using base_reg = Register<
            RegisterPack::pack_base + OffsetInPack * 8,
            RegWidth,
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
        static_assert((OffsetInPack + (RegWidth / 8u)) <=
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


}


#endif  // CPPREG_REGISTER_H
