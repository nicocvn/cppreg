//! cppreg library.
/**
 * @file      cppreg-all.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 */

#include <cstdint>
#include <type_traits>
#include <functional>
#include <limits>

// cppreg_Defines.h
#ifndef CPPREG_CPPREG_DEFINES_H
#define CPPREG_CPPREG_DEFINES_H
namespace cppreg {
    using Address_t = std::uintptr_t;
    using Width_t = std::uint8_t;
    using Offset_t = std::uint8_t;
    template <typename T>
    struct type_mask {
        constexpr static const T value = std::numeric_limits<T>::max();
    };
}
#endif  

// AccessPolicy.h
#ifndef CPPREG_ACCESSPOLICY_H
#define CPPREG_ACCESSPOLICY_H
namespace cppreg {
    template <typename MMIO_t, typename T, T mask, Offset_t offset>
    struct RegisterRead {
        constexpr static const bool is_trivial =
            (mask == type_mask<T>::value) && (offset == 0u);
        template <typename U = void>
        inline static T read(
            const MMIO_t* const mmio_device,
            typename std::enable_if<!is_trivial, U*>::type = nullptr
                            ) noexcept {
            return static_cast<T>((*mmio_device & mask) >> offset);
        };
        template <typename U = void>
        inline static T read(
            const MMIO_t* const mmio_device,
            typename std::enable_if<is_trivial, U*>::type = nullptr
                            ) noexcept {
            return static_cast<T>(*mmio_device);
        };
    };
    template <typename MMIO_t, typename T, T mask, Offset_t offset>
    struct RegisterWrite {
        constexpr static const bool is_trivial =
            (mask == type_mask<T>::value) && (offset == 0u);
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
        template <typename U = void>
        inline static void write(
            MMIO_t* const mmio_device,
            T value,
            typename std::enable_if<is_trivial, U*>::type = nullptr
                                ) noexcept {
            *mmio_device = value;
        };
    };
    template <typename MMIO_t, typename T, T mask, Offset_t offset, T value>
    struct RegisterWriteConstant {
        constexpr static const bool is_trivial =
            (mask == type_mask<T>::value) && (offset == 0u);
        template <typename U = void>
        inline static void write(
            MMIO_t* const mmio_device,
            typename std::enable_if<!is_trivial, U*>::type = nullptr
                                ) noexcept {
            *mmio_device = static_cast<T>(
                (*mmio_device & ~mask) | ((value << offset) & mask)
            );
        };
        template <typename U = void>
        inline static void write(
            MMIO_t* const mmio_device,
            typename std::enable_if<is_trivial, U*>::type = nullptr
                             ) noexcept {
            *mmio_device = value;
        };
    };
    struct read_only {
        template <typename MMIO_t, typename T, T mask, Offset_t offset>
        inline static T read(const MMIO_t* const mmio_device) noexcept {
            return RegisterRead<MMIO_t, T, mask, offset>::read(mmio_device);
        };
    };
    struct read_write : read_only {
        template <typename MMIO_t, typename T, T mask, Offset_t offset>
        inline static void write(MMIO_t* const mmio_device,
                                 const T value) noexcept {
            RegisterWrite<MMIO_t, T, mask, offset>::write(mmio_device, value);
        };
        template <typename MMIO_t, typename T, T mask, Offset_t offset, T value>
        inline static void write(MMIO_t* const mmio_device) noexcept {
            RegisterWriteConstant<MMIO_t, T, mask, offset, value>
            ::write(mmio_device);
        };
        template <typename MMIO_t, typename T, T mask>
        inline static void set(MMIO_t* const mmio_device)
        noexcept {
            RegisterWriteConstant<MMIO_t, T, mask, 0u, mask>
            ::write(mmio_device);
        };
        template <typename MMIO_t, typename T, T mask>
        inline static void clear(MMIO_t* const mmio_device)
        noexcept {
            RegisterWriteConstant<MMIO_t, T, mask, 0u, ~mask>
            ::write(mmio_device);
        };
        template <typename MMIO_t, typename T, T mask>
        inline static void toggle(MMIO_t* const mmio_device)
        noexcept {
            *mmio_device = static_cast<T>((*mmio_device) ^ mask);
        };
    };
    struct write_only {
        template <typename MMIO_t, typename T, T mask, Offset_t offset>
        inline static void write(MMIO_t* const mmio_device,
                                 const T value) noexcept {
            RegisterWrite<MMIO_t, T, type_mask<T>::value, 0u>::write(
                mmio_device, ((value << offset) & mask)
                                                                    );
        };
        template <typename MMIO_t, typename T, T mask, Offset_t offset, T value>
        inline static void write(MMIO_t* const mmio_device) noexcept {
            RegisterWriteConstant<
                MMIO_t, T, type_mask<T>::value, 0u, ((value << offset) & mask)
                                 >
            ::write(mmio_device);
        };
    };
}
#endif  

