//! cppreg library.
/**
 * @file      cppreg-all.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2019 Sendyne Corp. All rights reserved.
 */

#include <cstdint>
#include <type_traits>
#include <functional>
#include <limits>

// cppreg_Defines.h
#ifndef CPPREG_CPPREG_DEFINES_H
#define CPPREG_CPPREG_DEFINES_H
namespace cppreg {
using Address = std::uintptr_t;
enum class RegBitSize : std::uint8_t {
    b8,     
    b16,    
    b32,    
    b64     
};
using FieldWidth = std::uint8_t;
using FieldOffset = std::uint8_t;
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
struct TypeTraits {};
template <>
struct TypeTraits<RegBitSize::b8> {
    using type = std::uint8_t;
    constexpr static auto bit_size = std::uint8_t{8};
    constexpr static auto byte_size = std::uint8_t{bit_size / 8};
};
template <>
struct TypeTraits<RegBitSize::b16> {
    using type = std::uint16_t;
    constexpr static auto bit_size = std::uint8_t{16};
    constexpr static auto byte_size = std::uint8_t{bit_size / 8};
};
template <>
struct TypeTraits<RegBitSize::b32> {
    using type = std::uint32_t;
    constexpr static auto bit_size = std::uint8_t{32};
    constexpr static auto byte_size = std::uint8_t{bit_size / 8};
};
template <>
struct TypeTraits<RegBitSize::b64> {
    using type = std::uint64_t;
    constexpr static auto bit_size = std::uint8_t{64};
    constexpr static auto byte_size = std::uint8_t{bit_size / 8};
};
}    
#endif    

// Internals.h
#ifndef CPPREG_INTERNALS_H
#define CPPREG_INTERNALS_H
namespace cppreg {
namespace internals {
template <typename T, T value, T limit>
struct check_overflow : std::integral_constant<bool, value <= limit> {};
template <Address address, std::size_t alignment>
struct is_aligned
    : std::integral_constant<bool, (address & (alignment - 1)) == 0> {};
}    
}    
#endif    

// Memory.h
#ifndef CPPREG_DEV_MEMORY_H
#define CPPREG_DEV_MEMORY_H
namespace cppreg {
template <Address base_address, std::uint32_t pack_byte_size>
struct RegisterPack {
    constexpr static const Address pack_base = base_address;
    constexpr static const std::uint32_t size_in_bytes = pack_byte_size;
};
template <Address mem_address, std::size_t mem_byte_size>
struct MemoryDevice {
    using memory_storage = std::array<volatile std::uint8_t, mem_byte_size>;
    static memory_storage& _mem_storage;
    template <RegBitSize reg_size, std::size_t byte_offset>
    static const volatile typename TypeTraits<reg_size>::type& ro_memory() {
        static_assert(
            internals::is_aligned<mem_address + byte_offset,
                                  std::alignment_of<typename TypeTraits<
                                      reg_size>::type>::value>::value,
            "MemoryDevice:: ro request not aligned");
        return *(reinterpret_cast<const volatile
                                  typename TypeTraits<reg_size>::type*>(
            &_mem_storage[byte_offset]));
    }
    template <RegBitSize reg_size, std::size_t byte_offset>
    static volatile typename TypeTraits<reg_size>::type& rw_memory() {
        static_assert(
            internals::is_aligned<mem_address + byte_offset,
                                  std::alignment_of<typename TypeTraits<
                                      reg_size>::type>::value>::value,
            "MemoryDevice:: rw request not aligned");
        return *(
            reinterpret_cast<volatile typename TypeTraits<reg_size>::type*>(
                &_mem_storage[byte_offset]));
    }
};
template <Address a, std::size_t s>
typename MemoryDevice<a, s>::memory_storage& MemoryDevice<a, s>::_mem_storage =
    *(reinterpret_cast<typename MemoryDevice<a, s>::memory_storage*>(a));
template <typename RegisterPack>
struct RegisterMemoryDevice {
    using mem_device =
        MemoryDevice<RegisterPack::pack_base, RegisterPack::size_in_bytes>;
};
}    
#endif    

