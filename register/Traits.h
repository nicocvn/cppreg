//! Register traits implementation.
/**
 * @file      Traits.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2022 Sendyne Corp. All rights reserved.
 *
 * This header provides some traits for register type instantiation.
 */


#ifndef CPPREG_TRAITS_H
#define CPPREG_TRAITS_H


#include "cppreg_Defines.h"


namespace cppreg {


//! Register type traits based on size.
/**
 * @tparam size Register size in bits.
 */
template <RegBitSize size>
struct TypeTraits {};


//!@{ TypeTraits specializations.

//! 8-bit specialization.
template <>
struct TypeTraits<RegBitSize::b8> {
    using type = std::uint8_t;    // NOLINT
    constexpr static auto bit_size = std::uint8_t{8};
    constexpr static auto byte_size = std::uint8_t{bit_size / 8};
};

//! 16-bit specialization.
template <>
struct TypeTraits<RegBitSize::b16> {
    using type = std::uint16_t;    // NOLINT
    constexpr static auto bit_size = std::uint8_t{16};
    constexpr static auto byte_size = std::uint8_t{bit_size / 8};
};

//! 32-bit specialization.
template <>
struct TypeTraits<RegBitSize::b32> {
    using type = std::uint32_t;    // NOLINT
    constexpr static auto bit_size = std::uint8_t{32};
    constexpr static auto byte_size = std::uint8_t{bit_size / 8};
};

//! 64-bit specialization.
template <>
struct TypeTraits<RegBitSize::b64> {
    using type = std::uint64_t;    // NOLINT
    constexpr static auto bit_size = std::uint8_t{64};
    constexpr static auto byte_size = std::uint8_t{bit_size / 8};
};

//!@}


}    // namespace cppreg


#endif    // CPPREG_TRAITS_H
