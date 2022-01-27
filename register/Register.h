//! Register type implementation.
/**
 * @file      Register.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2022 Sendyne Corp. All rights reserved.
 *
 * This header provides the definitions related to register implementation.
 */


#ifndef CPPREG_REGISTER_H
#define CPPREG_REGISTER_H


#include "Memory.h"
#include "MergeWrite.h"
#include "ShadowValue.h"


namespace cppreg {


//! Register data structure.
/**
 * @tparam reg_address Register address.
 * @tparam reg_size Register size enum value.
 * @tparam reset_value Register reset value (0x0 if unknown).
 * @tparam use_shadow shadow Boolean flag to enable shadow value.
 *
 * This data structure will act as a container for fields and is
 * therefore limited to a strict minimum. It only carries information
 * about the register base address, size, and reset value.
 * In practice, register are implemented by deriving from this class to
 * create custom types.
 */
template <Address reg_address,
          RegBitSize reg_size,
          typename TypeTraits<reg_size>::type reset_value = 0x0,
          bool use_shadow = false>
struct Register {

    //! Register base type.
    using type = typename TypeTraits<reg_size>::type;    // NOLINT

    //! MMIO pointer type.
    using MMIO = volatile type;

    //! Boolean flag for shadow value management.
    using shadow = Shadow<Register, use_shadow>;    // NOLINT

    //! Register base address.
    constexpr static auto base_address = reg_address;

    //! Register size in bits.
    constexpr static auto size = TypeTraits<reg_size>::bit_size;

    //! Register reset value.
    constexpr static auto reset = reset_value;

    //! Register pack for memory device.
    using pack = RegisterPack<base_address, size / one_byte>;    // NOLINT

    //! Memory modifier.
    /**
     * @return A reference to the writable register memory.
     */
    static MMIO& rw_mem_device() {
        using MemDevice = typename RegisterMemoryDevice<pack>::mem_device;
        return MemDevice::template rw_memory<reg_size, 0>();
    }

    //! Memory accessor.
    /**
     * @return A reference to the read-only register memory.
     */
    static const MMIO& ro_mem_device() {
        using MemDevice = typename RegisterMemoryDevice<pack>::mem_device;
        return MemDevice::template ro_memory<reg_size, 0>();
    }

    //! Merge write start function.
    /**
     * @tparam F Field on which to perform the first write operation.
     * @param value Value to be written to the field.
     * @return A merge write data structure to chain further writes.
     */
    template <typename F>
    static MergeWrite<typename F::parent_register, F::mask> merge_write(
        const typename F::type value) noexcept {
        const auto lhs = static_cast<type>(value << F::offset);
        return MergeWrite<typename F::parent_register, F::mask>::create(
            static_cast<type>(lhs & F::mask));
    }

    //! Merge write start function for constant value.
    /**
     * @tparam F Field on which to perform the first write operation.
     * @tparam value Value to be written to the field.
     * @return A merge write data structure to chain further writes.
     */
    template <typename F,
              type value,
              typename T = MergeWrite_tmpl<typename F::parent_register,
                                           F::mask,
                                           F::offset,
                                           value>>
    static T merge_write() noexcept {

        // Check overflow.
        static_assert(
            internals::check_overflow<type, value, (F::mask >> F::offset)>::
                value,
            "Register::merge_write<value>:: value too large for the field");

        return T::create();
    }

    // Sanity check.
    static_assert(size != 0, "Register:: register definition with zero size");

    // Enforce alignment.
    static_assert(internals::is_aligned<reg_address,
                                        TypeTraits<reg_size>::byte_size>::value,
                  "Register:: address is mis-aligned for register type");
};


}    // namespace cppreg


#endif    // CPPREG_REGISTER_H
