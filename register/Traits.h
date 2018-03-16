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


    //! Register type traits based on size.
    /**
     * @tparam S Register size in bits.
     */
    template <RegBitSize S>
    struct TypeTraits;


    //!@{ TypeTraits specializations.
    //! 8-bit specialization.
    template <> struct TypeTraits<RegBitSize::b8> {
        using type = std::uint8_t;
        constexpr static const std::uint8_t bit_size = 8u;
        constexpr static const std::uint8_t byte_size = bit_size / 8u;
    };
    //! 16-bit specialization.
    template <> struct TypeTraits<RegBitSize::b16> {
        using type = std::uint16_t;
        constexpr static const std::uint8_t bit_size = 16u;
        constexpr static const std::uint8_t byte_size = bit_size / 8u;
    };
    //! 32-bit specialization.
    template <> struct TypeTraits<RegBitSize::b32> {
        using type = std::uint32_t;
        constexpr static const std::uint8_t bit_size = 32u;
        constexpr static const std::uint8_t byte_size = bit_size / 8u;
    };
    //! 64-bit specialization.
    template <> struct TypeTraits<RegBitSize::b64> {
        using type = std::uint64_t;
        constexpr static const std::uint8_t bit_size = 64u;
        constexpr static const std::uint8_t byte_size = bit_size / 8u;
    };
    //!@}


}


#endif  // CPPREG_TRAITS_H
