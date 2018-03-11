# cppreg: API #
Copyright Sendyne Corp., 2010-2018. All rights reserved ([LICENSE](LICENSE)).


## Introduction ##
`cppreg` provides ways to define custom C++ data types to represent memory-mapped input/output (MMIO) registers and fields. In essence, `cppreg` does contain very little *executable* code, but it does provide a framework to efficiently manipulate MMIO registers and fields.

`cppreg` is primarily designed to be used in applications on *ARM Cortex-M*-like hardware, that is, MCUs with 32 bits registers and address space. It can easily be extended to support other types of architecture but this is not provided out-of-the-box.

The entire implementation is encapsulated in the `cppreg::` namespace.

## Overview ##
`cppreg` provides two template structures that can be customized:

* `Register`: used to define a MMIO register and its memory device,
* `Field`: used to define a field in a MMIO register.

The `Register` type itself is simply designed to keep track of the register address, size and other additional data (*i.e.*, reset value and shadow value setting). The `Field` type is the most interesting one as it is the type that provides access to part of the register memory device depending on the access policy.


## Data types ##
`cppreg` introduces type aliases in order to parameterize the set of data types used in the implementation. By default the following types are defined (see [cppreg_Defines.h](cppreg_Defines.h) for more details):

* `Address_t` is the data type used to hold addresses of registers and fields; it is equivalent to `std::uintptr_t`,
* `Width_t` and `Offset_t` are the data types to represent register and field sizes and offsets; both are equivalent to `std::uint8_t` (this effectively limits the maximal width and offset to 256).

The data type used to manipulate register and field content is derived from the register size. At the moment only 32-bits, 16-bits, and 8-bits registers are supported but additional register sizes can easily be added (see [Traits.h](register/Traits.h)).


## Register ##
The `Register` type implementation (see [Register.h](register/Register.h)) is designed to encapsulate details relevant to a particular MMIO register and provides access to the register memory. In `cppreg` the data type used to represent the memory register is always marked as `volatile`.

To implement a particular register the following information are required at compile time:

* the address of the register,
* the register size (required to be different from `0`),
* the reset value of the register (this is optional and is defaulted to zero),
* a optional boolean flag to indicate if shadow value should be used (see below; this is optional and not enabled by default).

For example, consider a 32-bits register `PeripheralRegister` mapped at `0x40004242`. The `Register` type can be derived from to create a `PeripheralRegister` C++ type:

```c++
// Register is defined as a struct so public inheritance is the default.
struct PeripheralRegister : Register<0x40004242, 32u> {};
```

If `PeripheralRegister` has a reset value of `0xF0220F00` it can be added to the type definition by adding a template parameter (this is only useful when enabling shadow value as explained later):

```c++
struct PeripheralRegister : Register<0x40004242, 32u, 0xF0220F00> { ... };
```

Note that, it is also possible to simply define a type alias:

```c++
using PeripheralRegister = Register<0x40004242, 32u, 0xF0220F00>;
```

As we shall see below, the derived type `PeripheralRegister` is not very useful by itself. The benefit comes from using it to define `Field`-based types.


## Field ##
The `Field` type provided by `cppreg` (see [Field.h](register/Field.h)) contains the added value of the library in terms of type safety, efficiency and expression of intent. It is defined as a template structure and in order to define a custom field type the following information are required at compile time:

* a `Register`-type describing the register in which the field memory resides,
* the width of the field (required to be different from `0`),
* the offset of the field in the register,
* the access policy of the field (*i.e.*, read-write, read-only, or write-only).

Assume that the register `PeripheralRegister` from the previous example contains a 6-bits field `Frequency ` with an offset of 12 bits (with respect to the register base address; that is, starting at the 13-th bits because the first bit is the 0-th bit). The corresponding custom `Field` type would be defined as:

```c++
using Frequency = Field<PeripheralRegister, 6u, 12u, read_write>;
```

It can also be nested with the definition of `PeripheralRegister`:

```c++
// Register definition with nested field definition.
struct PeripheralRegister : Register<0x40004242, 32u> {
    using Frequency = Field<PeripheralRegister, 6u, 12u, read_write>; 
};

// This is strictly equivalent to:
namespace PeripheralRegister {
    using _REG = Register<0x40004242, 32u>;
    using Frequency = Field<_REG, 6u, 12u, read_write>; 
}

// Or even:
// (again, Field is defined as a struct so public inheritance is the default.
struct PeripheralRegister : Register<0x40004242, 32u> {
    struct Frequency : Field<PeripheralRegister, 6u, 12u, read_write> {};
};
```