// AccessPolicy.h
#ifndef CPPREG_ACCESSPOLICY_H
#define CPPREG_ACCESSPOLICY_H
namespace cppreg {
template <typename T, T mask, FieldOffset offset>
struct is_trivial_rw
    : std::integral_constant<bool,
                             (mask == type_mask<T>::value)
                                 && (offset == FieldOffset{0})> {};
template <typename T, T mask, FieldOffset offset, typename U>
using is_trivial =
    typename std::enable_if<is_trivial_rw<T, mask, offset>::value, U>::type;
template <typename T, T mask, FieldOffset offset, typename U>
using is_not_trivial =
    typename std::enable_if<!is_trivial_rw<T, mask, offset>::value, U>::type;
template <typename MMIO, typename T, T mask, FieldOffset offset>
struct RegisterRead {
    template <typename U = void>
    static T read(const MMIO& mmio_device,
                  is_not_trivial<T, mask, offset, U>* = nullptr) noexcept {
        return static_cast<T>((mmio_device & mask) >> offset);
    }
    template <typename U = void>
    static T read(const MMIO& mmio_device,
                  is_trivial<T, mask, offset, U>* = nullptr) noexcept {
        return static_cast<T>(mmio_device);
    }
};
template <typename MMIO, typename T, T mask, FieldOffset offset>
struct RegisterWrite {
    template <typename U = void>
    static void write(MMIO& mmio_device,
                      T value,
                      is_not_trivial<T, mask, offset, U>* = nullptr) noexcept {
        mmio_device =
            static_cast<T>((mmio_device & ~mask) | ((value << offset) & mask));
    }
    template <typename U = void>
    static void write(MMIO& mmio_device,
                      T value,
                      is_trivial<T, mask, offset, U>* = nullptr) noexcept {
        mmio_device = value;
    }
};
template <typename MMIO, typename T, T mask, FieldOffset offset, T value>
struct RegisterWriteConstant {
    template <typename U = void>
    static void write(MMIO& mmio_device,
                      is_not_trivial<T, mask, offset, U>* = nullptr) noexcept {
        mmio_device =
            static_cast<T>((mmio_device & ~mask) | ((value << offset) & mask));
    }
    template <typename U = void>
    static void write(MMIO& mmio_device,
                      is_trivial<T, mask, offset, U>* = nullptr) noexcept {
        mmio_device = value;
    }
};
struct read_only {
    template <typename MMIO, typename T, T mask, FieldOffset offset>
    static T read(const MMIO& mmio_device) noexcept {
        return RegisterRead<MMIO, T, mask, offset>::read(mmio_device);
    }
};
struct read_write : read_only {
    template <typename MMIO, typename T, T mask, FieldOffset offset>
    static void write(MMIO& mmio_device, const T value) noexcept {
        RegisterWrite<MMIO, T, mask, offset>::write(mmio_device, value);
    }
    template <typename MMIO, typename T, T mask, FieldOffset offset, T value>
    static void write(MMIO& mmio_device) noexcept {
        RegisterWriteConstant<MMIO, T, mask, offset, value>::write(mmio_device);
    }
    template <typename MMIO, typename T, T mask>
    static void set(MMIO& mmio_device) noexcept {
        RegisterWriteConstant<MMIO, T, mask, FieldOffset{0}, mask>::write(
            mmio_device);
    }
    template <typename MMIO, typename T, T mask>
    static void clear(MMIO& mmio_device) noexcept {
        RegisterWriteConstant<MMIO,
                              T,
                              mask,
                              FieldOffset{0},
                              static_cast<T>(~mask)>::write(mmio_device);
    }
    template <typename MMIO, typename T, T mask>
    static void toggle(MMIO& mmio_device) noexcept {
        mmio_device = static_cast<T>((mmio_device) ^ mask);
    }
};
struct write_only {
    template <typename MMIO, typename T, T mask, FieldOffset offset>
    static void write(MMIO& mmio_device, const T value) noexcept {
        RegisterWrite<MMIO, T, type_mask<T>::value, FieldOffset{0}>::write(
            mmio_device, ((value << offset) & mask));
    }
    template <typename MMIO, typename T, T mask, FieldOffset offset, T value>
    static void write(MMIO& mmio_device) noexcept {
        RegisterWriteConstant<MMIO,
                              T,
                              type_mask<T>::value,
                              FieldOffset{0},
                              ((value << offset) & mask)>::write(mmio_device);
    }
};
}    
#endif    

