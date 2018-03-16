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
    enum class RegBitSize {
        b8,     
        b16,    
        b32,    
        b64     
    };
    using FieldWidth_t = std::uint8_t;
    using FieldOffset_t = std::uint8_t;
    template <typename T>
    struct type_mask {
        constexpr static const T value = std::numeric_limits<T>::max();
    };
}
#endif  

// Traits.h
#ifndef CPPREG_TRAITS_H
#define CPPREG_TRAITS_H
namespace cppreg {
    template <RegBitSize S>
    struct TypeTraits;
    template <> struct TypeTraits<RegBitSize::b8> {
        using type = std::uint8_t;
        constexpr static const std::uint8_t bit_size = 8u;
        constexpr static const std::uint8_t byte_size = bit_size / 8u;
    };
    template <> struct TypeTraits<RegBitSize::b16> {
        using type = std::uint16_t;
        constexpr static const std::uint8_t bit_size = 16u;
        constexpr static const std::uint8_t byte_size = bit_size / 8u;
    };
    template <> struct TypeTraits<RegBitSize::b32> {
        using type = std::uint32_t;
        constexpr static const std::uint8_t bit_size = 32u;
        constexpr static const std::uint8_t byte_size = bit_size / 8u;
    };
    template <> struct TypeTraits<RegBitSize::b64> {
        using type = std::uint64_t;
        constexpr static const std::uint8_t bit_size = 64u;
        constexpr static const std::uint8_t byte_size = bit_size / 8u;
    };
}
#endif  

// Internals.h
#ifndef CPPREG_INTERNALS_H
#define CPPREG_INTERNALS_H
namespace cppreg {
namespace internals {
    template <
        typename T,
        T value,
        T limit
    >
    struct check_overflow : std::integral_constant<bool, value <= limit> {};
    template <Address_t address, std::size_t alignment>
    struct is_aligned : std::integral_constant<
        bool,
        (address & (alignment - 1)) == 0
                                              > {
    };
    template <Address_t address, std::uint32_t n, RegBitSize reg_size>
    struct memory_map {
        using mem_array_t = std::array<
            volatile typename TypeTraits<reg_size>::type,
            n / sizeof(typename TypeTraits<reg_size>::type)
                                      >;
        static mem_array_t& array;
        static_assert(
            is_aligned<address, TypeTraits<reg_size>::byte_size>::value,
            "memory_map: base address is mis-aligned for register type"
                     );
    };
    template <Address_t address, std::uint32_t n, RegBitSize reg_size>
    typename memory_map<address, n, reg_size>::mem_array_t&
        memory_map<address, n, reg_size>::array = *(
            reinterpret_cast<typename memory_map<address, n, reg_size>
            ::mem_array_t*>(
                address
            )
        );
}
}
#endif  

