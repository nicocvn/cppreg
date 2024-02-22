# cppreg: API #
Copyright Sendyne Corp., 2010-2019. All rights reserved ([LICENSE](LICENSE)).


## Introduction ##
`cppreg` provides ways to define custom C++ data types to represent memory-mapped input/output (MMIO) registers and fields. In essence, `cppreg` does contain very little *executable* code, but it does provide a framework to efficiently manipulate MMIO registers and fields.

The entire implementation is encapsulated in the `cppreg::` namespace. All the code examples assume this namespace is accessible (*i.e.*, `using namespace cppreg;` is implicit).


## Overview ##
`cppreg` API makes it possible to:

* define a pack of registers: a register pack is simply a group of registers contiguous in memory (this is often the case when dealing with registers associated with a peripheral),
* define single register at a specific memory address: this is provided as a fallback when the register pack implementation cannot be used,
* define fields within registers (packed or not): a field corresponds to a group of bits within a register and comes with a specific access policy which control read and write access.

The API was designed such that `cppreg`-based code is safer and more expressive than traditional low-level code while providing the same level of performance.

As explained below, when using `cppreg`, registers and fields are defined as C++ types specializing pre-defined template types. This can be done by explicitly deriving from the specialized template type or by using the `using` keyword (both approaches are strictly equivalent). With the exception of the merged write mechanism discussed below, all methods provided by the `cppreg` types are static methods.


## Data types ##
`cppreg` introduces type aliases in order to parameterize the set of data types used in the implementation. By default the following types are defined (see [cppreg_Defines.h](cppreg_Defines.h) for more details):

* `Address` is the data type used to hold addresses of registers and fields; it is equivalent to `std::uintptr_t`,
* register sizes are represented by the enumeration type `RegBitSize`,
* `FieldWidth` and `FieldOffset` are the data types to represent field sizes and offsets; both are equivalent to `std::uint8_t`.

### Register size ###
The `RegBitSize` enumeration type represents the register sizes supported in `cppreg` and the values are:

* `RegBitSize::b8` for 8-bit registers,
* `RegBitSize::b16` for 16-bit registers,
* `RegBitSize::b32` for 32-bit registers,
* `RegBitSize::b64` for 64-bit registers.

The register size is used to define the C++ data type that represent the register content in [Traits.h](Traits.h).


## Register interface ##
In `cppreg`, registers are represented as memory locations that contain fields, and do not provide any methods by themselves. There are two possible ways to define registers:

* for registers which are part of a pack (*i.e.*, group) the `RegisterPack` and `PackedRegister` types should be used,
* for standalone register, the `Register` type is available.

Most of the times registers are part of groups related to peripherals or specific functionalities within a MCU. It is therefore recommended to use the register pack implementation rather than the standalone one. This ensures that the assembly generated from `cppreg`-based code will be optimal. In other words, the difference between packed registers and standalone registers is only a matter of performance in the generated assembly: the packed register interface relies on mapping an array on the pack memory region, which provides to the compiler the ability to use offset between the various registers versus reloading their absolute addresses.

Moreover, the same level of functionality is provided by both implementations (`RegisterPack` is simply deriving from `Register`). That is, a packed register type can be replaced by a standalone register type (and *vice versa*).

### Register pack interface ###
To define a pack of registers:

1. define a `RegisterPack` type with the base address of the pack and the number of bytes,
2. define `PackedRegister` types for all the registers in the pack.

The interface is (see [RegisterPack.h](register/RegisterPack.h)):

* `struct RegisterPack<pack_base_address, pack_size_in_bytes>`:

    | parameter            | description                                |
    |:---------------------|:-------------------------------------------|
    | `pack_base_address`  | starting address of the pack memory region |
    | `pack_size_in_bytes` | size in bytes of the pack memory region    |

* `struct PackedRegister<pack_type, RegBitSize_value, offset_in_bits, reset_value, use_shadow_value>`:

    | parameter            | description                                |
    |:---------------------|:-------------------------------------------|
    | `pack_type`          | starting address of the pack memory region |
    | `RegBitSize_value`   | size in bytes of the pack memory region    |
    | `offset_in_bits`     | offset in bits wrt pack base address       |
    | `reset_value`        | register reset value (defaulted to zero)   |
    | `use_shadow_value`   | enable shadow value if `true` (see below)  |

