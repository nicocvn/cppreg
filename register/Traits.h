//! Register traits implementation.
/**
 * @file      Traits.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 *
 * This header provides some traits for register type instantiation.
 */


#ifndef CPPREG_TRAITS_H
#define CPPREG_TRAITS_H


#include "cppreg_Defines.h"


//! cppreg namespace.
namespace cppreg {


//    //! Register data type default implementation.
//    /**
//     * @tparam Size Register size.
//     *
//     * This will fail to compile if the register size is not implemented.
//     */
//    template <Width_t Size>
//    struct RegisterType;
//
//    //!@{ Specializations based on register size.
//    template <> struct RegisterType<8u> { using type = std::uint8_t; };
//    template <> struct RegisterType<16u> { using type = std::uint16_t; };
//    template <> struct RegisterType<32u> { using type = std::uint32_t; };
//    //!@}


    //! Register type traits based on size.
    /**
     * @tparam S Register size in bits.
     */
    template <RegBitSize S>
    struct TypeTraits;

    template <> struct TypeTraits<RegBitSize::b8> {
        using type = std::uint8_t;
        constexpr static const std::uint8_t bit_size = 8u;
        constexpr static const std::uint8_t byte_size = 1u;
        constexpr static const std::uint8_t max_field_width = 8u;
        constexpr static const std::uint8_t max_field_offset = 8u;
    };
    template <> struct TypeTraits<RegBitSize::b16> {
        using type = std::uint16_t;
        constexpr static const std::uint8_t bit_size = 16u;
        constexpr static const std::uint8_t byte_size = 2u;
        constexpr static const std::uint8_t max_field_width = 16u;
        constexpr static const std::uint8_t max_field_offset = 16u;
    };
    template <> struct TypeTraits<RegBitSize::b32> {
        using type = std::uint32_t;
        constexpr static const std::uint8_t bit_size = 32u;
        constexpr static const std::uint8_t byte_size = 4u;
        constexpr static const std::uint8_t max_field_width = 32u;
        constexpr static const std::uint8_t max_field_offset = 32u;
    };


}


#endif  // CPPREG_TRAITS_H
