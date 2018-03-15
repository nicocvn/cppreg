//! Merge write implementation.
/**
 * @file      MergeWrite.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 *
 * The "merge write" implementation is designed to make it possible to merge
 * write operations on different fields into a single one.
 * The implementation distinguishes between values known at compile time and
 * value known at run time.
 *
 * By design the merge write implementation forces the caller to chain and
 * finalize all write operations in a single pass.
 */


#ifndef CPPREG_MERGEWRITE_H
#define CPPREG_MERGEWRITE_H


#include "Overflow.h"
#include "AccessPolicy.h"
#include <functional>


//! cppreg namespace.
namespace cppreg {


    //! Merge write constant implementation.
    /**
     * @tparam Register Register on which the merged write will be performed.
     * @tparam mask Initial mask.
     * @tparam offset Initial offset.
     * @tparam value Initial value.
     *
     * The initial data will come from the field on which the merge write
     * will be initiated.
     *
     * This implementation is designed for operations in which all data is
     * available at compile time. This makes it possible to leverage a
     * template implementation and simplify most of the operations (based on
     * the access policies implementation that detects trivial operations). In
     * addition, this will also perform overflow checking.
     */
    template <
        typename Register,
        typename Register::type mask,
        FieldOffset_t offset,
        typename Register::type value
    > class MergeWrite_tmpl {


    private:

        // Type alias to register base type.
        using base_type = typename Register::type;

        // Disabled for shadow value register.
        static_assert(!Register::shadow::value,
                      "merge write is not available for shadow value register");

        // Accumulated value.
        constexpr static const base_type _accumulated_value =
            ((value << offset) & mask);

        // Combined mask.
        constexpr static const base_type _combined_mask = mask;

        // Default constructor.
        MergeWrite_tmpl() {};


    public:

        //! Instantiation method.
        inline static MergeWrite_tmpl make() noexcept { return {}; };

        //!@{ Non-copyable and non-moveable.
        MergeWrite_tmpl(const MergeWrite_tmpl&) = delete;
        MergeWrite_tmpl& operator=(const MergeWrite_tmpl&) = delete;
        MergeWrite_tmpl& operator=(MergeWrite_tmpl&&) = delete;
        MergeWrite_tmpl operator=(MergeWrite_tmpl) = delete;
        MergeWrite_tmpl(MergeWrite_tmpl&&) = delete;
        //!@}

        //! Closure method.
        /**
         * This is where the write happens.
         */
        inline void done() const && noexcept {

            // Get memory pointer.
            typename Register::MMIO_t& mmio_device =
                Register::rw_mem_device();

            // Write to the whole register using the current accumulated value
            // and combined mask.
            // No offset needed because we write to the whole register.
            RegisterWriteConstant<
                typename Register::MMIO_t,
                typename Register::type,
                _combined_mask,
                0u,
                _accumulated_value
                                 >::write(mmio_device);

        };

        //! With method for constant value.
        /**
         * @tparam F Field to be written
         * @tparam new_value Value to write to the field.
         * @return A new instance for chaining other write operations.
         */
        template <
            typename F,
            base_type new_value,
            typename T = MergeWrite_tmpl<
                Register,
                (_combined_mask | F::mask),
                0u,
                (_accumulated_value & ~F::mask) | ((new_value << F::offset) &
                                                   F::mask)
                                        >
        >
        inline
        typename std::enable_if<
            (internals::check_overflow<
                typename Register::type, new_value, (F::mask >> F::offset)
                                      >::value),
            T
                               >::type&&
        with() const && noexcept {

            // Check that the field belongs to the register.
            static_assert(std::is_same<
                              typename F::parent_register,
                              Register
                                      >::value,
                          "field is not from the same register in merge_write");

            return std::move(T::make());

        };


    };


    //! Merge write implementation.
    /**
     * @tparam Register Register on which the merged write will be performed.
     * @tparam mask Initial mask.
     *
     * The initial mask will come from the field on which the merge write
     * will be initiated.
     */
    template <
        typename Register,
        typename Register::type mask
    >
    class MergeWrite {


    public:

        //! Type alias to register base type.
        using base_type = typename Register::type;


    private:

        // Combined mask.
        constexpr static const base_type _combined_mask = mask;


    public:

        //! Static instantiation method.
        constexpr static MergeWrite make(const base_type value) noexcept {
            return MergeWrite(value);
        };

        //!@ Move constructor.
        MergeWrite(MergeWrite&& mw) noexcept
        : _accumulated_value(mw._accumulated_value) {};

        //!@{ Non-copyable.
        MergeWrite(const MergeWrite&) = delete;
        MergeWrite& operator=(const MergeWrite&) = delete;
        MergeWrite& operator=(MergeWrite&&) = delete;
        //!@}

        //! Closure method.
        /**
         * This is where the write happens.
         */
        inline void done() const && noexcept {

            // Get memory pointer.
            typename Register::MMIO_t& mmio_device =
                Register::rw_mem_device();

            // Write to the whole register using the current accumulated value
            // and combined mask.
            // No offset needed because we write to the whole register.
            RegisterWrite<
                typename Register::MMIO_t,
                base_type,
                _combined_mask,
                0u
                         >::write(mmio_device, _accumulated_value);

        };

        //! With method.
        /**
         * @tparam F Field type describing where to write in the register.
         * @param value Value to write to the register.
         * @return A reference to the current merge write data.
         */
        template <typename F>
        inline MergeWrite<Register, _combined_mask | F::mask> with
            (const base_type value) && noexcept {

            // Check that the field belongs to the register.
            static_assert(std::is_same<
                              typename F::parent_register,
                              Register
                                      >::value,
                          "field is not from the same register in merge_write");

            // Update accumulated value.
            F::policy::template write<
                base_type,
                base_type,
                F::mask,
                F::offset
                                     >(_accumulated_value, value);

            return
                std::move(
                    MergeWrite<Register, (_combined_mask | F::mask)>
                    ::make(_accumulated_value)
                         );

        };


    private:

        // Disabled for shadow value register.
        static_assert(!Register::shadow::value,
                      "merge write is not available for shadow value register");

        // Private default constructor.
        constexpr MergeWrite() : _accumulated_value(0u) {};
        constexpr MergeWrite(const base_type v) : _accumulated_value(v) {};

        // Accumulated value.
        base_type _accumulated_value;


    };


}


#endif  // CPPREG_MERGEWRITE_H
