# cppreg: quick start #
Copyright Sendyne Corp., 2010-2018. All rights reserved ([LICENSE](LICENSE)).


This document is a brief introduction to `cppreg` and how to use it. For more details about the implementation refers to the [API documentation](API.md).


## How to use the library ##
There are two recommended ways to use `cppreg` in a project:

* for CMake-based projects, it is sufficient to add the project directory (using CMake `add_subdirectory`) and then to link against the `cppreg` target:

    ```cmake
    # Include cppreg library.
    # The second argument is not necessarily required depending on the project setup.
    # This will import the cppreg target.
    # Because cppreg is a header-only library the cppreg target is actually an INTERFACE library target.
    add_subdirectory(/path/to/cppreg ${PROJECT_BINARY_DIR)/cppreg)
    
    ...
    
    # Some executable.
    add_executable(firmware firmware.cpp)
    target_link_libraries(firmware cppreg)
    ```
    
* for non-CMake projects, the easiest way is to use the [single header version of cppreg](single/cppreg-all.h) and import/copy it in your project.


### Recommended compiler settings ###
Although `cppreg` is entirely written in C++, there is little (if any) overhead in term of runtime performance if at least some level of optimization are enabled (mostly to take care of inlining). For GCC ARM the following settings are recommended (and similar settings should be used for other compilers): 

* enable `-Og` for debug builds (this helps significantly with inlining),
* for optimized builds (`-O2`,`-O3` or `-Os`) the default settings will be sufficient,
* link time and dead code optimization (LTO and `--gc-section` with `-ffunction-sections` and `-fdata-sections`) can also help produce a more optimized version of the code.


## Example: peripheral setup ##
Consider an arbitrary peripheral with a setup register:

| Absolute address (hex) | Register         | Width (bits) | Access             |
|:----------------------:|:-----------------|:------------:|:------------------:|
| 0xA400 0000 | Peripheral setup register   | 8            | R/W                |

The setup register bits are mapped as:

* FREQ field bits [0:4] to control the peripheral frequency,
* MODE field bits [5:6] to setup the peripheral mode,
* EN field bits [7] to enable the peripheral.

The goal of `cppreg` is to facilitate the manipulation of such a register and the first step is to define custom types (`Register` and `Field`) that maps to the peripheral:

```c++
#include <cppreg.h>
using namespace cppreg;

// Peripheral register.
// The first template parameter is the register address.
// The second template parameter is the register width in bits.
struct Peripheral : Register<0xA4000000, 8u> {

    // When defining a Field-based type:
    // - the first template parameter is the owning register,
    // - the second template parameter is the field width in bits,
    // - the third template parameter is the offset in bits,
    // - the last parameter is the access policy (readable and writable).
    using Frequency = Field<Peripheral, 5u, 0u, read_write>;    // FREQ
    using Mode = Field<Peripheral, 2u, 5u, read_write>;         // MODE
    using Enable = Field<Peripheral, 1u, 7u, read_write>;       // EN
    
    // To enable the peripheral:
    // write 1 to EN field; if the peripheral fails to start this will be reset to zero.
    // To disable the peripheral:
    // clear EN field; no effect if not enabled.
    
};
```

Now let's assume that we want to setup and enable the peripheral following the procedure:

1. we first set the mode, say with value `0x2`,
2. then the frequency, say with value `0x1A`,
3. then write `1` to the enable field to start the peripheral.

This translates to:

```c++
// Setup and enable.
Peripheral::Mode::write<0x2>();
Peripheral::Frequency::write<0x1A>();
Peripheral::Enable::set();

// Check if properly enabled.
if (!Peripheral::Enable::is_set()) {
    // Peripheral failed to start ...
};

...

// Later on we want to disable the peripheral.
Peripheral::Enable::clear();
```

A few remarks:

* in the `write` calls we can also use `write(value)` instead of `write<value>()`; the latter only exists if `value` is `constexpr` (*i.e.*, known at compile time) but the benefits is that it will check for overflow at compile time,
* 1-bit wide `Field`-based type have `is_set` and `is_clear` defined to conveniently query their states.

The advantage of `cppreg` is that it limits the possibility of errors (see the [API documentation](API.md) for more details):

```c++
// Overflow checks.
Peripheral::Mode::write<0x4>();         // Would not compile because it overflows the MODE field.
Peripheral::Frequency::write<0xFF>();   // Idem. This overflows the FREQ field.
Peripheral::Enable::set();

// Instead if you write:
Peripheral::Mode::write(0x4);           // Compile but writes 0x0 to the MODE field.
Peripheral::Frequency::write(0xFF);     // Compile but writes 0x1F to the FREQ field
```

We can even add more expressive methods for our peripheral:

