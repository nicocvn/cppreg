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


    //! Type alias for register and field widths.
    /**
     * This limit the implementation to a maximum size of 256 bits for any
     * register or field.
     * This corresponds to the number of bits in the a register or field.
     */
    using Width_t = std::uint8_t;


    //! Type alias for register and field offsets.
    /**
     * This limit the implementation to a maximum offset of 256 bits for any
     * register or field.
     * Given that we consider 32 bits address space and 32 bits register this
     * should be enough.
     */
    using Offset_t = std::uint8_t;


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