Note that, the reset value is only used when shadow value support is enabled.

The following example defines a 4 bytes register pack starting at address 0xA4000000 and containing: two 8-bit register and a 16-bit register. The `cppreg` implementation is:

```c++
struct SomePeripheral {

    // Define a register pack:
    // - starting at address 0xA4000000,
    // - with a size of 4 bytes.
    using SomePack = RegisterPack<0xA4000000, 4>;
    
    // Strictly equivalent formlation:
    // struct SomePack : RegisterPack<0xA4000000, 4> {};

    // Define the first 8-bit register:
    using FirstRegister = PackRegister<SomePack, RegBitSize::b8, 8 * 0>;
    
    // Define the second 8-bit register:
    // (the last template parameter indicates the offset in bits)
    using SecondRegister = PackRegister<SomePack, RegBitSize::b8, 8 * 1>;
    
    // Define the 16-bit register:
    // (the last template parameter indicates the offset in bits)
    using ThirdRegister = PackRegister<SomePack, RegBitSize::b16, 8 * 2>;
    
    // Strictly equivalent formulation:
    // struct FirstRegister : PackRegister<SomePack, RegBitSize::b8, 0> {};
    // ...
    
}
```

There are a few requirements when defining packed registers:

* for a register of size N bits, the pack base address has to be aligned on a N bits boundary,
* for a register of size N bits, the pack base address plus the offset has to be aligned on a N bits boundary,
* the offset plus the size of register should be less or equal to the size of the pack.

These requirements are enforced at compile time and errors will be generated if there are not met.

### Standalone register interface ###
The interface for standalone register is (see [Register.h](register/Register.h)):

`struct Register<register_address, RegBitSize_value, reset_value, use_shadow_value>`:

| parameter            | description                                |
|:---------------------|:-------------------------------------------|
| `register_address`   | register absolute address                  |
| `RegBitSize_value`   | size in bytes of the pack memory region    |
| `reset_value`        | register reset value (defaulted to zero)   |
| `use_shadow_value`   | enable shadow value if `true` (see below)  |

Note that, the reset value is only used when shadow value support is enabled.

For example, consider a 32-bit register `SomeRegister` mapped at `0x40004242`. The `Register` type is created using:

```c++
using SomeRegister = Register<0x40004242, RegBitSize::b32>;

// Strictly equivalent formulation:
// struct PeripheralRegister : Register<0x40004242, RegBitSize::b32> {};
```

Similarly to the packed register interface, for a N bits register the address is required to be aligned on a N bits boundary. This requirement is enforced at compile time and an error will be generated if not met.

### Register pack goodies ###
`cppreg` provides a few additional goodies to simplify register packs usage. Let's consider the following peripheral with four registers, each containing a single read/write `Data`field:

```c++
struct Peripheral {

    // Define a register pack:
    // - starting at address 0xA4000000,
    // - with a size of 4 bytes.
    using Pack = RegisterPack<0xA4000000, 4>;
    
    // Registers and fields:
    struct Channel0 : PackedRegister<Pack, RegBistSize::b8, 8 * 0> {
        using Data = Field<Channel0, 8u, 0u, read_write>;
    };
    struct Channel1 : PackedRegister<Pack, RegBistSize::b8, 8 * 1> {
        using Data = Field<Channel1, 8u, 0u, read_write>;
    };
    struct Channel2 : PackedRegister<Pack, RegBistSize::b8, 8 * 2> {
        using Data = Field<Channel2, 8u, 0u, read_write>;
    };
    struct Channel3 : PackedRegister<Pack, RegBistSize::b8, 8 * 3> {
        using Data = Field<Channel3, 8u, 0u, read_write>;
    };
    
}
```

`PackIndexing` can be used to easily access field in packed registers using indexes:

