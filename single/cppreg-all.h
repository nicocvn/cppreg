//! cppreg library.
/**
 * @file      cppreg-all.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 */

#include <cstdint>
#include <type_traits>
#include <functional>

// cppreg_Defines.h
#ifndef CPPREG_CPPREG_DEFINES_H
#define CPPREG_CPPREG_DEFINES_H
namespace cppreg {
    using Address_t = std::uintptr_t;
    using Width_t = std::uint8_t;
    using Offset_t = std::uint8_t;
}
#endif  

// AccessPolicy.h
#ifndef CPPREG_ACCESSPOLICY_H
#define CPPREG_ACCESSPOLICY_H
namespace cppreg {
    struct read_only {
        template <typename MMIO_t, typename T>
        inline static T read(const MMIO_t* const mmio_device,
                             const T mask,
                             const Offset_t offset) noexcept {
            return static_cast<T>((*mmio_device & mask) >> offset);
        };
    };
    struct read_write : read_only {
        template <typename MMIO_t, typename T>
        inline static void write(MMIO_t* const mmio_device,
                                 const T mask,
                                 const Offset_t offset,
                                 const T value) noexcept {
            *mmio_device = static_cast<T>((*mmio_device & ~mask) |
                                          ((value << offset) & mask));
        };
        template <typename MMIO_t, typename T>
        inline static void set(MMIO_t* const mmio_device, const T mask)
        noexcept {
            *mmio_device = static_cast<T>((*mmio_device) | mask);
        };
        template <typename MMIO_t, typename T>
        inline static void clear(MMIO_t* const mmio_device, const T mask)
        noexcept {
            *mmio_device = static_cast<T>((*mmio_device) & ~mask);
        };
        template <typename MMIO_t, typename T>
        inline static void toggle(MMIO_t* const mmio_device, const T mask)
        noexcept {
            *mmio_device = static_cast<T>((*mmio_device) ^ mask);
        };
    };
    struct write_only {
        template <typename MMIO_t, typename T>
        inline static void write(MMIO_t* const mmio_device,
                                 const T mask,
                                 const Offset_t offset,
                                 const T value) noexcept {
            *mmio_device = ((value << offset) & mask);
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
    template <typename Register>
    class MergeWrite {
    public:
        using base_type = typename Register::type;
        static MergeWrite create_instance(const base_type value,
                                          const base_type mask) noexcept {
            MergeWrite mw;
            mw._accumulated_value = value;
            mw._combined_mask = mask;
            return mw;
        };
        MergeWrite(MergeWrite&& mw) noexcept
            : _accumulated_value(mw._accumulated_value),
              _combined_mask(mw._combined_mask) {
        };
        MergeWrite(const MergeWrite&) = delete;
        MergeWrite& operator=(const MergeWrite&) = delete;
        ~MergeWrite() {
            typename Register::MMIO_t* const mmio_device =
                Register::rw_mem_pointer();
            *mmio_device = static_cast<base_type>(
                (*mmio_device & ~_combined_mask) |
                ((_accumulated_value) & _combined_mask)
            );
        };
        template <typename F>
        MergeWrite&& with(const base_type value) && noexcept {
            static_assert(std::is_same<
                              typename F::parent_register,
                              Register
                                      >::value,
                          "field is not from the same register in merge_write");
            F::policy::write(&_accumulated_value,
                             F::mask,
                             F::offset,
                             value);
            _combined_mask = _combined_mask | F::mask;
            return std::move(*this);
        };
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
        static_assert(!Register::shadow::use_shadow,
                      "merge write is not available for shadow value register");
        MergeWrite() : _accumulated_value(0u),
                       _combined_mask(0u) {};
        base_type _accumulated_value;
        base_type _combined_mask;
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
        inline static MergeWrite<typename F::parent_register>
        merge_write(const typename F::type value) noexcept {
            return
                MergeWrite<typename F::parent_register>
                ::create_instance(((value << F::offset) & F::mask), F::mask);
        };
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
        constexpr static const Width_t width = FieldWidth;
        constexpr static const Offset_t offset = FieldOffset;
        using policy = AccessPolicy;
        constexpr static const type mask = make_shifted_mask<type>(width,
                                                                   offset);
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
            return
                AccessPolicy::read(parent_register::ro_mem_pointer(),
                                   mask,
                                   offset);
        };
        template <typename T = type>
        inline static void
        write(const typename std::enable_if<
            !parent_register::shadow::use_shadow, T
                                           >::type value) noexcept {
            AccessPolicy::write(parent_register::rw_mem_pointer(),
                                mask,
                                offset,
                                value);
        };
        template <typename T = type>
        inline static void
        write(const typename std::enable_if<
            parent_register::shadow::use_shadow, T
                                           >::type value) noexcept {
            parent_register::shadow::value =
                (parent_register::shadow::value & ~mask) |
                ((value << offset) & mask);
            AccessPolicy::write(parent_register::rw_mem_pointer(),
                                ~(0u),
                                0u,
                                parent_register::shadow::value);
        };
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
        inline static void set() noexcept {
            AccessPolicy::set(parent_register::rw_mem_pointer(), mask);
        };
        inline static void clear() noexcept {
            AccessPolicy::clear(parent_register::rw_mem_pointer(), mask);
        };
        inline static void toggle() noexcept {
            AccessPolicy::toggle(parent_register::rw_mem_pointer(), mask);
        };
        template <typename T = bool>
        inline static typename std::enable_if<FieldWidth == 1, T>::type
        is_set() noexcept {
            return (Field::read() == 1u);
        };
        template <typename T = bool>
        inline static typename std::enable_if<FieldWidth == 1, T>::type
        is_clear() noexcept {
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