// AccessPolicy.h
#ifndef CPPREG_ACCESSPOLICY_H
#define CPPREG_ACCESSPOLICY_H
namespace cppreg {
    template <typename MMIO_t, typename T, T mask, FieldOffset_t offset>
    struct RegisterRead {
        constexpr static const bool is_trivial =
            (mask == type_mask<T>::value) && (offset == 0u);
        template <typename U = void>
        inline static T read(
            const MMIO_t& mmio_device,
            typename std::enable_if<!is_trivial, U*>::type = nullptr
                            ) noexcept {
            return static_cast<T>((mmio_device & mask) >> offset);
        };
        template <typename U = void>
        inline static T read(
            const MMIO_t& mmio_device,
            typename std::enable_if<is_trivial, U*>::type = nullptr
                            ) noexcept {
            return static_cast<T>(mmio_device);
        };
    };
    template <typename MMIO_t, typename T, T mask, FieldOffset_t offset>
    struct RegisterWrite {
        constexpr static const bool is_trivial =
            (mask == type_mask<T>::value) && (offset == 0u);
        template <typename U = void>
        inline static void write(
            MMIO_t& mmio_device,
            T value,
            typename std::enable_if<!is_trivial, U*>::type = nullptr
                                ) noexcept {
            mmio_device = static_cast<T>(
                (mmio_device & ~mask) | ((value << offset) & mask)
            );
        };
        template <typename U = void>
        inline static void write(
            MMIO_t& mmio_device,
            T value,
            typename std::enable_if<is_trivial, U*>::type = nullptr
                                ) noexcept {
            mmio_device = value;
        };
    };
    template <
        typename MMIO_t, typename T, T mask, FieldOffset_t offset, T value
    >
    struct RegisterWriteConstant {
        constexpr static const bool is_trivial =
            (mask == type_mask<T>::value) && (offset == 0u);
        template <typename U = void>
        inline static void write(
            MMIO_t& mmio_device,
            typename std::enable_if<!is_trivial, U*>::type = nullptr
                                ) noexcept {
            mmio_device = static_cast<T>(
                (mmio_device & ~mask) | ((value << offset) & mask)
            );
        };
        template <typename U = void>
        inline static void write(
            MMIO_t& mmio_device,
            typename std::enable_if<is_trivial, U*>::type = nullptr
                             ) noexcept {
            mmio_device = value;
        };
    };
    struct read_only {
        template <typename MMIO_t, typename T, T mask, FieldOffset_t offset>
        inline static T read(const MMIO_t& mmio_device) noexcept {
            return RegisterRead<MMIO_t, T, mask, offset>::read(mmio_device);
        };
    };
    struct read_write : read_only {
        template <typename MMIO_t, typename T, T mask, FieldOffset_t offset>
        inline static void write(MMIO_t& mmio_device,
                                 const T value) noexcept {
            RegisterWrite<MMIO_t, T, mask, offset>::write(mmio_device, value);
        };
        template <
            typename MMIO_t, typename T, T mask, FieldOffset_t offset, T value
        >
        inline static void write(MMIO_t& mmio_device) noexcept {
            RegisterWriteConstant<MMIO_t, T, mask, offset, value>
            ::write(mmio_device);
        };
        template <typename MMIO_t, typename T, T mask>
        inline static void set(MMIO_t& mmio_device)
        noexcept {
            RegisterWriteConstant<MMIO_t, T, mask, 0u, mask>
            ::write(mmio_device);
        };
        template <typename MMIO_t, typename T, T mask>
        inline static void clear(MMIO_t& mmio_device)
        noexcept {
            RegisterWriteConstant<MMIO_t, T, mask, 0u, ~mask>
            ::write(mmio_device);
        };
        template <typename MMIO_t, typename T, T mask>
        inline static void toggle(MMIO_t& mmio_device)
        noexcept {
            mmio_device = static_cast<T>((mmio_device) ^ mask);
        };
    };
    struct write_only {
        template <typename MMIO_t, typename T, T mask, FieldOffset_t offset>
        inline static void write(MMIO_t& mmio_device,
                                 const T value) noexcept {
            RegisterWrite<MMIO_t, T, type_mask<T>::value, 0u>::write(
                mmio_device, ((value << offset) & mask)
                                                                    );
        };
        template <
            typename MMIO_t, typename T, T mask, FieldOffset_t offset, T value
        >
        inline static void write(MMIO_t& mmio_device) noexcept {
            RegisterWriteConstant<
                MMIO_t, T, type_mask<T>::value, 0u, ((value << offset) & mask)
                                 >
            ::write(mmio_device);
        };
    };
}
#endif  

// Mask.h
#ifndef CPPREG_MASK_H
#define CPPREG_MASK_H
namespace cppreg {
    template <typename Mask_t>
    constexpr Mask_t make_mask(const FieldWidth_t width) noexcept {
        return width == 0 ?
               0u
                          :
               static_cast<Mask_t>(
                   (make_mask<Mask_t>(FieldWidth_t(width - 1)) << 1) | 1
               );
    };
    template <typename Mask_t>
    constexpr Mask_t make_shifted_mask(const FieldWidth_t width,
                                       const FieldOffset_t offset) noexcept {
        return static_cast<Mask_t>(make_mask<Mask_t>(width) << offset);
    };
}
#endif  

// ShadowValue.h
#ifndef CPPREG_SHADOWVALUE_H
#define CPPREG_SHADOWVALUE_H
namespace cppreg {
    template <typename Register, bool use_shadow>
    struct Shadow : std::false_type {};
    template <typename Register>
    struct Shadow<Register, true> : std::true_type {
        static typename Register::type shadow_value;
    };
    template <typename Register>
    typename Register::type Shadow<Register, true>::shadow_value =
        Register::reset;
}
#endif  

