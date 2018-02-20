//! Merge write implementation.
/**
 * @file      MergeWrite.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 *
 * The "merge write" implementation is designed to make it possible to merge
 * write operations on different fields into a single one.
 */


#ifndef CPPREG_MERGEWRITE_H
#define CPPREG_MERGEWRITE_H


#include "Overflow.h"
#include "AccessPolicy.h"
#include <functional>


//! cppreg namespace.
namespace cppreg {


    //! Write operation holding structure.
    /**
     * @tparam Register Underlying register for the final write operation.
     */
    template <typename Register>
    class MergeWrite {


    public:

        //! Type alias to register base type.
        using base_type = typename Register::type;

        //! Static instantiation method.
        static MergeWrite create_instance(const base_type value,
                                          const base_type mask) noexcept {
            MergeWrite mw;
            mw._accumulated_value = value;
            mw._combined_mask = mask;
            return mw;
        };

        //!@ Move constructor.
        MergeWrite(MergeWrite&& mw) noexcept
            : _accumulated_value(mw._accumulated_value),
              _combined_mask(mw._combined_mask) {
        };

        //!@{ Non-copyable.
        MergeWrite(const MergeWrite&) = delete;
        MergeWrite& operator=(const MergeWrite&) = delete;
        //!@}

        //! Destructor.
        /**
         * This is where the write operation is performed.
         */
        ~MergeWrite() {

            // Get memory pointer.
            typename Register::MMIO_t* const mmio_device =
                Register::rw_mem_pointer();

            // Write to the whole register using the current accumulated value
            // and combined mask.
            // No offset needed because we write to the whole register.
            *mmio_device = static_cast<base_type>(
                (*mmio_device & ~_combined_mask) |
                ((_accumulated_value) & _combined_mask)
            );

        };

        //! With method.
        /**
         * @tparam F Field type describing where to write in the register.
         * @param value Value to write to the register.
         * @return A reference to the current merge write data.
         *
         * This method is used to add another operation to the final merged
         * write.
         */
        template <typename F>
        MergeWrite&& with(const base_type value) && noexcept {

            // Check that the field belongs to the register.
            static_assert(std::is_same<
                              typename F::parent_register,
                              Register
                                      >::value,
                          "field is not from the same register in merge_write");

            // Update accumulated value.
            F::policy::write(&_accumulated_value,
                             F::mask,
                             F::offset,
                             value);

            // Update combine mask.
            _combined_mask = _combined_mask | F::mask;

            return std::move(*this);

        };

        //! With method with compile-time check.
        /**
         * @tparam F Field type describing where to write in the register.
         * @param value Value to write to the register.
         * @return A reference to the current merge write data.
         *
         * This method is used to add another operation to the final merged
         * write.
         *
         * This method performs a compile-time check to avoid overflowing the
         * field.
         */
        template <
            typename F,
            base_type value,
            typename T = MergeWrite
        >
        typename std::enable_if<
            (internals::check_overflow<
                Register::size, value, (F::mask >> F::offset)
                                      >::result::value),
            T
                               >::type&&
        with() && noexcept {
            return std::move(*this).template with<F>(value);
        };


    private:

        // Disabled for shadow value register.
        static_assert(!Register::shadow::use_shadow,
                      "merge write is not available for shadow value register");

        // Private default constructor.
        MergeWrite() : _accumulated_value(0u),
                       _combined_mask(0u) {};

        // Accumulated value.
        base_type _accumulated_value;

        // Combined mask.
        base_type _combined_mask;


    };


}


#endif  // CPPREG_MERGEWRITE_H
