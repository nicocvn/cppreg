//! Merge write implementation.
/**
 * @file      MergeWrite.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2022 Sendyne Corp. All rights reserved.
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


#include "AccessPolicy.h"
#include "Internals.h"


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
 * the access polict implementation that detects trivial operations). In
 * addition, this will also perform overflow checking.
 */
template <typename Register,
          typename Register::type mask,
          FieldOffset offset,
          typename Register::type value>
class MergeWrite_tmpl {    // NOLINT

private:
    // Type alias to register base type.
    using base_type = typename Register::type;    // NOLINT

    // Accumulated value.
    constexpr static auto _accumulated_value =    // NOLINT
        base_type{(value << offset) & mask};

    // Combined mask.
    constexpr static auto _combined_mask = mask;    // NOLINT

    // Type helper.
    template <typename F, base_type new_value>
    using propagated =    // NOLINT
        MergeWrite_tmpl<Register,
                        (_combined_mask | F::mask),
                        FieldOffset{0},
                        (_accumulated_value
                         & static_cast<typename Register::type>(~F::mask))
                            | ((new_value << F::offset) & F::mask)>;

    // Default constructor.
    MergeWrite_tmpl() = default;

    // Disabled for shadow value register.
    static_assert(!Register::shadow::value,
                  "merge write is not available for shadow value register");

public:
    //! Instantiation method.
    static MergeWrite_tmpl create() noexcept {
        return {};
    }

    //! Destructor.
    ~MergeWrite_tmpl() = default;

    //! Move constructor.
    MergeWrite_tmpl(MergeWrite_tmpl&&) noexcept = default;

    //!@{ Non-copyable and non-assignable.
    MergeWrite_tmpl(const MergeWrite_tmpl&) = delete;
    MergeWrite_tmpl& operator=(const MergeWrite_tmpl&) = delete;
    MergeWrite_tmpl& operator=(MergeWrite_tmpl&&) = delete;
    MergeWrite_tmpl operator=(MergeWrite_tmpl) = delete;
    //!@}

    //! Closure method.
    /**
     * This is where the write happens.
     */
    void done() const&& noexcept {

        // Get memory pointer.
        typename Register::MMIO& mmio_device = Register::rw_mem_device();

        // Write to the whole register using the current accumulated value
        // and combined mask.
        // No offset needed because we write to the whole register.
        RegisterWriteConstant<typename Register::MMIO,
                              typename Register::type,
                              _combined_mask,
                              FieldOffset{0},
                              _accumulated_value>::write(mmio_device);
    }

    //! With method for constant value.
    /**
     * @tparam F Field to be written
     * @tparam new_value Value to write to the field.
     * @return A merge write instance with accumulated data.
     */
    template <typename F, base_type field_value>
    propagated<F, field_value> with() const&& noexcept {

        // Check that the field belongs to the register.
        static_assert(
            std::is_same<typename F::parent_register, Register>::value,
            "MergeWrite_tmpl:: field is not from the same register");

        // Check that there is no overflow.
        constexpr auto no_overflow =
            internals::check_overflow<typename Register::type,
                                      field_value,
                                      (F::mask >> F::offset)>::value;
        static_assert(no_overflow,
                      "MergeWrite_tmpl:: field overflow in with() call");

        return propagated<F, field_value>{};
    }
};


//! Merge write implementation.
/**
 * @tparam Register Register on which the merged write will be performed.
 * @tparam mask Initial mask.
 *
 * The initial mask will come from the field on which the merge write
 * will be initiated.
 */
template <typename Register, typename Register::type mask>
class MergeWrite {

private:
    // Type alias to register base type.
    using base_type = typename Register::type;    // NOLINT

    // Accumulated value.
    base_type _accumulated_value;    // NOLINT

    // Combined mask.
    constexpr static auto _combined_mask = mask;    // NOLINT

    // Type helper.
    template <typename F>
    using propagated =    // NOLINT
        MergeWrite<Register, _combined_mask | F::mask>;

    // Private default constructor.
    constexpr MergeWrite() : _accumulated_value{0} {};
    constexpr explicit MergeWrite(const base_type v) : _accumulated_value{v} {};

    // Disabled for shadow value register.
    static_assert(!Register::shadow::value,
                  "merge write is not available for shadow value register");

public:
    //! Static instantiation method.
    constexpr static MergeWrite create(const base_type value) noexcept {
        return MergeWrite(value);
    }

    //! Destructor.
    ~MergeWrite() = default;

    //!@ Move constructor.
    MergeWrite(MergeWrite&& mw) noexcept
        : _accumulated_value{mw._accumulated_value} {};

    //!@{ Non-copyable and non-assignable.
    MergeWrite(const MergeWrite&) = delete;
    MergeWrite& operator=(const MergeWrite&) = delete;
    MergeWrite& operator=(MergeWrite&&) = delete;
    //!@}

    //! Closure method.
    /**
     * This is where the write happens.
     */
    void done() const&& noexcept {

        // Get memory pointer.
        typename Register::MMIO& mmio_device = Register::rw_mem_device();

        // Write to the whole register using the current accumulated value
        // and combined mask.
        // No offset needed because we write to the whole register.
        RegisterWrite<typename Register::MMIO,
                      base_type,
                      _combined_mask,
                      FieldOffset{0}>::write(mmio_device, _accumulated_value);
    }

    //! With method.
    /**
     * @tparam F Field type describing where to write in the register.
     * @param value Value to write to the register.
     * @return A merge write instance with accumulated data.
     */
    template <typename F>
    propagated<F> with(const base_type value) const&& noexcept {

        // Check that the field belongs to the register.
        static_assert(
            std::is_same<typename F::parent_register, Register>::value,
            "field is not from the same register in merge_write");

        // Update accumulated value.
        constexpr auto neg_mask = static_cast<base_type>(~F::mask);
        const auto shifted_value = static_cast<base_type>(value << F::offset);
        const auto lhs = static_cast<base_type>(_accumulated_value & neg_mask);
        const auto rhs = static_cast<base_type>(shifted_value & F::mask);
        return propagated<F>::create(static_cast<base_type>(lhs | rhs));
    }
};


}    // namespace cppreg


#endif    // CPPREG_MERGEWRITE_H
