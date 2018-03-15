//! Bit mask implementation.
/**
 * @file      Mask.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 *
 * The implementation is designed to compute masks prior to runtime by
 * relying on constexpr function. This will work as intended if the function
 * argument is known at compile time, otherwise it will be evaluated at runtime.
 */


#ifndef CPPREG_MASK_H
#define CPPREG_MASK_H


#include "cppreg_Defines.h"


//! cppreg namespace.
namespace cppreg {


    //! Mask constexpr function.
    /**
     * @tparam Mask_t Mask data type (will be derived from register).
     * @param width Mask width.
     * @return The mask value.
     */
    template <typename Mask_t>
    constexpr Mask_t make_mask(const FieldWidth_t width) noexcept {
        return width == 0 ?
               0u
                          :
               static_cast<Mask_t>(
                   (make_mask<Mask_t>(FieldWidth_t(width - 1)) << 1) | 1
               );
    };


    //! Shifted mask constexpr function.
    /**
     * @tparam Mask_t Mask data type (will be derived from register).
     * @param width Mask width.
     * @param offset Mask offset.
     * @return The mask value.
     */
    template <typename Mask_t>
    constexpr Mask_t make_shifted_mask(const FieldWidth_t width,
                                       const FieldOffset_t offset) noexcept {
        return static_cast<Mask_t>(make_mask<Mask_t>(width) << offset);
    };


}


#endif  // CPPREG_MASK_H
