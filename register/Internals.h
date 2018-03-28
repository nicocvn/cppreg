//! Internals implementation.
/**
 * @file      Internals.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 *
 * This header collects various implementations which are required for cppreg
 * implementation but not intended to be fully exposed to the user.
 */


#include "cppreg_Defines.h"
#include "Traits.h"


#ifndef CPPREG_INTERNALS_H
#define CPPREG_INTERNALS_H


//! cppreg::internals namespace.
namespace cppreg {
namespace internals {


    //! Overflow check implementation.
    /**
     * @tparam T Data type.
     * @tparam value Value to check.
     * @tparam limit Overflow limit value.
     *
     * This structure defines a type result set to std::true_type if there is
     * no overflow and set to std::false_type if there is overflow.
     * There is overflow if value if strictly larger than limit.
     */
    template <
        typename T,
        T value,
        T limit
    >
    struct check_overflow : std::integral_constant<bool, value <= limit> {};


    //! is_aligned implementation.
    /**
     * @tparam address Address to be checked for alignment.
     * @tparam alignment Alignment boundary in bytes.
     *
     * This will only derived from std::true_type if the address is aligned.
     */
    template <Address_t address, std::size_t alignment>
    struct is_aligned : std::integral_constant<
        bool,
        (address & (alignment - 1)) == 0
                                              > {
    };


    //! Memory map implementation for packed registers.
    /**
     * @tparam address Memory region base address.
     * @tparam n Memory size in bytes.
     * @tparam reg_size Register bit size enum value.
     *
     * This structure is used to map an array structure onto a memory region.
     * The size of the array elements is defined by the register size.
     */
    template <Address_t address, std::uint32_t n, RegBitSize reg_size>
    struct memory_map {

        //! Array type.
        using mem_array_t = std::array<
            volatile typename TypeTraits<reg_size>::type,
            n / sizeof(typename TypeTraits<reg_size>::type)
                                      >;

        //! Static reference to memory array.
        static mem_array_t& array;

        // Alignment check.
        static_assert(
            is_aligned<address, TypeTraits<reg_size>::byte_size>::value,
            "memory_map: base address is mis-aligned for register type"
                     );

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


}
}


#endif  // CPPREG_INTERNALS_H
