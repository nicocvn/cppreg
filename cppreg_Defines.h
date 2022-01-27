//! Project-wide definitions header.
/**
 * @file      cppreg_Defines.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2022 Sendyne Corp. All rights reserved.
 *
 * This header mostly defines data types used across the project. These data
 * types have been defined with 32-bits address space hardware in mind. It
 * can easily be extended to larger types if needed.
 */


#ifndef CPPREG_CPPREG_DEFINES_H
#define CPPREG_CPPREG_DEFINES_H


#include "cppreg_Includes.h"


namespace cppreg {


//! Type alias for register and field addresses.
/**
 * By design this type ensures that any address can be stored.
 */
using Address = std::uintptr_t;


//! Enumeration type for register size in bits.
/**
 * This is used to enforce the supported register sizes.
 */
enum class RegBitSize : std::uint8_t {
    //! 8-bit register.
    b8,    // NOLINT
    //! 16-bit register.
    b16,    // NOLINT
    //! 32-bit register.
    b32,    // NOLINT
    //! 64-bit register.
    b64    // NOLINT
};


//! Type alias field width.
using FieldWidth = std::uint8_t;


//! Type alias for field offset.
using FieldOffset = std::uint8_t;


//! Shorthand for max value as a mask.
/**
 * @tparam T Data type.
 *
 * This is used to define register masks.
 */
template <typename T>
struct type_mask {    // NOLINT
    constexpr static const T value = std::numeric_limits<T>::max();
};


//! Global constant to convert bits to bytes.
constexpr static const auto one_byte = std::size_t{8};


}    // namespace cppreg


#endif    // CPPREG_CPPREG_DEFINES_H
