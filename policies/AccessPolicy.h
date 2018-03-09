//! Access policy implementation.
/**
 * @file      AccessPolicy.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
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
 *   values (i.e., known at compile time) and non-constant value. This was
 *   intended to make it possible to simplify operations at compile time and
 *   minimize overhead.
 */


#ifndef CPPREG_ACCESSPOLICY_H
#define CPPREG_ACCESSPOLICY_H


#include "cppreg_Defines.h"


//! cppreg namespace.
namespace cppreg {


    //! Register read implementation.
    /**
     * @tparam MMIO_t Memory device type.
     * @tparam T Register data type.
     * @tparam mask Mask for the read operation.
     * @tparam offset Offset for the read operation.
     *
     * The mask and offset are used to define a specific field within the
     * register.
     */
    template <typename MMIO_t, typename T, T mask, Offset_t offset>
    struct RegisterRead {

        //! Boolean flag for trivial implementation.
        /**
         * The trivial corresponds to reading the whole register, that is,
         * the mask is identity and the offset is zero.
         */
        constexpr static const bool is_trivial =
            (mask == type_mask<T>::value) && (offset == 0u);

        //! Non-trivial read implementation.
        /**
         * @param mmio_device Pointer to the register memory device.
         * @return The content of the register field.
         */
        template <typename U = void>
        inline static T read(
            const MMIO_t* const mmio_device,
            typename std::enable_if<!is_trivial, U*>::type = nullptr
                            ) noexcept {
            return static_cast<T>((*mmio_device & mask) >> offset);
        };

        //! Trivial read implementation.
        /**
         * @param mmio_device Pointer to the register memory device.
         * @return The content of the register field.
         */
        template <typename U = void>
        inline static T read(
            const MMIO_t* const mmio_device,
            typename std::enable_if<is_trivial, U*>::type = nullptr
                            ) noexcept {
            return static_cast<T>(*mmio_device);
        };

    };


    //! Register write implementation.
    /**
     * @tparam MMIO_t Memory device type.
     * @tparam T Register data type.
     * @tparam mask Mask for the read operation.
     * @tparam offset Offset for the read operation.
     *
     * The mask and offset are used to define a specific field within the
     * register.
     *
     * This write implementation is used only when the value to be written is
     * not a constant expression.
     */
    template <typename MMIO_t, typename T, T mask, Offset_t offset>
    struct RegisterWrite {

        //! Boolean flag for trivial implementation.
        /**
         * The trivial corresponds to writing to the whole register, that is,
         * the mask is identity and the offset is zero.
         */
        constexpr static const bool is_trivial =
            (mask == type_mask<T>::value) && (offset == 0u);

        //! Non-trivial write implementation.
        /**
         * @param mmio_device Pointer to the register memory device.
         * @param value Value to be written to the register field.
         */
        template <typename U = void>
        inline static void write(
            MMIO_t* const mmio_device,
            T value,
            typename std::enable_if<!is_trivial, U*>::type = nullptr
                                ) noexcept {
            *mmio_device = static_cast<T>(
                (*mmio_device & ~mask) | ((value << offset) & mask)
            );
        };

        //! Trivial write implementation.
        /**
         * @param mmio_device Pointer to the register memory device.
         * @param value Value to be written to the register field.
         */
        template <typename U = void>
        inline static void write(
            MMIO_t* const mmio_device,
            T value,
            typename std::enable_if<is_trivial, U*>::type = nullptr
                                ) noexcept {
            *mmio_device = value;
        };

    };


    //! Register write constant implementation.
    /**
     * @tparam MMIO_t Memory device type.
     * @tparam T Register data type.
     * @tparam mask Mask for the read operation.
     * @tparam offset Offset for the read operation.
     * @tparam value Value to be written to the register field.
     *
     * The mask and offset are used to define a specific field within the
     * register.
     *
     * This write implementation is used only when the value to be written is
     * a constant expression.
     */
    template <typename MMIO_t, typename T, T mask, Offset_t offset, T value>
    struct RegisterWriteConstant {

        //! Boolean flag for trivial implementation.
        /**
         * The trivial corresponds to writing to the whole register, that is,
         * the mask is identity and the offset is zero.
         */
        constexpr static const bool is_trivial =
            (mask == type_mask<T>::value) && (offset == 0u);

        //! Non-trivial write implementation.
        /**
         * @param mmio_device Pointer to the register memory device.
         */
        template <typename U = void>
        inline static void write(
            MMIO_t* const mmio_device,
            typename std::enable_if<!is_trivial, U*>::type = nullptr
                                ) noexcept {
            *mmio_device = static_cast<T>(
                (*mmio_device & ~mask) | ((value << offset) & mask)
            );
        };