which then makes it possible to write expression like:

```c++
PeripheralRegister::Frequency::clear();
PeripheralRegister::Frequency::write(0x10u);
```

to clear the `Frequency` register and then write `0x10` to it.

As the last example suggests, any `Field`-based type must define its access policy (the last template parameter). Depending on the access policy various static methods are available (or not) to perform read and write operations.

### Access policy ###
The last template parameter of a `Field`-based type describes the access policy of the field. Three access policies are available: 

* `read_write` for readable and writable fields,
* `read_only` for read-only fields,
* `write_only` for write-only fields.

Depending on the access policy, the `Field`-based type will provide accessors and/or modifier to its data as described by the following table:

| Method        | R/W       | RO        | WO        | Description                                           |
|:--------------|:---------:|:---------:|:---------:| :-----------------------------------------------------|
| `read()`      | YES       | YES       | NO        | return the content of the field                       |
| `write(value)`| YES       | NO        | YES       | write `value` to the field                            |
| `set()`       | YES       | NO        | NO        | set all the bits of the field to `1`                  |
| `clear()`     | YES       | NO        | NO        | clear all the bits of the field (*i.e.*, set to `0`)  |
| `toggle()`    | YES       | NO        | NO        | toggle all the bits of the field                      |
| `is_set()`    | YES       | NO        | NO        | `true` is all bits set to 1                           |
| `is_clear()`  | YES       | NO        | NO        | `true` is all bits set to 0                           |

Any attempt at calling an access method which is not provided by a given policy will result in a compilation error. This is one of the mechanism used by `cppreg` to provide safety when accessing registers and fields.

For example using our previous `PeripheralRegister` example:

```c++
// Register definition with nested fields definitions.
struct PeripheralRegister : Register<0x40004242, 32u> {
    using Frequency = Field<PeripheralRegister, 6u, 12u, read_write>; 
    using Mode = Field<PeripheralRegister, 4u, 18u, write_only>; 
    using State = Field<PeripheralRegister, 4u, 18u, read_only>;  
};

// This would compile:
PeripheralRegister::Frequency::write(0x10);
const auto freq = PeripheralRegister::Frequency::read();
const auto state = PeripheralRegister::State::read();

// This would not compile:
PeripheralRegister::State::write(0x1);
const auto mode = PeripheralRegister::Mode::read();

// This would compile ...
// But read the section dedicated to write-only fields.
PeripheralRegister::Mode::write(0xA);
```

### Constant value and overflow check ###
When performing write operations for any `Field`-based type, `cppreg` distinguishes between constant values (known at compile time) and non-constant values:

```c++
SomeField::write<0xAB>();       // Template version for constant value write.
SomeField::write(0xAB);         // Function argument function.
```

The advantages of using the constant value version are:

* `cppreg` will (most of the time) use a faster implementation for the write operation,
* a compile-time error will occur if the value overflow the field.

**Recommendation:** use the constant value version whenever it is possible.

Note that, even when using the non-constant value version overflow will not occur: only the bits part of the `Field`-type will be written and any data that does not fit the region of the memory device assigned to the `Field`-type will not be modified:

```c++
// Register definition with nested fields definitions.
struct PeripheralRegister : Register<0x40004242, 32u> {
    using Frequency = Field<PeripheralRegister, 8u, 12u, read_write>;
};

// These two calls are strictly equivalent:
PeripheralRegister::Frequency::write(0xAB);
PeripheralRegister::Frequency::write<0xAB>();

// This call does not perform a compile-time check for overflow:
PeripheralRegister::Frequency::write(0x111);    // But this will only write 0x11 to the memory device.

// This call does perform a compile-time check for overflow and will not compile:
PeripheralRegister::Frequency::write<0x111>();
```


## Shadow value: a workaround for write-only fields ##
Write-only fields are somewhat special as extra-care has to be taken when manipulating them. The main difficulty resides in the fact that write-only field can be read but the value obtained by reading it is fixed (*e.g.*, it always reads as zero). `cppreg` assumes that write-only fields can actually be read from; if such an access on some given architecture would trigger an error (*à la FPGA*) then `cppreg` is not a good choice to deal with write-only fields on this particular architecture. 

Consider the following situation:

```c++
struct Reg : Register <0x00000001, 8u> {
    using f1 = Field<Reg, 1u, 0u, read_write>;
    using f2 = Field<Reg, 1u, 1u, write_only>; // Always reads as zero.
}
```