// Mask.h
#ifndef CPPREG_MASK_H
#define CPPREG_MASK_H
namespace cppreg {
template <typename Mask>
constexpr Mask make_mask(const FieldWidth width) noexcept {
    return width == 0 ? static_cast<Mask>(0u)
                      : static_cast<Mask>(
                          (make_mask<Mask>(FieldWidth(width - 1)) << 1) | 1);
}
template <typename Mask>
constexpr Mask make_shifted_mask(const FieldWidth width,
                                 const FieldOffset offset) noexcept {
    return static_cast<Mask>(make_mask<Mask>(width) << offset);
}
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
typename Register::type Shadow<Register, true>::shadow_value = Register::reset;
}    
#endif    

// MergeWrite.h
#ifndef CPPREG_MERGEWRITE_H
#define CPPREG_MERGEWRITE_H
namespace cppreg {
template <typename Register,
          typename Register::type mask,
          FieldOffset offset,
          typename Register::type value>
class MergeWrite_tmpl {
private:
    using base_type = typename Register::type;
    constexpr static auto _accumulated_value =
        base_type{(value << offset) & mask};
    constexpr static auto _combined_mask = mask;
    template <typename F, base_type new_value>
    using propagated =
        MergeWrite_tmpl<Register,
                        (_combined_mask | F::mask),
                        FieldOffset{0},
                        (_accumulated_value & ~F::mask)
                            | ((new_value << F::offset) & F::mask)>;
    MergeWrite_tmpl() = default;
    static_assert(!Register::shadow::value,
                  "merge write is not available for shadow value register");
public:
    static MergeWrite_tmpl create() noexcept {
        return {};
    }
    MergeWrite_tmpl(const MergeWrite_tmpl&) = delete;
    MergeWrite_tmpl& operator=(const MergeWrite_tmpl&) = delete;
    MergeWrite_tmpl& operator=(MergeWrite_tmpl&&) = delete;
    MergeWrite_tmpl operator=(MergeWrite_tmpl) = delete;
    MergeWrite_tmpl(MergeWrite_tmpl&&) = delete;
    void done() const&& noexcept {
        typename Register::MMIO& mmio_device = Register::rw_mem_device();
        RegisterWriteConstant<typename Register::MMIO,
                              typename Register::type,
                              _combined_mask,
                              FieldOffset{0},
                              _accumulated_value>::write(mmio_device);
    }
    template <typename F, base_type field_value>
    propagated<F, field_value>&& with() const&& noexcept {
        static_assert(
            std::is_same<typename F::parent_register, Register>::value,
            "MergeWrite_tmpl:: field is not from the same register");
        constexpr auto no_overflow =
            internals::check_overflow<typename Register::type,
                                      field_value,
                                      (F::mask >> F::offset)>::value;
        static_assert(no_overflow,
                      "MergeWrite_tmpl:: field overflow in with() call");
        return std::move(propagated<F, field_value>{});
    }
};
template <typename Register, typename Register::type mask>
class MergeWrite {
private:
    using base_type = typename Register::type;
    base_type _accumulated_value;
    constexpr static auto _combined_mask = mask;
    template <typename F>
    using propagated = MergeWrite<Register, _combined_mask | F::mask>;
    constexpr MergeWrite() : _accumulated_value{0} {};
    constexpr explicit MergeWrite(const base_type v) : _accumulated_value{v} {};
    static_assert(!Register::shadow::value,
                  "merge write is not available for shadow value register");
public:
    constexpr static MergeWrite create(const base_type value) noexcept {
        return MergeWrite(value);
    }
    MergeWrite(MergeWrite&& mw) noexcept
        : _accumulated_value{mw._accumulated_value} {};
    MergeWrite(const MergeWrite&) = delete;
    MergeWrite& operator=(const MergeWrite&) = delete;
    MergeWrite& operator=(MergeWrite&&) = delete;
    void done() const&& noexcept {
        typename Register::MMIO& mmio_device = Register::rw_mem_device();
        RegisterWrite<typename Register::MMIO,
                      base_type,
                      _combined_mask,
                      FieldOffset{0}>::write(mmio_device, _accumulated_value);
    }
    template <typename F>
    propagated<F> with(const base_type value) const&& noexcept {
        static_assert(
            std::is_same<typename F::parent_register, Register>::value,
            "field is not from the same register in merge_write");
        const auto new_value = static_cast<base_type>(
            (_accumulated_value & ~F::mask) | ((value << F::offset) & F::mask));
        return std::move(propagated<F>::create(new_value));
    }
};
}    
#endif    