        //! Trivial write implementation.
        /**
         * @param mmio_device Pointer to the register memory device.
         */
        template <typename U = void>
        inline static void write(
            MMIO_t* const mmio_device,
            typename std::enable_if<is_trivial, U*>::type = nullptr
                             ) noexcept {
            *mmio_device = value;
        };

    };


    //! Read-only access policy.
    struct read_only {

        //! Read access implementation.
        /**
         * @tparam MMIO_t Register memory device type.
         * @tparam T Field data type.
         * @tparam mask Field mask.
         * @tparam offset Field offset.
         * @param mmio_device Pointer to register mapped memory.
         * @return The value at the field location.
         */
        template <typename MMIO_t, typename T, T mask, Offset_t offset>
        inline static T read(const MMIO_t* const mmio_device) noexcept {
            return RegisterRead<MMIO_t, T, mask, offset>::read(mmio_device);
        };

    };


    //! Read-write access policy.
    struct read_write : read_only {

        //! Write access implementation.
        /**
         * @tparam MMIO_t Register memory device type.
         * @tparam T Field data type.
         * @tparam mask Field mask.
         * @tparam offset Field offset.
         * @param mmio_device Pointer to register mapped memory.
         * @param value Value to be written at the field location.
         */
        template <typename MMIO_t, typename T, T mask, Offset_t offset>
        inline static void write(MMIO_t* const mmio_device,
                                 const T value) noexcept {
            RegisterWrite<MMIO_t, T, mask, offset>::write(mmio_device, value);
        };

        //! Write access implementation for constant value.
        /**
         * @tparam MMIO_t Register memory device type.
         * @tparam T Field data type.
         * @tparam mask Field mask.
         * @tparam offset Field offset.
         * @tparam value Value to be written at the field location.
         * @param mmio_device Pointer to register mapped memory.
         */
        template <typename MMIO_t, typename T, T mask, Offset_t offset, T value>
        inline static void write(MMIO_t* const mmio_device) noexcept {
            RegisterWriteConstant<MMIO_t, T, mask, offset, value>
            ::write(mmio_device);
        };

        //! Set field implementation.
        /**
         * @tparam T Field data type.
         * @tparam mask Field mask.
         * @param mmio_device Pointer to register mapped memory.
         */
        template <typename MMIO_t, typename T, T mask>
        inline static void set(MMIO_t* const mmio_device)
        noexcept {
            RegisterWriteConstant<MMIO_t, T, mask, 0u, mask>
            ::write(mmio_device);
        };

        //! Clear field implementation.
        /**
         * @tparam MMIO_t Register memory device type.
         * @tparam T Field data type.
         * @tparam mask Field mask.
         * @param mmio_device Pointer to register mapped memory.
         */
        template <typename MMIO_t, typename T, T mask>
        inline static void clear(MMIO_t* const mmio_device)
        noexcept {
            RegisterWriteConstant<MMIO_t, T, mask, 0u, ~mask>
            ::write(mmio_device);
        };

        //! Toggle field implementation.
        /**
         * @tparam MMIO_t Register memory device type.
         * @tparam T Field data type.
         * @tparam mask Field mask.
         * @param mmio_device Pointer to register mapped memory.
         */
        template <typename MMIO_t, typename T, T mask>
        inline static void toggle(MMIO_t* const mmio_device)
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
         * @tparam mask Field mask.
         * @tparam offset Field offset
         * @param mmio_device Pointer to register mapped memory.
         * @param value Value to be written at the field location.
         */
        template <typename MMIO_t, typename T, T mask, Offset_t offset>
        inline static void write(MMIO_t* const mmio_device,
                                 const T value) noexcept {

            // For write-only fields we can only write to the whole register.
            RegisterWrite<MMIO_t, T, type_mask<T>::value, 0u>::write(
                mmio_device, ((value << offset) & mask)
                                                                    );
        };

        //! Write access implementation for constant value.
        /**
         * @tparam MMIO_t Register memory device type.
         * @tparam T Field data type.
         * @tparam mask Field mask.
         * @tparam offset Field offset
         * @tparam value Value to be written at the field location.
         * @param mmio_device Pointer to register mapped memory.
         */
        template <typename MMIO_t, typename T, T mask, Offset_t offset, T value>
        inline static void write(MMIO_t* const mmio_device) noexcept {

            // For write-only fields we can only write to the whole register.
            RegisterWriteConstant<
                MMIO_t, T, type_mask<T>::value, 0u, ((value << offset) & mask)
                                 >
            ::write(mmio_device);

        };

    };


}


#endif  // CPPREG_ACCESSPOLICY_H
