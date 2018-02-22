//! Access policy abstract implementation.
/**
 * @file      AccessPolicy.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 *
 * Access policies are used to describe if register fields are read-write,
 * read-only or write-only.
 */


#ifndef CPPREG_ACCESSPOLICY_H
#define CPPREG_ACCESSPOLICY_H


#include "cppreg_Defines.h"


//! cppreg namespace.
namespace cppreg {


    //! Read-only access policy.
    struct read_only {

        //! Read access implementation.
        /**
         * @tparam MMIO_t Register memory device type.
         * @tparam T Field data type.
         * @param mmio_device Pointer to register mapped memory.
         * @param mask Field mask.
         * @param offset Field offset.
         * @return The value at the field location.
         */
        template <typename MMIO_t, typename T>
        inline static T read(const MMIO_t* const mmio_device,
                             const T mask,
                             const Offset_t offset) noexcept {
            return static_cast<T>((*mmio_device & mask) >> offset);
        };

    };


    //! Read-write access policy.
    struct read_write : read_only {

        //! Write access implementation.
        /**
         * @tparam MMIO_t Register memory device type.
         * @tparam T Field data type.
         * @param mmio_device Pointer to register mapped memory.
         * @param mask Field mask.
         * @param offset Field offset.
         * @param value Value to be written at the field location.
         */
        template <typename MMIO_t, typename T>
        inline static void write(MMIO_t* const mmio_device,
                                 const T mask,
                                 const Offset_t offset,
                                 const T value) noexcept {
            *mmio_device = static_cast<T>((*mmio_device & ~mask) |
                                          ((value << offset) & mask));
        };

        //! Set field implementation.
        /**
         * @tparam T Field data type.
         * @param mmio_device Pointer to register mapped memory.
         * @param mask Field mask.
         */
        template <typename MMIO_t, typename T>
        inline static void set(MMIO_t* const mmio_device, const T mask)
        noexcept {
            *mmio_device = static_cast<T>((*mmio_device) | mask);
        };

        //! Clear field implementation.
        /**
         * @tparam MMIO_t Register memory device type.
         * @tparam T Field data type.
         * @param mmio_device Pointer to register mapped memory.
         * @param mask Field mask.
         */
        template <typename MMIO_t, typename T>
        inline static void clear(MMIO_t* const mmio_device, const T mask)
        noexcept {
            *mmio_device = static_cast<T>((*mmio_device) & ~mask);
        };

        //! Toggle field implementation.
        /**
         * @tparam MMIO_t Register memory device type.
         * @tparam T Field data type.
         * @param mmio_device Pointer to register mapped memory.
         * @param mask Field mask.
         */
        template <typename MMIO_t, typename T>
        inline static void toggle(MMIO_t* const mmio_device, const T mask)
        noexcept {
            *mmio_device = static_cast<T>((*mmio_device) ^ mask);
        };

    };


    //! Write-only access policy.
    struct write_only {

        //! Write access implementation.
        /**
         * @tparam MMIO_t Register memory device type.
         * @tparam T Field data type.
         * @param mmio_device Pointer to register mapped memory.
         * @param mask Field mask.
         * @param offset Field offset
         * @param value Value to be written at the field location.
         */
        template <typename MMIO_t, typename T>
        inline static void write(MMIO_t* const mmio_device,
                                 const T mask,
                                 const Offset_t offset,
                                 const T value) noexcept {

            // We cannot read the current value so we simply fully write to it.
            *mmio_device = ((value << offset) & mask);

        };

    };


}


#endif  // CPPREG_ACCESSPOLICY_H