Here is what will be happening (assuming the register is initially zeroed out):

```c++
Reg::f1::write<0x1>();      // reg = (... 0000) | (... 0001) = (... 0001)
Reg::f2::write<0x1>();      // reg = (... 0010), f1 got wiped out.
Reg::f1::write<0x1>();      // reg = (... 0000) | (... 0001) = (... 0001), f2 wiped out cause it reads as zero.
```

This shows two issues:

* the default `write` implementation for a write-only field will wipe out the register bits that are not part of the field,
* when writing to the read-write field it wipes out the write-only field because there is no way to retrieve the value that was previously written.

As a workaround, `cppreg` offers a shadow value implementation which mitigates the issue by tracking the register value. This implementation can be triggered when defining a register type by using an explicit reset value and a boolean flag:

```c++
struct Reg : Register<
    0x40004242,         // Register address
    32u,                // Register size
    0x42u               // Register reset value
    true                // Enable shadow value for the register
    >
{
    using f1 = Field<Reg, 1u, 0u, read_write>;
    using f2 = Field<Reg, 1u, 1u, write_only>; // Always reads as zero.
};
```

The shadow value implementation for a write-only field works as follow:

* at static initialization time, the reset value of the register owning the field is used to initialize the shadow value (the shadow value is used for the entire register content),
* at each write access to any of the register fields, the shadow value will be updated and written to the entire register memory.

This mechanism ensures that the register value is always consistent. This comes at the price of additional memory (storage for the shadow value) and instructions (updating and copying the shadow value).

A few safety guidelines:

* the register shadow value can be accessed directly from the register type but this value should not be modified manually (it is intended to provide read access),
* if the shadow value implementation is used then it should be used everywhere the register is accessed, otherwise the shadow value will be out of sync,
* in case a shadow value register contains fields that can be modified directly by hardware, the user should implement a synchronization mechanism before performing writing operations.


## MergeWrite: writing to multiple fields at once ##
It is sometimes the case that multiple fields within a register needs to be written at the same time. For example, when setting the clock dividers in a MCU it is often recommended to write all their values to the corresponding register at the same time (to avoid overclocking part of the MCU).

Consider the following setup (not so artifical; it is inspired by a real flash memory controller peripheral):

```c++
struct FlashCtrl : Register<0xF0008282, 8u> {

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

1. The previous flash command failed because it attempted to write or erase in a protected section, at that point the content of the `FlashCtrl` register is `1001 XXXX` where `XXXX` is whatver value associated with the command that failed.
2. Before we can perform a new flash command we need to clear the `ProtectionError` by writing 1 to it (otherwise the new command will not be started); so one could think about doing:

    ```c++
    FlashCtrl::ProtectionError::set();    // Write to ProtectionError to clear it.
    ```

    however this write at `1000 XXXX | 0001 0000 = 1001 XXXX` at the register level and thus start the command that previously failed.
3. At this point one could try to set the value for the new command but that will fail as well (because `ProtectionError` was not cleared and it is required to be).
4. A possible alternative would be to fully zero out the `FlashCtrl` register but that would somewhat defeat the purpose of `cppreg`.

For this kind of situation a *merge write* mechanism was implemented in `cppreg` to merge multiple write operations into a single one. This makes it possible to write the following code to solve the flash controller issue:


```c++
// Write to both ProtectionError and CommandComplete.
FlashCtrl::merge_write<FlashCtrl::ProtectionError, 0x1>().with<FlashCtrl::CommandComplete, 0x0>().done();

// This will correspond to write with a mask set to 1001 0000,
// which boils down to write (at the register level): 
// 0000 XXXX | 0001 0000 = 0001 XXXX ... CommandComplete is not set to 1 !
```

The `merge_write` method is only available in `Register`-based type that do not enable the shadow value mechanism. The `Field`-based types used in the chained call are required to *be from* the `Register` type used to call `merge_write`. In addition, the `Field`-types are also required to be writable. By design, the successive write operations have to be chained, that is, it is not possible to capture a merged write context and add other write operations to it; it always has to be of the form: `register::merge_write<field1, xxx>().with<field2, xxx>(). ... .done()`.

**Warning:** if`done()` is not called at the end of the successive write operations no write at all will be performed.

Similarly to regular write operations it is recommended to use the template version (as shown in the example) if possible: this will enable overflow checking and possibly use faster write implementations. If not possible the values to be written are passed as arguments to the various methods.

