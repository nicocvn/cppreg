//! Project-wide definitions header.
/**
 * @file      cppreg_Defines.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 *
 * This header mostly defines data types used across the project. These data
 * types have been defined with 32-bits address space hardware in mind. It
 * can easily be extended to larger types if needed.
 */


#ifndef CPPREG_CPPREG_DEFINES_H
#define CPPREG_CPPREG_DEFINES_H


#include "cppreg_Includes.h"


//! cppreg namespace.
namespace cppreg {


    //! Type alias for register and field addresses.
    /**
     * By design this type ensures that any address can be stored.
     */
    using Address_t = std::uintptr_t;


    //! Enumeration type for register size in bits.
    /**
     * This is used to enforce the supported register sizes.
     */
    enum class RegBitSize {
        b8,     //!< 8-bit register.
        b16,    //!< 16-bit register.
        b32,    //!< 32-bit register.
        b64     //!< 64-bit register.
    };

    //! Type alias field width.
    using FieldWidth_t = std::uint8_t;


    //! Type alias for field offset.
    using FieldOffset_t = std::uint8_t;


    //! Shorthand for max value as a mask.
    /**
     * @tparam T Data type.
     *
     * This is used to define register masks.
     */
    template <typename T>
    struct type_mask {
        constexpr static const T value = std::numeric_limits<T>::max();
    };


}


#endif  // CPPREG_CPPREG_DEFINES_H
