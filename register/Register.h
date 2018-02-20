//! Register type implementation.
/**
 * @file      Register.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 *
 * This header provides the definitions related to register implementation.
 */


#ifndef CPPREG_REGISTER_H
#define CPPREG_REGISTER_H


#include "Traits.h"
#include "MergeWrite.h"
#include "ShadowValue.h"


//! cppreg namespace.
namespace cppreg {


    //! Register data structure.
    /**
     * @tparam address Register address.
     * @tparam width Register total width (i.e., size).
     * @tparam reset Register reset value (0x0 if unknown).
     * @tparam shadow Boolean flag to enable shadow value (enabled if `true`).
     *
     * This data structure will act as a container for fields and is
     * therefore limited to a strict minimum. It only carries information
     * about the register base address, size, and reset value.
     * In practice, register are implemented by deriving from this class to
     * create custom types.
     */
    template <
        Address_t RegAddress,
        Width_t RegWidth,
        typename RegisterType<RegWidth>::type ResetValue = 0x0,
        bool UseShadow = false
    >
    struct Register {

        //! Register base type.
        using type = typename RegisterType<RegWidth>::type;

        //! MMIO pointer type.
        using MMIO_t = volatile type;

        //! Register base address.
        constexpr static const Address_t base_address = RegAddress;

        //! Register total width.
        constexpr static const Width_t size = RegWidth;

        //! Register reset value.
        constexpr static const type reset = ResetValue;

        //! Boolean flag for shadow value management.
        using shadow = Shadow<Register, UseShadow>;

        //! Memory modifier.
        /**
         * @return A pointer to the writable register memory.
         */
        static MMIO_t* rw_mem_pointer() {
            return reinterpret_cast<MMIO_t* const>(base_address);
        };

        //! Memory accessor.
        /**
         * @return A pointer to the read-only register memory.
         */
        static const MMIO_t* ro_mem_pointer() {
            return reinterpret_cast<const MMIO_t* const>(base_address);
        };

        //! Merge write function.
        /**
         * @tparam F Field on which to perform the write operation.
         * @param value Value to write to the field.
         * @return A merge write data structure.
         */
        template <typename F>
        inline static MergeWrite<typename F::parent_register>
        merge_write(const typename F::type value) noexcept {
            return
                MergeWrite<typename F::parent_register>
                ::create_instance(((value << F::offset) & F::mask), F::mask);
        };

        //! Merge write function.
        /**
         * @tparam F Field on which to perform the write operation.
         * @param value Value to write to the field.
         * @return A merge write data structure.
         */
        template <
            typename F,
            type value,
            typename T = MergeWrite<typename F::parent_register>
        >
        inline static
        typename std::enable_if<
            internals::check_overflow<
                size, value, (F::mask >> F::offset)
                                     >::result::value,
            T
                               >::type
        merge_write() noexcept {
            return
                MergeWrite<typename F::parent_register>
                ::create_instance(((value << F::offset) & F::mask), F::mask);
        };

        // Sanity check.
        static_assert(RegWidth != 0u,
                      "defining a Register type of width 0u is not allowed");

    };


}


#endif  // CPPREG_REGISTER_H



