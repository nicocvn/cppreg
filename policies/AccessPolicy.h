//! Access policy implementation.
/**
 * @file      AccessPolicy.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2019 Sendyne Corp. All rights reserved.
 *
 * Access policies are used to describe if register fields are read-write,
 * read-only or write-only.
 *
 * - The read and write implementations distinguish between trivial and
 *   non-trivial operations. A trivial operation corresponds to a read or
 *   write over a whole register; in such a case no masking of shifting is
 *   necessary. On the other hand, if the operation is only over a region of
 *   the register proper masking and shifting is required. The switch between
 *   trivial and non-trivial implementations is done automatically.
 * - The write implementation also distinguishes between writing constant
 *   values (i.e., known at compile time) and non-constant value. This is
 *   intended to make it possible to simplify operations at compile time and
 *   minimize overhead.
 */


#ifndef CPPREG_ACCESSPOLICY_H
#define CPPREG_ACCESSPOLICY_H


#include "cppreg_Defines.h"


namespace cppreg {


//!@{ Trivial register read/write detector.

//! Boolean flag to detect trivial read/write.
/**
 * @tparam T Register data type.
 * @tparam mask Mask for the read operation.
 * @tparam offset Offset for the read operation.
 */
template <typename T, T mask, FieldOffset offset>
struct is_trivial_rw
    : std::integral_constant<bool,
                             (mask == type_mask<T>::value)
                                 && (offset == FieldOffset{0})> {};

//! Trivial read/write enable_if helper.
/**
 * @tparam T Register data type.
 * @tparam mask Mask for the read operation.
 * @tparam offset Offset for the read operation.
 * @tparam U Enabled type if trivial.
 */
template <typename T, T mask, FieldOffset offset, typename U>
using is_trivial =
    typename std::enable_if<is_trivial_rw<T, mask, offset>::value, U>::type;

//! Non-trivial read/write enable_if helper.
/**
 * @tparam T Register data type.
 * @tparam mask Mask for the read operation.
 * @tparam offset Offset for the read operation.
 * @tparam U Enabled type if non-trivial.
 */
template <typename T, T mask, FieldOffset offset, typename U>
using is_not_trivial =
    typename std::enable_if<!is_trivial_rw<T, mask, offset>::value, U>::type;

//!@}


//! Register read implementation.
/**
 * @tparam MMIO Memory device type.
 * @tparam T Register data type.
 * @tparam mask Mask for the read operation.
 * @tparam offset Offset for the read operation.
 */
template <typename MMIO, typename T, T mask, FieldOffset offset>
struct RegisterRead {

    //! Non-trivial read implementation.
    /**
     * @param mmio_device Pointer to the register memory device.
     * @return The content of the register field.
     */
    template <typename U = void>
    static T read(const MMIO& mmio_device,
                  is_not_trivial<T, mask, offset, U>* = nullptr) noexcept {
        return static_cast<T>((mmio_device & mask) >> offset);
    }

    //! Trivial read implementation.
    /**
     * @param mmio_device Pointer to the register memory device.
     * @return The content of the register field.
     */
    template <typename U = void>
    static T read(const MMIO& mmio_device,
                  is_trivial<T, mask, offset, U>* = nullptr) noexcept {
        return static_cast<T>(mmio_device);
    }
};


//! Register write implementation.
/**
 * @tparam MMIO Memory device type.
 * @tparam T Register data type.
 * @tparam mask Mask for the read operation.
 * @tparam offset Offset for the read operation.
 */
template <typename MMIO, typename T, T mask, FieldOffset offset>
struct RegisterWrite {

    //! Non-trivial write implementation.
    /**
     * @param mmio_device Pointer to the register memory device.
     * @param value Value to be written to the register field.
     */
    template <typename U = void>
    static void write(MMIO& mmio_device,
                      T value,
                      is_not_trivial<T, mask, offset, U>* = nullptr) noexcept {
        mmio_device =
            static_cast<T>((mmio_device & ~mask) | ((value << offset) & mask));
    }

    //! Trivial write implementation.
    /**
     * @param mmio_device Pointer to the register memory device.
     * @param value Value to be written to the register field.
     */
    template <typename U = void>
    static void write(MMIO& mmio_device,
                      T value,
                      is_trivial<T, mask, offset, U>* = nullptr) noexcept {
        mmio_device = value;
    }
};


//! Register write constant implementation.
/**
 * @tparam MMIO Memory device type.
 * @tparam T Register data type.
 * @tparam mask Mask for the read operation.
 * @tparam offset Offset for the read operation.
 * @tparam value Value to be written to the register field.
 */
template <typename MMIO, typename T, T mask, FieldOffset offset, T value>
struct RegisterWriteConstant {