// Traits.h
#ifndef CPPREG_TRAITS_H
#define CPPREG_TRAITS_H
namespace cppreg {
    template <Width_t Size>
    struct RegisterType;
    template <> struct RegisterType<8u> { using type = std::uint8_t; };
    template <> struct RegisterType<16u> { using type = std::uint16_t; };
    template <> struct RegisterType<32u> { using type = std::uint32_t; };
}
#endif  

// Overflow.h
#ifndef CPPREG_OVERFLOW_H
#define CPPREG_OVERFLOW_H
namespace cppreg {
namespace internals {
    template <
        Width_t W,
        typename RegisterType<W>::type value,
        typename RegisterType<W>::type limit
    >
    struct check_overflow {
        using result =
        typename std::integral_constant<bool, value <= limit>::type;
    };
}
}
#endif  

// Mask.h
#ifndef CPPREG_MASK_H
#define CPPREG_MASK_H
namespace cppreg {
    template <typename Mask_t>
    constexpr Mask_t make_mask(const Width_t width) noexcept {
        return width == 0 ?
               0u
                          :
               static_cast<Mask_t>(
                   (make_mask<Mask_t>(Width_t(width - 1)) << 1) | 1
               );
    };
    template <typename Mask_t>
    constexpr Mask_t make_shifted_mask(const Width_t width,
                                       const Offset_t offset) noexcept {
        return static_cast<Mask_t>(make_mask<Mask_t>(width) << offset);
    };
}
#endif  

// ShadowValue.h
#ifndef CPPREG_SHADOWVALUE_H
#define CPPREG_SHADOWVALUE_H
namespace cppreg {
    template <typename Register, bool UseShadow>
    struct Shadow {
        constexpr static const bool use_shadow = false;
    };
    template <typename Register>
    struct Shadow<Register, true> {
        static typename Register::type value;
        constexpr static const bool use_shadow = true;
    };
    template <typename Register>
    typename Register::type Shadow<Register, true>::value =
        Register::reset;
    template <typename Register>
    constexpr const bool Shadow<Register, true>::use_shadow;
}
#endif  

// MergeWrite.h
#ifndef CPPREG_MERGEWRITE_H
#define CPPREG_MERGEWRITE_H
namespace cppreg {
    template <
        typename Register,
        typename Register::type mask,
        Offset_t offset,
        typename Register::type value
    > class MergeWrite_tmpl {
    public:
        using base_type = typename Register::type;
    private:
        static_assert(!Register::shadow::use_shadow,
                      "merge write is not available for shadow value register");
        constexpr static const base_type _accumulated_value =
            ((value << offset) & mask);
        constexpr static const base_type _combined_mask = mask;
        MergeWrite_tmpl() {};
    public:
        inline static MergeWrite_tmpl make() noexcept { return {}; };
        MergeWrite_tmpl(const MergeWrite_tmpl&) = delete;
        MergeWrite_tmpl& operator=(const MergeWrite_tmpl&) = delete;
        MergeWrite_tmpl& operator=(MergeWrite_tmpl&&) = delete;
        MergeWrite_tmpl operator=(MergeWrite_tmpl) = delete;
        MergeWrite_tmpl(MergeWrite_tmpl&&) = delete;
        inline void done() const && noexcept {
            typename Register::MMIO_t* const mmio_device =
                Register::rw_mem_pointer();
            RegisterWriteConstant<
                typename Register::MMIO_t,
                typename Register::type,
                _combined_mask,
                0u,
                _accumulated_value
                                 >::write(mmio_device);
        };
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
                Register::size, new_value, (F::mask >> F::offset)
                                      >::result::value),
            T
                               >::type&&
        with() const && noexcept {
            return std::move(T::make());
        };
    };
    template <
        typename Register,
        typename Register::type mask
    >
    class MergeWrite {
    public:
        using base_type = typename Register::type;
    private:
        constexpr static const base_type _combined_mask = mask;
    public:
        constexpr static MergeWrite make(const base_type value) noexcept {
            return MergeWrite(value);
        };
        MergeWrite(MergeWrite&& mw) noexcept
        : _accumulated_value(mw._accumulated_value) {};
        MergeWrite(const MergeWrite&) = delete;
        MergeWrite& operator=(const MergeWrite&) = delete;
        MergeWrite& operator=(MergeWrite&&) = delete;
        inline void done() const && noexcept {
            typename Register::MMIO_t* const mmio_device =
                Register::rw_mem_pointer();
            RegisterWrite<
                typename Register::MMIO_t,
                base_type,
                _combined_mask,
                0u
                         >::write(mmio_device, _accumulated_value);
        };
        template <typename F>
        inline MergeWrite<Register, _combined_mask | F::mask> with
            (const base_type value) && noexcept {
            static_assert(std::is_same<
                              typename F::parent_register,
                              Register
                                      >::value,
                          "field is not from the same register in merge_write");
            F::policy::template write<
                base_type,
                base_type,
                F::mask,
                F::offset
                                     >(&_accumulated_value, value);
            return
                std::move(
                    MergeWrite<Register, (_combined_mask | F::mask)>
                    ::make(_accumulated_value)
                         );
        };
    private:
        static_assert(!Register::shadow::use_shadow,
                      "merge write is not available for shadow value register");
        constexpr MergeWrite() : _accumulated_value(0u) {};
        constexpr MergeWrite(const base_type v) : _accumulated_value(v) {};
        base_type _accumulated_value;
    };
}
#endif  