```c++
#include <cppreg.h>
using namespace cppreg;

// Peripheral register.
struct Peripheral : Register<0xA4000000, 8u> {

    using Frequency = Field<Peripheral, 5u, 0u, read_write>;    // FREQ
    using Mode = Field<Peripheral, 2u, 5u, read_write>;         // MODE
    using Enable = Field<Peripheral, 1u, 7u, read_write>;       // EN
    
    // To enable the peripheral:
    // write 1 to EN field; if the peripheral fails to start this will be reset to zero.
    // To disable the peripheral:
    // clear EN field; no effect if not enabled.
    
    // Configuration with mode and frequency.
    template <Mode::type mode, Frequency::type f>
    inline static void configure() {
        // Using template parameters we can check for overflow at compile time.
        Mode::write<mode>();
        Frequency::write<f>();
    };
    
    // Enable method.
    inline static void enable() {
        Enable::set();
    };
    
    // Disable method.
    inline static void disable() {
        Enable::clear();
    };
    
};

Peripheral::configure<0x2, 01A>();
Peripheral::enable();
```


## Example: simple interface for GPIO pins ##
Let's assume we are dealing with a 32 pins GPIO peripheral on some MCU and that the following memory map and registers are available:

| Absolute address (hex) | Register                 | Width (bits) | Access   |
|:----------------------:|:-------------------------|:--:|:------------------:|
| 0xF400 0000 | Port data output register           | 32 | R/W                |
| 0xF400 0004 | Port set output register            | 32 | W (always reads 0) |
| 0xF400 0008 | Port clear output register          | 32 | W (always reads 0) |
| 0xF400 000C | Port toggle output register         | 32 | W (always reads 0) |
| 0xF400 0010 | Port data input output register     | 32 | W (always reads 0) |
| 0xF400 0014 | Port data direction output register | 32 | R/W                |

For each register, the 32 bits maps to the 32 pins of the GPIO peripheral (bit 0 maps to pin 0 and so on). In other words, each register is composed of 32 fields that maps to the GPIO pins.

Using `cppreg` we can define custom types for these registers and define an interface for the GPIO peripheral:

```c++
// GPIO peripheral namespace.
namespace gpio {

    // To define a register type:
    // using x = Register<register_address, register_width in bits>;
    
    // Data output register (PDOR).
    using pdor = Register<0xF4000000, 32u>;

    // Set output register (PSOR).
    using psor = Register<0xF4000004, 32u>;

    // Clear output register (PCOR).
    using pcor = Register<0xF4000008, 32u>;

    // Toggle output register (PTOR).
    using ptor = Register<0xF400000C, 32u>;

    // Data input register.
    using pdir = Register<0xF4000010, 32u>

    // Data direction output register.
    using pddr = Register<0xF4000014, 32u>    

}
```

For the purpose of this example we further assume that we are only interested in two pins of the GPIO: we have a red LED on pin 14 and a blue LED on pin 16. We can now use the `Register` types defined above to map these pins to specific `Field`. Because the logic is independent of the pin number we can even define a generic LED interface:

```c++
// LEDs namespace.
namespace leds {

    // LED interface template.
    template <std::uint8_t Pin>
    struct LED_Interface {

        // Define the relevant fields.
        // Some of these fields are write only (e.g., PSOR) but we define them
        // as read write (it will always read zero but we will not read them).
        using pin_direction = Field<gpio::pddr, 1u, Pin, AccessPolicy::rw>;
        using pin_set = Field<gpio::psor, 1u, Pin, AccessPolicy::rw>;
        using pin_clear = Field<gpio::pcor, 1u, Pin, AccessPolicy::rw>;
        using pin_toggle = Field<gpio::ptor, 1u, Pin, AccessPolicy::rw>;

        // We also define some constants.
        constexpr static const gpio::pddr::type pin_output_dir = 1u;

        // We can now define the static methods of the interface.
        // The pin output direction is set in the init method.
        inline static void on() {
            pin_set::set();      // Set PSOR to 1.
        };
        inline static void off() {
            pin_clear::set();    // Set PCOR to 1.
        };
        inline static void toggle() {
            pin_toggle::set();   // Set PTOR to 1.
        };
        inline static void init() {
            pin_direction::write(pin_output_dir);
            off();
        };

    }

    // Define the types for our two LEDs.
    using red = LED_Interface<14>;
    using blue = LED_Interface<16>;
    
}
```

At this point we have defined an interface to initialize and control the LEDs attached to two GPIO pins. Note that, at no moment we had to deal with masking or shifting operations. Furthermore, we only needed to deal with the register addresses when defining the related types. At compile time, `cppreg` also makes sure that the field actually fits within the register specifications (a `Field` type cannot overflow its `Register` type).

Using this interface it becomes easy to write very expressive code such as:

```c++
leds::red::init();
leds::blue::init();

// Turn on both LEDs.
leds::red::on();
leds::blue::on();

// Wait a bit.
for (std::size_t i = 0; i < 500000; ++i)
    __NOP();

// Turn off the blue LED.
leds::blue::off();
```

A quick note: in this example some of the registers are write-only (set, clear and toggle); in genereal extra care has to be taken when dealing with write-only fields or registers but for this example the implementation still work fine due to the nature of the GPIO registers. Check the [API documentation](API.md) for more details.