    //! Non-trivial write implementation.
    /**
     * @param mmio_device Pointer to the register memory device.
     */
    template <typename U = void>
    static void write(MMIO& mmio_device,
                      is_not_trivial<T, mask, offset, U>* = nullptr) noexcept {
        mmio_device =
            static_cast<T>((mmio_device & ~mask) | ((value << offset) & mask));
    }

    //! Trivial write implementation.
    /**
     * @param mmio_device Pointer to the register memory device.
     */
    template <typename U = void>
    static void write(MMIO& mmio_device,
                      is_trivial<T, mask, offset, U>* = nullptr) noexcept {
        mmio_device = value;
    }
};


//! Read-only access policy.
struct read_only {

    //! Read access implementation.
    /**
     * @tparam MMIO Register memory device type.
     * @tparam T Field data type.
     * @tparam mask Field mask.
     * @tparam offset Field offset.
     * @param mmio_device Pointer to register mapped memory.
     * @return The value at the field location.
     */
    template <typename MMIO, typename T, T mask, FieldOffset offset>
    static T read(const MMIO& mmio_device) noexcept {
        return RegisterRead<MMIO, T, mask, offset>::read(mmio_device);
    }
};


//! Read-write access policy.
struct read_write : read_only {

    //! Write access implementation.
    /**
     * @tparam MMIO Register memory device type.
     * @tparam T Field data type.
     * @tparam mask Field mask.
     * @tparam offset Field offset.
     * @param mmio_device Pointer to register mapped memory.
     * @param value Value to be written at the field location.
     */
    template <typename MMIO, typename T, T mask, FieldOffset offset>
    static void write(MMIO& mmio_device, const T value) noexcept {
        RegisterWrite<MMIO, T, mask, offset>::write(mmio_device, value);
    }

    //! Write access implementation for constant value.
    /**
     * @tparam MMIO Register memory device type.
     * @tparam T Field data type.
     * @tparam mask Field mask.
     * @tparam offset Field offset.
     * @tparam value Value to be written at the field location.
     * @param mmio_device Pointer to register mapped memory.
     */
    template <typename MMIO, typename T, T mask, FieldOffset offset, T value>
    static void write(MMIO& mmio_device) noexcept {
        RegisterWriteConstant<MMIO, T, mask, offset, value>::write(mmio_device);
    }

    //! Set field implementation.
    /**
     * @tparam T Field data type.
     * @tparam mask Field mask.
     * @param mmio_device Pointer to register mapped memory.
     */
    template <typename MMIO, typename T, T mask>
    static void set(MMIO& mmio_device) noexcept {
        RegisterWriteConstant<MMIO, T, mask, FieldOffset{0}, mask>::write(
            mmio_device);
    }

    //! Clear field implementation.
    /**
     * @tparam MMIO Register memory device type.
     * @tparam T Field data type.
     * @tparam mask Field mask.
     * @param mmio_device Pointer to register mapped memory.
     */
    template <typename MMIO, typename T, T mask>
    static void clear(MMIO& mmio_device) noexcept {
        RegisterWriteConstant<MMIO,
                              T,
                              mask,
                              FieldOffset{0},
                              static_cast<T>(~mask)>::write(mmio_device);
    }

    //! Toggle field implementation.
    /**
     * @tparam MMIO Register memory device type.
     * @tparam T Field data type.
     * @tparam mask Field mask.
     * @param mmio_device Pointer to register mapped memory.
     */
    template <typename MMIO, typename T, T mask>
    static void toggle(MMIO& mmio_device) noexcept {
        mmio_device = static_cast<T>((mmio_device) ^ mask);
    }
};


//! Write-only access policy.
struct write_only {

    //! Write access implementation.
    /**
     * @tparam MMIO Register memory device type.
     * @tparam T Field data type.
     * @tparam mask Field mask.
     * @tparam offset Field offset
     * @param mmio_device Pointer to register mapped memory.
     * @param value Value to be written at the field location.
     */
    template <typename MMIO, typename T, T mask, FieldOffset offset>
    static void write(MMIO& mmio_device, const T value) noexcept {

        // For write-only fields we can only write to the whole register.
        RegisterWrite<MMIO, T, type_mask<T>::value, FieldOffset{0}>::write(
            mmio_device, ((value << offset) & mask));
    }

    //! Write access implementation for constant value.
    /**
     * @tparam MMIO Register memory device type.
     * @tparam T Field data type.
     * @tparam mask Field mask.
     * @tparam offset Field offset
     * @tparam value Value to be written at the field location.
     * @param mmio_device Pointer to register mapped memory.
     */
    template <typename MMIO, typename T, T mask, FieldOffset offset, T value>
    static void write(MMIO& mmio_device) noexcept {

        // For write-only fields we can only write to the whole register.
        RegisterWriteConstant<MMIO,
                              T,
                              type_mask<T>::value,
                              FieldOffset{0},
                              ((value << offset) & mask)>::write(mmio_device);
    }
};


}    // namespace cppreg


#endif    // CPPREG_ACCESSPOLICY_H
