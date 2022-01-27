//! Memory device implementation.
/**
 * @file      Memory.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2022 Sendyne Corp. All rights reserved.
 */


#ifndef CPPREG_DEV_MEMORY_H
#define CPPREG_DEV_MEMORY_H


#include "Internals.h"
#include "Traits.h"

#include <array>


namespace cppreg {


//! Register pack base implementation.
/**
 * @tparam base_address Pack base address.
 * @tparam pack_byte_size Pack size in bytes.
 */
template <Address base_address, std::uint32_t pack_byte_size>
struct RegisterPack {

    //! Base address.
    constexpr static const Address pack_base = base_address;

    //! Pack size in bytes.
    constexpr static const std::uint32_t size_in_bytes = pack_byte_size;
};


//! Memory device.
/**
 * @tparam mem_address Address of the memory device.
 * @tparam mem_byte_size Memory device size in bytes.
 */
template <Address mem_address, std::size_t mem_byte_size>
struct MemoryDevice {

    //! Storage type.
    using MemStorage = std::array<volatile std::uint8_t, mem_byte_size>;

    //! Memory device storage.
    static MemStorage& _mem_storage;    // NOLINT

    //! Accessor.
    template <RegBitSize reg_size, std::size_t byte_offset>
    static const volatile typename TypeTraits<reg_size>::type& ro_memory() {

        // Check alignment.
        static_assert(
            internals::is_aligned<mem_address + byte_offset,
                                  std::alignment_of<typename TypeTraits<
                                      reg_size>::type>::value>::value,
            "MemoryDevice:: ro request not aligned");

        return *(reinterpret_cast<const volatile    // NOLINT
                                  typename TypeTraits<reg_size>::type*>(
            &_mem_storage[byte_offset]));
    }

    //! Modifier.
    template <RegBitSize reg_size, std::size_t byte_offset>
    static volatile typename TypeTraits<reg_size>::type& rw_memory() {

        // Check alignment.
        static_assert(
            internals::is_aligned<mem_address + byte_offset,
                                  std::alignment_of<typename TypeTraits<
                                      reg_size>::type>::value>::value,
            "MemoryDevice:: rw request not aligned");

        return *(    // NOLINTNEXTLINE
            reinterpret_cast<volatile typename TypeTraits<reg_size>::type*>(
                &_mem_storage[byte_offset]));
    }
};

//! Static reference definition.
template <Address a, std::size_t s>    // NOLINTNEXTLINE
typename MemoryDevice<a, s>::MemStorage& MemoryDevice<a, s>::_mem_storage =
    // NOLINTNEXTLINE
    *(reinterpret_cast<typename MemoryDevice<a, s>::MemStorage*>(a));


//! Register memory device for register pack.
/**
 *
 * @tparam pack_address Register pack address.
 * @tparam n_bytes Register pack size in bytes.
 */
template <typename RegisterPack>
struct RegisterMemoryDevice {
    using mem_device =    // NOLINT
        MemoryDevice<RegisterPack::pack_base, RegisterPack::size_in_bytes>;
};


}    // namespace cppreg


#endif    // CPPREG_DEV_MEMORY_H