// Register.h
#ifndef CPPREG_REGISTER_H
#define CPPREG_REGISTER_H
namespace cppreg {
    template <
        Address_t RegAddress,
        Width_t RegWidth,
        typename RegisterType<RegWidth>::type ResetValue = 0x0,
        bool UseShadow = false
    >
    struct Register {
        using type = typename RegisterType<RegWidth>::type;
        using MMIO_t = volatile type;
        constexpr static const Address_t base_address = RegAddress;
        constexpr static const Width_t size = RegWidth;
        constexpr static const type reset = ResetValue;
        using shadow = Shadow<Register, UseShadow>;
        static MMIO_t* rw_mem_pointer() {
            return reinterpret_cast<MMIO_t* const>(base_address);
        };
        static const MMIO_t* ro_mem_pointer() {
            return reinterpret_cast<const MMIO_t* const>(base_address);
        };
        template <typename F>
        inline static MergeWrite<typename F::parent_register, F::mask>
        merge_write(const typename F::type value) noexcept {
            return
                MergeWrite<typename F::parent_register, F::mask>
                ::make(((value << F::offset) & F::mask));
        };
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
                size, value, (F::mask >> F::offset)
                                     >::result::value,
            T
                               >::type&&
        merge_write() noexcept {
            return std::move(T::make());
        };
        static_assert(RegWidth != 0u,
                      "defining a Register type of width 0u is not allowed");
    };
}
#endif  

// Field.h
#ifndef CPPREG_REGISTERFIELD_H
#define CPPREG_REGISTERFIELD_H
namespace cppreg {
    template <
        typename BaseRegister,
        Width_t FieldWidth,
        Offset_t FieldOffset,
        typename AccessPolicy
    >
    struct Field {
        using parent_register = BaseRegister;
        using type = typename parent_register::type;
        using MMIO_t = typename parent_register::MMIO_t;
        using policy = AccessPolicy;
        constexpr static const Width_t width = FieldWidth;
        constexpr static const Offset_t offset = FieldOffset;
        constexpr static const type mask = make_shifted_mask<type>(width,
                                                                   offset);
        constexpr static const bool has_shadow =
            parent_register::shadow::use_shadow;
        template <type value>
        struct check_overflow {
            constexpr static const bool result =
                internals::check_overflow<
                    parent_register::size,
                    value,
                    (mask >> offset)
                                         >::result::value;
        };
        inline static type read() noexcept {
            return policy::template read<MMIO_t, type, mask, offset>(
                parent_register::ro_mem_pointer()
                                                                    );
        };
        template <typename T = type>
        inline static void
        write(const typename std::enable_if<!has_shadow, T>::type value)
        noexcept {
            policy::template write<MMIO_t, type, mask, offset>(
                parent_register::rw_mem_pointer(),
                value
                                                              );
        };
        template <typename T = type>
        inline static void
        write(const typename std::enable_if<has_shadow, T>::type value)
        noexcept {
            RegisterWrite<type, type, mask, offset>
            ::write(&parent_register::shadow::value, value);
            policy::template write<MMIO_t, type, type_mask<type>::value, 0u>(
                parent_register::rw_mem_pointer(),
                parent_register::shadow::value
                                                                            );
        };
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
                parent_register::rw_mem_pointer()
                                                                     );
        };
        template <type value, typename T = void>
        inline static
        typename std::enable_if<
            has_shadow
            &&
            check_overflow<value>::result,
            T
                               >::type
        write() noexcept {
            write(value);
        };
        inline static void set() noexcept {
            policy::template
            set<MMIO_t, type, mask>(parent_register::rw_mem_pointer());
        };
        inline static void clear() noexcept {
            policy::template
            clear<MMIO_t, type, mask>(parent_register::rw_mem_pointer());
        };
        inline static void toggle() noexcept {
            policy::template
            toggle<MMIO_t, type, mask>(parent_register::rw_mem_pointer());
        };
        inline static bool is_set() noexcept {
            return (Field::read() == (mask >> offset));
        };
        inline static bool is_clear() noexcept {
            return (Field::read() == 0u);
        };
        static_assert(parent_register::size >= width,
                      "field width is larger than parent register size");
        static_assert(parent_register::size >= width + offset,
                      "offset + width is larger than parent register size");
        static_assert(FieldWidth != 0u,
                      "defining a Field type of width 0u is not allowed");
    };
}
#endif  