```c++
// Map indexes.
using Channels = PackIndexing<
    Peripheral::Channel0::Data,
    Peripheral::Channel1::Data,
    Peripheral::Channel2::Data,
    Peripheral::Channel3::Data
>;

// Read some data.
const auto x0 = Channels::elem<0>::read();
const auto x1 = Channels::elem<1>::read();
const auto x2 = Channels::elem<2>::read();
const auto x3 = Channels::elem<3>::read();
```

`cppreg` also provides a template-based for loop implementation to simplify such iterations:

```c++
// Map indexes.
using Channels = PackIndexing<
    Peripheral::Channel0::Data,
    Peripheral::Channel1::Data,
    Peripheral::Channel2::Data,
    Peripheral::Channel3::Data
>;

// Define a functor structure to collect data.
static std::array<std::uint8_t, Channels::n_elems> some_buffer = {};
struct ChannelsCollector {
   template <std::size_t index>
   void operator()() {
       some_buffer[index] = Channels::elem<index>::read();
   };
};

// Iterate over the pack.
pack_loop<Channels>::apply<ChannelsCollector>();
```

If C++14 is used, another version of the loop is also available that makes it possible to use polymorphic lambdas:

```c++
// Map indexes.
using Channels = PackIndexing<
    Peripheral::Channel0::Data,
    Peripheral::Channel1::Data,
    Peripheral::Channel2::Data,
    Peripheral::Channel3::Data
>;

// Define a buffer to collect data.
static std::array<std::uint8_t, Channels::n_elems> some_buffer = {};

// Iterate over the pack and use a lambda.
// Note the "auto index" ... this is required because the loop will
// use std::integral_constant to pass the index while iterating.
pack_loop<Channels>::apply([](auto index) {
    some_buffer[index] = Channels::elem<index>::read();
    Channels::elem<index>::template write<index>();
});
```

### Accessing registers memory ###
The register memory can be accessed directly using the static methods:

- `rw_mem_device()` for read/write access.
- `ro_mem_devices` for read-only access.


## Field interface ##
The `Field` template type provided by `cppreg` (see [Field.h](register/Field.h)) contains the added value of the library in terms of type safety, efficiency and expression of intent. The interface is:

`struct Field<register, width_in_bits, offset_in_bits, access_policy>`:

| parameter            | description                                |
|:---------------------|:-------------------------------------------|
| `register`           | register type owning the field             |
| `width_in_bits`      | width in bits (*i.e.*, size)               |
| `offset_in_bits`     | offset in bits wrt to register address     |
| `access_policy`      | access policy type (see below)             |

Compile-time errors will be generated if the field width and offset are not consistent with the parent register size.

### Access policy ###
The last template parameter of a `Field`-based type describes the access policy of the field. Three access policies are available: 

* `read_write` for readable and writable fields,
* `read_only` for read-only fields,
* `write_only` for write-only fields.

Depending on the access policy, the `Field`-based type will provide accessors and/or modifier to its data as described by the following table:

| method        | R/W       | RO        | WO        | description                                           |
|:--------------|:---------:|:---------:|:---------:| :-----------------------------------------------------|
| `read()`      | YES       | YES       | NO        | return the content of the field                       |
| `write(value)`| YES       | NO        | YES       | write `value` to the field                            |
| `set()`       | YES       | NO        | NO        | set all the bits of the field to `1`                  |
| `clear()`     | YES       | NO        | NO        | clear all the bits of the field (*i.e.*, set to `0`)  |
| `toggle()`    | YES       | NO        | NO        | toggle all the bits of the field                      |
| `is_set()`    | YES       | NO        | NO        | `true` is all bits set to 1                           |
| `is_clear()`  | YES       | NO        | NO        | `true` is all bits set to 0                           |

Any attempt at calling an access method which is not provided by a given policy will result in a compilation error. This is one of the mechanism used by `cppreg` to provide safety when accessing registers and fields.

### Example ###
Consider a 32-bit register located at 0x40004242 containing (among other things): a R/W FREQ field over bits [12:17], a WO MODE field over bits [18:21], and a RO STATE field one over bits [28:31]. The `cppreg` implementation is:

