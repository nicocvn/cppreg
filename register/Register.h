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
     * @tparam reg_address Register address.
     * @tparam reg_size Register size enum value.
     * @tparam ResetValue Register reset value (0x0 if unknown).
     * @tparam UseShadow shadow Boolean flag to enable shadow value.
     *
     * This data structure will act as a container for fields and is
     * therefore limited to a strict minimum. It only carries information
     * about the register base address, size, and reset value.
     * In practice, register are implemented by deriving from this class to
     * create custom types.
     */
    template <
        Address_t reg_address,
        RegBitSize reg_size,
        typename TypeTraits<reg_size>::type reset_value = 0x0,
        bool use_shadow = false
    >
    struct Register {

        //! Register base type.
        using type = typename TypeTraits<reg_size>::type;

        //! MMIO pointer type.
        using MMIO_t = volatile type;

        //! Boolean flag for shadow value management.
        using shadow = Shadow<Register, use_shadow>;

        //! Register base address.
        constexpr static const Address_t base_address = reg_address;

        //! Register size in bits.
        constexpr static const std::uint8_t size =
            TypeTraits<reg_size>::bit_size;

        //! Register reset value.
        constexpr static const type reset = reset_value;

        //! Memory modifier.
        /**
         * @return A reference to the writable register memory.
         */
        static MMIO_t& rw_mem_device() {
            return *(reinterpret_cast<MMIO_t* const>(base_address));
        };

        //! Memory accessor.
        /**
         * @return A reference to the read-only register memory.
         */
        static const MMIO_t& ro_mem_device() {
            return *(reinterpret_cast<const MMIO_t* const>(base_address));
        };

        //! Merge write start function.
        /**
         * @tparam F Field on which to perform the first write operation.
         * @param value Value to be written to the field.
         * @return A merge write data structure to chain further writes.
         */
        template <typename F>
        inline static MergeWrite<typename F::parent_register, F::mask>
        merge_write(const typename F::type value) noexcept {
            return
                MergeWrite<typename F::parent_register, F::mask>
                ::make(((value << F::offset) & F::mask));
        };

        //! Merge write start function for constant value.
        /**
         * @tparam F Field on which to perform the first write operation.
         * @tparam value Value to be written to the field.
         * @return A merge write data structure to chain further writes.
         */
        template <
            typename F,
            type value,
            typename T = MergeWrite_tmpl<
                typename F::parent_register,
                F::mask,
                F::offset,
                value
                                        >
        >
        inline static
        typename std::enable_if<
            internals::check_overflow<
                type, value, (F::mask >> F::offset)
                                     >::value,
            T
                               >::type&&
        merge_write() noexcept {
            return std::move(T::make());
        };

        // Sanity check.
        static_assert(size != 0u,
                      "defining a Register type of zero size is not allowed");

    };


}


#endif  // CPPREG_REGISTER_H
