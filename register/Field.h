//! Register field type implementation.
/**
 * @file      Field.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 *
 * This header provides the definitions related to register field
 * implementation.
 */


#ifndef CPPREG_REGISTERFIELD_H
#define CPPREG_REGISTERFIELD_H


#include "Register.h"
#include "Mask.h"
#include "AccessPolicy.h"
#include "ShadowValue.h"


//! cppreg namespace.
namespace cppreg {

    //! Register field implementation.
    /**
     * @tparam BaseRegister Parent register.
     * @tparam width Field width.
     * @tparam offset Field offset.
     * @tparam P Access policy type (rw, ro, wo).
     *
     * This data structure provides static methods to deal with field access
     * (read, write, set, clear, and toggle). These methods availability depends
     * on the access policy (e.g., a read-only field does not implement a
     * write method).
     * In practice, fields are implemented by deriving from this class to
     * create custom types.
     */
    template <
        typename BaseRegister,
        Width_t FieldWidth,
        Offset_t FieldOffset,
        typename AccessPolicy
    >
    struct Field {

        //! Parent register for the field.
        using parent_register = BaseRegister;

        //! Field data type derived from register data type.
        using type = typename parent_register::type;

        //! MMIO type.
        using MMIO_t = typename parent_register::MMIO_t;

        //! Field width.
        constexpr static const Width_t width = FieldWidth;

        //! Field offset.
        constexpr static const Offset_t offset = FieldOffset;

        //! Field policy.
        using policy = AccessPolicy;

        //! Field mask.
        /**
         * The field mask is computed at compile time.
         */
        constexpr static const type mask = make_shifted_mask<type>(width,
                                                                   offset);

        //! Customized overflow check implementation for Field types.
        /**
         * @tparam value Value to be checked for overflow.
         * @return `true` if no overflow, `false` otherwise.
         */
        template <type value>
        struct check_overflow {
            constexpr static const bool result =
                internals::check_overflow<
                    parent_register::size,
                    value,
                    (mask >> offset)
                                         >::result::value;
        };

        //! Field read method.
        /**
         * @return Field value.
         */
        inline static type read() noexcept {
            return
                AccessPolicy
                ::template read<MMIO_t>(parent_register::ro_mem_pointer(),
                                        mask,
                                        offset);
        };

        //! Field write method (shadow value disabled).
        /**
         * @param value Value to be written to the field.
         *
         * We use SFINAE to discriminate for registers with shadow value
         * enabled.
         *
         * This method does not perform any check on the input value. If the
         * input value is too large for the field size it will not overflow
         * but only the part that fits in the field will be written.
         * For safe write see
         */
        template <typename T = type>
        inline static void
        write(const typename std::enable_if<
            !parent_register::shadow::use_shadow, T
                                           >::type value) noexcept {
            AccessPolicy
            ::template write<MMIO_t>(parent_register::rw_mem_pointer(),
                                     mask,
                                     offset,
                                     value);
        };

        //! Field write method (shadow value enabled).
        /**
         * @param value Value to be written to the field.
         *
         * We use SFINAE to discriminate for registers with shadow value
         * enabled.
         *
         * This method does not perform any check on the input value. If the
         * input value is too large for the field size it will not overflow
         * but only the part that fits in the field will be written.
         * For safe write see
         */
        template <typename T = type>
        inline static void
        write(const typename std::enable_if<
            parent_register::shadow::use_shadow, T
                                           >::type value) noexcept {

            // Update shadow value.
            // Fetch the whole register content.
            // This assumes that reading a write-only fields return some value.
            parent_register::shadow::value =
                (parent_register::shadow::value & ~mask) |
                ((value << offset) & mask);

            // Write as a block to the register, that is, we do not use the
            // mask and offset.
            AccessPolicy
            ::template write<MMIO_t>(parent_register::rw_mem_pointer(),
                                     ~(0u),
                                     0u,
                                     parent_register::shadow::value);

        };

        //! Field write method with compile-time check (shadow value disabled).
        /**
         * @tparam value Value to be written to the field
         *
         * We use SFINAE to discriminate for registers with shadow value
         * enabled.
         *
         * This method performs a compile-time check to avoid overflowing the
         * field.
         */
        template <type value, typename T = void>
        inline static
        typename std::enable_if<
            (!parent_register::shadow::use_shadow)
            &&
            check_overflow<value>::result,
            T
                               >::type
        write() noexcept {
            write(value);
        };

        //! Field write method with compile-time check (shadow value enabled).
        /**
         * @tparam value Value to be written to the field
         *
         * We use SFINAE to discriminate for registers with shadow value
         * enabled.
         *
         * This method performs a compile-time check to avoid overflowing the
         * field.
         */
        template <type value, typename T = void>
        inline static
        typename std::enable_if<
            parent_register::shadow::use_shadow
            &&
            check_overflow<value>::result,
            T
                               >::type
        write() noexcept {
            write(value);
        };


        //! Field set method.
        /**
         * This method will set all bits in the field.
         */
        inline static void set() noexcept {
            AccessPolicy
            ::template set<MMIO_t>(parent_register::rw_mem_pointer(), mask);
        };

        //! Field clear method.
        /**
         * This method will clear all bits in the field.
         */
        inline static void clear() noexcept {
            AccessPolicy
            ::template clear<MMIO_t>(parent_register::rw_mem_pointer(), mask);
        };

        //! Field toggle method.
        /**
         * This method will toggle all bits in the field.
         */
        inline static void toggle() noexcept {
            AccessPolicy
            ::template toggle<MMIO_t>(parent_register::rw_mem_pointer(), mask);
        };

        //! Is field set bool method.
        /**
         * @return `true` if the 1-bit field is set to 1, `false` otherwise.
         *
         * This is only available if the field is 1 bit wide.
         */
        template <typename T = bool>
        inline static typename std::enable_if<FieldWidth == 1, T>::type
        is_set() noexcept {
            return (Field::read() == 1u);
        };

        //! Is field clear bool method.
        /**
         * @return `true` if the 1-bit field is set to 0, `false` otherwise.
         *
         * This is only available if the field is 1 bit wide.
         */
        template <typename T = bool>
        inline static typename std::enable_if<FieldWidth == 1, T>::type
        is_clear() noexcept {
            return (Field::read() == 0u);
        };
        
        // Consistency checking.
        // The width of the field cannot exceed the register size and the
        // width added to the offset cannot exceed the register size.
        static_assert(parent_register::size >= width,
                      "field width is larger than parent register size");
        static_assert(parent_register::size >= width + offset,
                      "offset + width is larger than parent register size");
        static_assert(FieldWidth != 0u,
                      "defining a Field type of width 0u is not allowed");

    };


}


#endif  // CPPREG_REGISTERFIELD_H