```c++
// Register definition with nested fields definitions.
struct SomeRegister : Register<0x40004242, RegBitSize::b32> {
    using Frequency = Field<SomeRegister, 6u, 12u, read_write>;
    using Mode = Field<SomeRegister, 4u, 18u, write_only>;
    using State = Field<SomeRegister, 4u, 28u, read_only>;
    
    // Strictly equivalent formulation:
    // struct Frequency : Field<SomeRegister, 6u, 12u, read_write> {};
    // ...
};

// This would compile:
SomeRegister::Frequency::write<0x10>();
const auto freq = SomeRegister::Frequency::read();
const auto state = SomeRegister::State::read();

// This would not compile (State is read-only):
SomeRegister::State::write<0x1>();
const auto mode = SomeRegister::Mode::read();

// This would compile ...
// But read the section dedicated to write-only fields !!!
SomeRegister::Mode::write<0xA>();
```

### Constant value and overflow check ###
When performing write operations for any `Field`-based type, `cppreg` distinguishes between constant values (known at compile time) and non-constant values:

```c++
SomeField::write<0xAB>();       // Template version for constant value write.
SomeField::write(0xAB);         // Function argument version.
```

The advantages of using the constant value form are:

* `cppreg` will most of the time use a faster implementation for the write operation (this is particularly true if the field spans an entire register),
* a compile-time error will occur if the value overflow the field.

Note that, even when using the non-constant value form overflow will not occur: only the bits fitting in the `Field`-type will be written and any data that does not fit the region of the memory assigned to the `Field`-type will not be modified. For example:

```c++
// Register definition with nested fields definitions.
struct SomeRegister : SomeRegister <0x40004242, RegBitSize::b32> {
    using Frequency = Field<SomeRegister, 8u, 12u, read_write>;
};

// These two calls are strictly equivalent:
SomeRegister::Frequency::write(0xAB);
SomeRegister::Frequency::write<0xAB>();

// This call does not perform a compile-time check for overflow:
// But this will only write 0x11 to the memory device.
SomeRegister::Frequency::write(0x111); 

// This call does perform a compile-time check for overflow and will not compile:
SomeRegister::Frequency::write<0x111>();
```


## Shadow value: a workaround for write-only fields ##
Write-only fields are somewhat special and extra-care has to be taken when manipulating them. The main difficulty resides in the fact that write-only field can be read but the value obtained by reading it is fixed (*e.g.* it always reads as zero). `cppreg` assumes that write-only fields can actually be read from; if such an access on some architecture would trigger an error (*à la FPGA*) then `cppreg` is not a good choice to deal with write-only fields on this particular architecture.

Consider the following situation:

```c++
struct Reg : Register <0x00000001, RegBitSize::b8> {
    using f1 = Field<Reg, 1u, 0u, read_write>;
    using f2 = Field<Reg, 1u, 1u, write_only>; // Always reads as zero.
}
```

Here is what will be happening (assuming the register is initially zeroed out):

```c++
Reg::f1::write<0x1>();  // Reg = (... 0000) | (... 0001) = (... 0001)
Reg::f2::write<0x1>();  // Reg = (... 0010), f1 got wiped out.
Reg::f1::write<0x1>();  // Reg = (... 0000) | (... 0001) = (... 0001), f2 wiped out cause it reads as zero.
```

This shows two issues:

* the default `write` implementation for a write-only field will wipe out the register bits that are not part of the field,
* when writing to the read-write field it wipes out the write-only field because there is no way to retrieve the value that was previously written.

On the other hand, if we were considering an example where a single write-only field extend over an entire register there will be no issue.

As a workaround, `cppreg` offers a shadow value implementation which mitigates the issue by tracking the register value. This implementation can be triggered when defining a register type by using an explicit reset value and a boolean flag:

```c++
struct Reg : Register<
    0x40004242,         // Register address
    RegBitSize::b32,    // Register size
    0x42u,              // Register reset value
    true                // Enable shadow value for the register
    >
{
    using f1 = Field<Reg, 1u, 0u, read_write>;
    using f2 = Field<Reg, 1u, 1u, write_only>; // Always reads as zero.
};
```