// Register.h
#ifndef CPPREG_REGISTER_H
#define CPPREG_REGISTER_H
namespace cppreg {
template <Address reg_address,
          RegBitSize reg_size,
          typename TypeTraits<reg_size>::type reset_value = 0x0,
          bool use_shadow = false>
struct Register {
    using type = typename TypeTraits<reg_size>::type;
    using MMIO = volatile type;
    using shadow = Shadow<Register, use_shadow>;
    constexpr static auto base_address = reg_address;
    constexpr static auto size = TypeTraits<reg_size>::bit_size;
    constexpr static auto reset = reset_value;
    using pack = RegisterPack<base_address, size / 8u>;
    static MMIO& rw_mem_device() {
        using mem_device = typename RegisterMemoryDevice<pack>::mem_device;
        return mem_device::template rw_memory<reg_size, 0>();
    }
    static const MMIO& ro_mem_device() {
        using mem_device = typename RegisterMemoryDevice<pack>::mem_device;
        return mem_device::template ro_memory<reg_size, 0>();
    }
    template <typename F>
    static MergeWrite<typename F::parent_register, F::mask> merge_write(
        const typename F::type value) noexcept {
        return MergeWrite<typename F::parent_register, F::mask>::create(
            static_cast<type>((value << F::offset) & F::mask));
    }
    template <typename F,
              type value,
              typename T = MergeWrite_tmpl<typename F::parent_register,
                                           F::mask,
                                           F::offset,
                                           value>>
    static T&& merge_write() noexcept {
        static_assert(
            internals::check_overflow<type, value, (F::mask >> F::offset)>::
                value,
            "Register::merge_write<value>:: value too large for the field");
        return std::move(T::create());
    }
    static_assert(size != 0u, "Register:: register definition with zero size");
    static_assert(internals::is_aligned<reg_address,
                                        TypeTraits<reg_size>::byte_size>::value,
                  "Register:: address is mis-aligned for register type");
};
}    
#endif    

// RegisterPack.h
#ifndef CPPREG_REGISTERPACK_H
#define CPPREG_REGISTERPACK_H
namespace cppreg {
template <typename RegisterPack,
          RegBitSize reg_size,
          std::uint32_t bit_offset,
          typename TypeTraits<reg_size>::type reset_value = 0x0,
          bool use_shadow = false>
struct PackedRegister : Register<RegisterPack::pack_base + (bit_offset / 8u),
                                 reg_size,
                                 reset_value,
                                 use_shadow> {
    using pack = RegisterPack;
    using base_reg = Register<RegisterPack::pack_base + (bit_offset / 8u),
                              reg_size,
                              reset_value,
                              use_shadow>;
    static typename base_reg::MMIO& rw_mem_device() noexcept {
        using mem_device =
            typename RegisterMemoryDevice<RegisterPack>::mem_device;
        return mem_device::template rw_memory<reg_size, (bit_offset / 8u)>();
    }
    static const typename base_reg::MMIO& ro_mem_device() noexcept {
        using mem_device =
            typename RegisterMemoryDevice<RegisterPack>::mem_device;
        return mem_device::template ro_memory<reg_size, (bit_offset / 8u)>();
    }
    static_assert(TypeTraits<reg_size>::byte_size + (bit_offset / 8u)
                      <= RegisterPack::size_in_bytes,
                  "PackRegister:: packed register is overflowing the pack");
    static_assert(
        internals::is_aligned<RegisterPack::pack_base,
                              TypeTraits<reg_size>::byte_size>::value,
        "PackedRegister:: pack base address is mis-aligned for register type");
    static_assert(
        internals::is_aligned<RegisterPack::pack_base + (bit_offset / 8u),
                              TypeTraits<reg_size>::byte_size>::value,
        "PackedRegister:: offset address is mis-aligned for register type");
};
template <typename... T>
struct PackIndexing {
    using tuple_t = typename std::tuple<T...>;
    constexpr static const std::size_t n_elems =
        std::tuple_size<tuple_t>::value;
    template <std::size_t N>
    using elem = typename std::tuple_element<N, tuple_t>::type;
};
template <std::size_t start, std::size_t end>
struct for_loop {
    template <typename Func>
    static void apply() noexcept {
        Func().template operator()<start>();
        if (start < end)
            for_loop<start + 1ul, end>::template apply<Func>();
    }
#if __cplusplus >= 201402L
    template <typename Op>
    static void apply(Op&& f) noexcept {
        if (start < end) {
            f(std::integral_constant<std::size_t, start>{});
            for_loop<start + 1ul, end>::apply(std::forward<Op>(f));
        };
    }
#endif    
};
template <std::size_t end>
struct for_loop<end, end> {
    template <typename Func>
    static void apply() noexcept {}
#if __cplusplus >= 201402L
    template <typename Op>
    static void apply(Op&& f) noexcept {}
#endif    
};
template <typename IndexedPack>
struct pack_loop : for_loop<0, IndexedPack::n_elems> {};
}    
#endif    