// MergeWrite.h
#ifndef CPPREG_MERGEWRITE_H
#define CPPREG_MERGEWRITE_H
namespace cppreg {
    template <
        typename Register,
        typename Register::type mask,
        FieldOffset_t offset,
        typename Register::type value
    > class MergeWrite_tmpl {
    private:
        using base_type = typename Register::type;
        static_assert(!Register::shadow::value,
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
            typename Register::MMIO_t& mmio_device =
                Register::rw_mem_device();
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
                typename Register::type, new_value, (F::mask >> F::offset)
                                      >::value),
            T
                               >::type&&
        with() const && noexcept {
            static_assert(std::is_same<
                              typename F::parent_register,
                              Register
                                      >::value,
                          "field is not from the same register in merge_write");
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
            typename Register::MMIO_t& mmio_device =
                Register::rw_mem_device();
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
                                     >(_accumulated_value, value);
            return
                std::move(
                    MergeWrite<Register, (_combined_mask | F::mask)>
                    ::make(_accumulated_value)
                         );
        };
    private:
        static_assert(!Register::shadow::value,
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
        Address_t reg_address,
        RegBitSize reg_size,
        typename TypeTraits<reg_size>::type reset_value = 0x0,
        bool use_shadow = false
    >
    struct Register {
        using type = typename TypeTraits<reg_size>::type;
        using MMIO_t = volatile type;
        using shadow = Shadow<Register, use_shadow>;
        constexpr static const Address_t base_address = reg_address;
        constexpr static const std::uint8_t size =
            TypeTraits<reg_size>::bit_size;
        constexpr static const type reset = reset_value;
        static MMIO_t& rw_mem_device() {
            return *(reinterpret_cast<MMIO_t* const>(base_address));
        };
        static const MMIO_t& ro_mem_device() {
            return *(reinterpret_cast<const MMIO_t* const>(base_address));
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
                type, value, (F::mask >> F::offset)
                                     >::value,
            T
                               >::type&&
        merge_write() noexcept {
            return std::move(T::make());
        };
        static_assert(size != 0u,
                      "Register: register definition with zero size");
        static_assert(
            internals::is_aligned<reg_address, TypeTraits<reg_size>::byte_size>
            ::value,
            "Register: address is mis-aligned for register type"
                     );
    };
}
#endif  

// RegisterPack.h
#ifndef CPPREG_REGISTERPACK_H
#define CPPREG_REGISTERPACK_H
namespace cppreg {
    template <
        Address_t base_address,
        std::uint32_t pack_byte_size
    > struct RegisterPack {
        constexpr static const Address_t pack_base = base_address;
        constexpr static const std::uint32_t size_in_bytes = pack_byte_size;
    };
    template <
        typename RegisterPack,
        RegBitSize reg_size,
        std::uint32_t bit_offset,
        typename TypeTraits<reg_size>::type reset_value = 0x0,
        bool use_shadow = false
    >
    struct PackedRegister : Register<
        RegisterPack::pack_base + (bit_offset / 8u),
        reg_size,
        reset_value,
        use_shadow
                                    > {
        using base_reg = Register<
            RegisterPack::pack_base + (bit_offset / 8u),
            reg_size,
            reset_value,
            use_shadow
                                 >;
        using mem_map_t = internals::memory_map<
            RegisterPack::pack_base,
            RegisterPack::size_in_bytes,
            reg_size
                                               >;
        inline static typename base_reg::MMIO_t& rw_mem_device() noexcept {
            return mem_map_t::array[bit_offset
                                    / TypeTraits<reg_size>::bit_size];
        };
        inline static const typename base_reg::MMIO_t& ro_mem_device() noexcept {
            return mem_map_t::array[bit_offset
                                    / TypeTraits<reg_size>::bit_size];
        };
        static_assert(
            TypeTraits<reg_size>::byte_size + (bit_offset / 8u) <=
                RegisterPack::size_in_bytes,
            "PackRegister: packed register is overflowing the pack"
                     );
        static_assert(
            internals::is_aligned<
                RegisterPack::pack_base,
                TypeTraits<reg_size>::byte_size
                                 >::value,
            "PackedRegister: pack base address is mis-aligned for register type"
                     );
        static_assert(
            internals::is_aligned<
                RegisterPack::pack_base + (bit_offset / 8u),
                TypeTraits<reg_size>::byte_size
                                 >::value,
            "PackedRegister: offset address is mis-aligned for register type"
                     );
    };
    template <typename... T>
    struct PackIndexing {
        using tuple_t = typename std::tuple<T...>;
        template <std::size_t N>
        using regs = typename std::tuple_element<N, tuple_t>::type;
    };
    template <std::size_t start, std::size_t end>
    struct for_loop {
        template <typename Func>
        inline static void loop() noexcept {
            Func().template operator()<start>();
            if (start < end)
                for_loop<start + 1ul, end>::template loop<Func>();
        };
#if __cplusplus >= 201402L
        template <typename Op>
        inline static void apply(Op& f) noexcept {
            if (start < end) {
                f(std::integral_constant<std::size_t, start>{});
                for_loop<start + 1ul, end>::apply(f);
            };
        };
#endif  
    };
    template <std::size_t end>
    struct for_loop<end, end> {
        template <typename Func>
        inline static void loop() noexcept {};
#if __cplusplus >= 201402L
        template <typename Op>
        inline static void apply(Op& f) noexcept {};
#endif  
    };
    template <typename IndexedPack>
    struct range_loop : for_loop<
        0, std::tuple_size<typename IndexedPack::tuple_t>::value
                                > {};
}
#endif  

// Field.h
#ifndef CPPREG_REGISTERFIELD_H
#define CPPREG_REGISTERFIELD_H
namespace cppreg {
    template <
        typename BaseRegister,
        FieldWidth_t field_width,
        FieldOffset_t field_offset,
        typename AccessPolicy
    >
    struct Field {
        using parent_register = BaseRegister;
        using type = typename parent_register::type;
        using MMIO_t = typename parent_register::MMIO_t;
        using policy = AccessPolicy;
        constexpr static const FieldWidth_t width = field_width;
        constexpr static const FieldOffset_t offset = field_offset;
        constexpr static const type mask = make_shifted_mask<type>(width,
                                                                   offset);
        constexpr static const bool has_shadow =
            parent_register::shadow::value;
        template <type value>
        struct check_overflow : internals::check_overflow<
            type,
            value,
            (mask >> offset)
                                                         > {};
        inline static type read() noexcept {
            return policy::template read<MMIO_t, type, mask, offset>(
                parent_register::ro_mem_device()
                                                                    );
        };
        template <typename T = type>
        inline static void
        write(const typename std::enable_if<!has_shadow, T>::type value)
        noexcept {
            policy::template write<MMIO_t, type, mask, offset>(
                parent_register::rw_mem_device(),
                value
                                                              );
        };
        template <typename T = type>
        inline static void
        write(const typename std::enable_if<has_shadow, T>::type value)
        noexcept {
            RegisterWrite<type, type, mask, offset>
            ::write(parent_register::shadow::shadow_value, value);
            policy::template write<MMIO_t, type, type_mask<type>::value, 0u>(
                parent_register::rw_mem_device(),
                parent_register::shadow::shadow_value
                                                                            );
        };
        template <type value, typename T = void>
        inline static
        typename std::enable_if<
            !has_shadow
            &&
            check_overflow<value>::value,
            T
                               >::type
        write() noexcept {
            policy::template write<MMIO_t, type, mask, offset, value>(
                parent_register::rw_mem_device()
                                                                     );
        };
        template <type value, typename T = void>
        inline static
        typename std::enable_if<
            has_shadow
            &&
            check_overflow<value>::value,
            T
                               >::type
        write() noexcept {
            write(value);
        };
        inline static void set() noexcept {
            policy::template
            set<MMIO_t, type, mask>(parent_register::rw_mem_device());
        };
        inline static void clear() noexcept {
            policy::template
            clear<MMIO_t, type, mask>(parent_register::rw_mem_device());
        };
        inline static void toggle() noexcept {
            policy::template
            toggle<MMIO_t, type, mask>(parent_register::rw_mem_device());
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
        static_assert(width != 0u,
                      "defining a Field type of width 0u is not allowed");
    };
}
#endif  