The shadow value implementation for a write-only field works as follows:

* at static initialization time, the reset value of the register owning the field is used to initialize the shadow value (the shadow value is used for the entire register content),
* at each write access to any of the register fields, the shadow value will be updated and written to the entire register memory.

This mechanism ensures that the register value is always consistent. This comes at the price of additional memory (storage for the shadow value) and instructions (updating and copying the shadow value).

A few safety guidelines:

* the register shadow value can be accessed directly from the register type but this value should not be modified manually (it is intended to provide read access),
* if the shadow value implementation is used then it should be used everywhere the register is accessed, otherwise the shadow value will be out of sync,
* in case a shadow value register contains fields that can be modified directly by hardware, the user should implement a synchronization mechanism before performing writing operations.


## MergeWrite: writing to multiple fields at once ##
It is sometimes the case that multiple fields within a register needs to be written at the same time. For example, when setting the clock dividers in a MCU it is often recommended to write all their values to the corresponding register at the same time (to avoid mis-clocking part of the MCU).

Consider the following setup (not so artificial; it is inspired by a real flash memory controller peripheral):

```c++
struct FlashCtrl : Register<0xF0008282, RegBitSize::b8> {

    // Command field.
    // Can bet set to various values to trigger write, erase or check operations.
    using Command = Field<FlashCtrl, 4u, 0u, read_write>;
    
    // Set if a flash write/erase command was done to a protected section.
    // To clear write 1 to it.
    // If set this should be cleared prior to starting a new command.
    using ProtectionError = Field<FlashCtrl, 1u, 4u, read_write>;

    // Command complete field.
    // Read behavior: 1 = command completed, 0 = command in progress
    // Write behavior: 1 = Start command, 0 = no effect
    using CommandComplete = Field<FlashCtrl, 1u, 7u, read_write>;
    
};
```

Now let's assume the following scenario:

1. The previous flash command failed because it attempted to write or erase in a protected section, at that point the content of the `FlashCtrl` register is `1001 XXXX` where `XXXX` is whatever value associated with the command that failed.
2. Before we can perform a new flash command we need to clear the `ProtectionError` by writing 1 to it (otherwise the new command will not be started); so one could think about doing:

    ```c++
    FlashCtrl::ProtectionError::set();    // Write to ProtectionError to clear it.
    ```

    however this write `1000 XXXX | 0001 0000 = 1001 XXXX` at the register level and thus start the command that previously failed.
3. At this point one could try to set the value for the new command but that will fail as well (because `ProtectionError` was not cleared and it is required to be).
4. A possible alternative would be to fully zero out the `FlashCtrl` register but that would somewhat defeat the purpose of `cppreg`.

For this kind of situation a *merged write* mechanism was implemented in `cppreg` to merge multiple write operations into a single one. This makes it possible to write the following code to solve the flash controller issue:


```c++
// Write to both ProtectionError and CommandComplete.
FlashCtrl::merge_write<FlashCtrl::ProtectionError, 0x1>().with<FlashCtrl::CommandComplete, 0x0>().done();

// This will correspond to write with a mask set to 1001 0000,
// which boils down to write (at the register level): 
// 0000 XXXX | 0001 0000 = 0001 XXXX ... CommandComplete is not set to 1!
```

The `merge_write` method is only available in register type (`PackedRegister` or `Register`) that do not enable the shadow value mechanism. The `Field`-based types used in the chained call are required to *be from* the register type used to call `merge_write`. In addition, the `Field`-types are also required to be writable. By design, the successive write operations have to be chained, that is, it is not possible to capture a `merge_write` context and add other write operations to it; it always has to be of the form: `register::merge_write<field1, xxx>().with<field2, xxx>(). ... .done()`.

**Warning:** if`done()` is not called at the end of the successive write operations no write at all will be performed.

Similarly to regular write operations it is recommended to use the template version (as shown in the example) if possible: this will enable overflow checking and possibly use faster write implementations. If not possible the values to be written are passed as arguments to the various calls.