// Field.h
#ifndef CPPREG_REGISTERFIELD_H
#define CPPREG_REGISTERFIELD_H
namespace cppreg {
template <typename BaseRegister,
          FieldWidth field_width,
          FieldOffset field_offset,
          typename AccessPolicy>
struct Field {
    using parent_register = BaseRegister;
    using type = typename parent_register::type;
    using MMIO = typename parent_register::MMIO;
    using policy = AccessPolicy;
    constexpr static auto width = field_width;
    constexpr static auto offset = field_offset;
    constexpr static auto mask = make_shifted_mask<type>(width, offset);
    template <type value, typename T>
    using if_no_shadow =
        typename std::enable_if<!parent_register::shadow::value, T>::type;
    template <type value, typename T>
    using if_shadow =
        typename std::enable_if<parent_register::shadow::value, T>::type;
    static type read() noexcept {
        return policy::template read<MMIO, type, mask, offset>(
            parent_register::ro_mem_device());
    }
    template <typename T = type>
    static void write(const if_no_shadow<type{0}, T> value) noexcept {
        policy::template write<MMIO, type, mask, offset>(
            parent_register::rw_mem_device(), value);
    }
    template <typename T = type>
    static void write(const if_shadow<type{0}, T> value) noexcept {
        RegisterWrite<type, type, mask, offset>::write(
            parent_register::shadow::shadow_value, value);
        policy::
            template write<MMIO, type, type_mask<type>::value, FieldOffset{0}>(
                parent_register::rw_mem_device(),
                parent_register::shadow::shadow_value);
    }
    template <type value, typename T = void>
    static void write(if_no_shadow<value, T>* = nullptr) noexcept {
        policy::template write<MMIO, type, mask, offset, value>(
            parent_register::rw_mem_device());
        static_assert(
            internals::check_overflow<type, value, (mask >> offset)>::value,
            "Field::write<value>: value too large for the field");
    }
    template <type value, typename T = void>
    static void write(if_shadow<value, T>* = nullptr) noexcept {
        write(value);
        static_assert(
            internals::check_overflow<type, value, (mask >> offset)>::value,
            "Field::write<value>: value too large for the field");
    }
    static void set() noexcept {
        policy::template set<MMIO, type, mask>(
            parent_register::rw_mem_device());
    }
    static void clear() noexcept {
        policy::template clear<MMIO, type, mask>(
            parent_register::rw_mem_device());
    }
    static void toggle() noexcept {
        policy::template toggle<MMIO, type, mask>(
            parent_register::rw_mem_device());
    }
    static bool is_set() noexcept {
        return (Field::read() == (mask >> offset));
    }
    static bool is_clear() noexcept {
        return (Field::read() == type{0});
    }
    static_assert(parent_register::size >= width,
                  "Field:: field width is larger than parent register size");
    static_assert(parent_register::size >= width + offset,
                  "Field:: offset + width is larger than parent register size");
    static_assert(width != FieldWidth{0},
                  "Field:: defining a Field type of zero width is not allowed");
};
}    
#endif    

