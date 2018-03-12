//! Register field type implementation.
/**
 * @file      Field.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 *
 * This header provides the definitions related to register field
 * implementation.
 * Strictly speaking a field is defined as a region of a register.
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
     * @tparam P Access policy type.
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

        //! Field policy.
        using policy = AccessPolicy;

        //! Field width.
        constexpr static const Width_t width = FieldWidth;

        //! Field offset.
        constexpr static const Offset_t offset = FieldOffset;

        //! Field mask.
        constexpr static const type mask = make_shifted_mask<type>(width,
                                                                   offset);

        //! Boolean flag indicating if a shadow value is used.
        constexpr static const bool has_shadow =
            parent_register::shadow::value;

        //! Customized overflow check implementation for Field types.
        /**
         * @tparam value Value to be checked for overflow.
         * @return `true` if no overflow, `false` otherwise.
         *
         * This is only used for the template form of the write method.
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

        //!@ Field read method.
        /**
         * @return Field value.
         */
        inline static type read() noexcept {
            return policy::template read<MMIO_t, type, mask, offset>(
                parent_register::ro_mem_device()
                                                                    );
        };

        //! Field write method (no shadow value).
        /**
         * @param value Value to be written to the field.
         */
        template <typename T = type>
        inline static void
        write(const typename std::enable_if<!has_shadow, T>::type value)
        noexcept {
            policy::template write<MMIO_t, type, mask, offset>(
                parent_register::rw_mem_device(),
                value
                                                              );
        };

        //! Field write method (w/ shadow value).
        /**
         * @param value Value to be written to the field.
         */
        template <typename T = type>
        inline static void
        write(const typename std::enable_if<has_shadow, T>::type value)
        noexcept {

            // Update shadow value.
            // This assumes that reading a write-only fields return some value.
            RegisterWrite<type, type, mask, offset>
            ::write(parent_register::shadow::shadow_value, value);

            // Write as a block to the register, that is, we do not use the
            // mask and offset.
            policy::template write<MMIO_t, type, type_mask<type>::value, 0u>(
                parent_register::rw_mem_device(),
                parent_register::shadow::shadow_value
                                                                            );

        };

        //! Field write method with overflow check (no shadow value).
        /**
         * @tparam value Value to be written to the field
         *
         * This method performs a compile-time check to avoid overflowing the
         * field and uses the constant write implementation.
         */
        template <type value, typename T = void>
        inline static
        typename std::enable_if<
            !has_shadow
            &&
            check_overflow<value>::result,
            T
                               >::type
        write() noexcept {
            policy::template write<MMIO_t, type, mask, offset, value>(
                parent_register::rw_mem_device()
                                                                     );
        };

        //! Field write method with overflow check (w/ shadow value).
        /**
         * @tparam value Value to be written to the field
         *
         * This method performs a compile-time check to avoid overflowing the
         * field and uses the constant write implementation.
         */
        template <type value, typename T = void>
        inline static
        typename std::enable_if<
            has_shadow
            &&
            check_overflow<value>::result,
            T
                               >::type
        write() noexcept {

            // For this particular we can simply forward to the non-constant
            // implementation.
            write(value);

        };

        //! Field set method.
        /**
         * This method will set all bits in the field.
         */
        inline static void set() noexcept {
            policy::template
            set<MMIO_t, type, mask>(parent_register::rw_mem_device());
        };

        //! Field clear method.
        /**
         * This method will clear all bits in the field.
         */
        inline static void clear() noexcept {
            policy::template
            clear<MMIO_t, type, mask>(parent_register::rw_mem_device());
        };

        //! Field toggle method.
        /**
         * This method will toggle all bits in the field.
         */
        inline static void toggle() noexcept {
            policy::template
            toggle<MMIO_t, type, mask>(parent_register::rw_mem_device());
        };

        //! Is field set bool method.
        /**
         * @return `true` if all the bits are set to 1, `false` otherwise.
         */
        inline static bool is_set() noexcept {
            return (Field::read() == (mask >> offset));
        };

        //! Is field clear bool method.
        /**
         * @return `true` if all the bits are set to 0, `false` otherwise.
         */
        inline static bool is_clear() noexcept {
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
