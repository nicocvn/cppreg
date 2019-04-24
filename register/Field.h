//! Register field type implementation.
/**
 * @file      Field.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2019 Sendyne Corp. All rights reserved.
 *
 * This header provides the definitions related to register field
 * implementation. Strictly speaking a field is defined as a region of a
 * register.
 */


#ifndef CPPREG_REGISTERFIELD_H
#define CPPREG_REGISTERFIELD_H


#include "AccessPolicy.h"
#include "Internals.h"
#include "Mask.h"


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
template <typename BaseRegister,
          FieldWidth field_width,
          FieldOffset field_offset,
          typename AccessPolicy>
struct Field {

    //! Parent register for the field.
    using parent_register = BaseRegister;

    //! Field data type derived from register data type.
    using type = typename parent_register::type;

    //! MMIO type.
    using MMIO = typename parent_register::MMIO;

    //! Field policy.
    using policy = AccessPolicy;

    //! Field width.
    constexpr static auto width = field_width;

    //! Field offset.
    constexpr static auto offset = field_offset;

    //! Field mask.
    constexpr static auto mask = make_shifted_mask<type>(width, offset);

    //!@{ Helpers for write method selection based on shadow value.
    template <type value, typename T>
    using if_no_shadow =
        typename std::enable_if<!parent_register::shadow::value, T>::type;
    template <type value, typename T>
    using if_shadow =
        typename std::enable_if<parent_register::shadow::value, T>::type;
    //!@}

    //!@ Field read method.
    /**
     * @return Field value.
     */
    static type read() noexcept {
        return policy::template read<MMIO, type, mask, offset>(
            parent_register::ro_mem_device());
    }

    //! Field write value method (no shadow value).
    /**
     * @param value Value to be written to the field.
     */
    template <typename T = type>
    static void write(const if_no_shadow<type{0}, T> value) noexcept {
        policy::template write<MMIO, type, mask, offset>(
            parent_register::rw_mem_device(), value);
    }

    //! Field write value method (w/ shadow value).
    /**
     * @param value Value to be written to the field.
     */
    template <typename T = type>
    static void write(const if_shadow<type{0}, T> value) noexcept {

        // Update shadow value.
        RegisterWrite<type, type, mask, offset>::write(
            parent_register::shadow::shadow_value, value);

        // Write as a block to the register, that is, we do not use the
        // field mask and field offset.
        policy::
            template write<MMIO, type, type_mask<type>::value, FieldOffset{0}>(
                parent_register::rw_mem_device(),
                parent_register::shadow::shadow_value);
    }

    //! Field write constant method (no shadow value).
    /**
     * @tparam value Constant to be written to the field
     *
     * This method performs a compile-time check to avoid overflowing the
     * field and uses the constant write implementation.
     */
    template <type value, typename T = void>
    static void write(if_no_shadow<value, T>* = nullptr) noexcept {
        policy::template write<MMIO, type, mask, offset, value>(
            parent_register::rw_mem_device());

        // Check for overflow.
        static_assert(
            internals::check_overflow<type, value, (mask >> offset)>::value,
            "Field::write<value>: value too large for the field");
    }

    //! Field write constant method (w/ shadow value).
    /**
     * @tparam value Constant to be written to the field
     *
     * This method performs a compile-time check to avoid overflowing the
     * field and uses the constant write implementation.
     */
    template <type value, typename T = void>
    static void write(if_shadow<value, T>* = nullptr) noexcept {
        // For this one we simply forward to the non-constant
        // implementation because the shadow value needs to be updated.
        write(value);

        // Check for overflow.
        static_assert(
            internals::check_overflow<type, value, (mask >> offset)>::value,
            "Field::write<value>: value too large for the field");
    }

    //! Field set method.
    /**
     * This method will set all bits in the field.
     */
    static void set() noexcept {
        policy::template set<MMIO, type, mask>(
            parent_register::rw_mem_device());
    }

    //! Field clear method.
    /**
     * This method will clear all bits in the field.
     */
    static void clear() noexcept {
        policy::template clear<MMIO, type, mask>(
            parent_register::rw_mem_device());
    }

    //! Field toggle method.
    /**
     * This method will toggle all bits in the field.
     */
    static void toggle() noexcept {
        policy::template toggle<MMIO, type, mask>(
            parent_register::rw_mem_device());
    }

    //! Is field set bool method.
    /**
     * @return `true` if all the bits are set to 1, `false` otherwise.
     */
    static bool is_set() noexcept {
        return (Field::read() == (mask >> offset));
    }

    //! Is field clear bool method.
    /**
     * @return `true` if all the bits are set to 0, `false` otherwise.
     */
    static bool is_clear() noexcept {
        return (Field::read() == type{0});
    }

    // Consistency checking.
    // The width of the field cannot exceed the register size and the
    // width added to the offset cannot exceed the register size.
    static_assert(parent_register::size >= width,
                  "Field:: field width is larger than parent register size");
    static_assert(parent_register::size >= width + offset,
                  "Field:: offset + width is larger than parent register size");
    static_assert(width != FieldWidth{0},
                  "Field:: defining a Field type of zero width is not allowed");
};


}    // namespace cppreg


#endif    // CPPREG_REGISTERFIELD_H
