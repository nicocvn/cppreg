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


## Prologue ##
When developing firmware code for embedded MCUs it is customary that peripherals and devices are configured and operated through MMIO registers (for example, a UART or GPIO peripheral). For a given peripheral the related registers are grouped together at a specific location in memory. `cppreg` makes it possible to *map* C++ constructs to such peripherals and registers in order to get safer and more expressive code. The following two examples illustrate how to define such constructs using `cppreg`.


## Example: peripheral setup ##
Consider an arbitrary peripheral with the following registers:

| Address (hex) | Register         | Width (bits) | Access             |
|:-------------:|:-----------------|:------------:|:------------------:|
| 0xA400 0000   | Setup register   | 8            | R/W                |
| 0xA400 0001   | RX data register | 8            | R                  |
| 0xA400 0002   | TX data register | 8            | W                  |

The setup register bits are mapped as:

* FREQ field bits [0:4] to control the peripheral frequency,
* MODE field bits [5:6] to setup the peripheral mode,
* EN field bits [7] to enable the peripheral.

The RX and TX data registers both contain a single DATA field occupying the whole register. The DATA field is read-only for the RX register and write-only for the TX register.

The goal of `cppreg` is to facilitate the manipulation of such a peripheral. This can be done as follow:

```c++
#include <cppreg.h> // use cppreg-all.h instead if you are using the single header.
using namespace cppreg;

// Peripheral structure.
// This will contain all the peripheral registers definitions.
struct Peripheral {

    // Define a register pack type.
    // This is used to indicate that the register are packed together in memory.
    using periph_pack = RegisterPack<
        0xA400 0000,        // Base address of the pack (i.e., peripheral).
        3                   // Number of bytes for all peripheral registers.
    >;
    
    // Define the setup register and the fields.
    struct Setup : PackedRegister<
        periph_pack,                // Pack to which the register belongs to.
        0,                          // Offset in bits from the base.
        8                           // Register width in bits 
    > {
    
        // When defining a Field-based type:
        // - the first template parameter is the owning register,
        // - the second template parameter is the field width in bits,
        // - the third template parameter is the offset in bits,
        // - the last parameter is the access policy.
        using Frequency = Field<Setup, 5u, 0u, read_write>;    // FREQ
        using Mode = Field<Setup, 2u, 5u, read_write>;         // MODE
        using Enable = Field<Setup, 1u, 7u, read_write>;       // EN
    
    };
    
    // Define the RX data register.
    struct RX : PackedRegister<
        periph_pack,                // Pack to which the register belongs to.
        8,                          // Offset in bits from the base.
        8                           // Register width in bits 
    > {
        using Data = Field<RX, 8u, 0u, read_only>;
    };
    
    // Define the RX data register.
    struct TX : PackedRegister<
        periph_pack,                // Pack to which the register belongs to.
        8 * 2,                      // Offset in bits from the base.
        8                           // Register width in bits 
    > {
        using Data = Field<TX, 8u, 0u, read_only>;
    };

};
```

For more details about the various types (`RegisterPack`, `PackedRegister`, and `Field` see the [API documentation](API.md)).

Now let's assume that we want to setup and enable the peripheral following the procedure:

1. we first set the mode, say with value `0x2`,
2. then the frequency, say with value `0x1A`,
3. then write `1` to the enable field to start the peripheral.

Once enabled we also want to implement a echo loop that will simply read data from the RX register and put them in the TX register so that the peripheral will echo whatever it receives.

This translates to:

```c++
// Setup and enable.
Peripheral::Setup::Mode::write<0x2>();
Peripheral::Setup::Frequency::write<0x1A>();
Peripheral::Setup::Enable::set();

// Check if properly enabled.
if (!Peripheral::Setup::Enable::is_set()) {
    // Peripheral failed to start ...
};

// Echo loopback.
while (true) {

    // Read data.
    const auto incoming_data = Peripheral::RX::Data::read();
    
    // Echo the data.
    Peripheral::TX::Data::write(incoming_data);

};
```

A few remarks:

* the `write` calls for the `Setup` register pass the data as template arguments, while the write call for the `TX` register pass it as a function argument: if the value to be written is known at compile time it is recommended to use the template form; the template form will detect overflow (see below) and will also make it possible to use a faster write implementation in some cases,
* `Field`-based types have `is_set` and `is_clear` defined to conveniently query their states.

In this example, we can already see how `cppreg` limits the possibility of errors (see the [API documentation](API.md) for more details):

```c++
// Overflow checks.
Peripheral::Setup::Mode::write<0x4>();         // Would not compile because it overflows the MODE field.
Peripheral::Setup::Frequency::write<0xFF>();   // Idem. This overflows the FREQ field.
Peripheral::Setup::Enable::set();

// Instead if you write:
Peripheral::Setup::Mode::write(0x4);           // Compile but writes 0x0 to the MODE field.
Peripheral::Setup::Frequency::write(0xFF);     // Compile but writes 0x1F to the FREQ field

// Access policies.
Peripheral::RX::Data::write<0x1>();            // Would not compile because read-only.
Peripheral::TX::Data::read();                  // Would not compile because write-only.
```

We can even add more expressive methods for our peripheral:

```c++
// Peripheral register.
struct PeripheralInterface : Peripheral {
    
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
    
    // ...
    
};

PeripheralInterface::configure<0x2, 01A>();
PeripheralInterface::enable();
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

    // Register pack.
    // 6 x 32-bits = 6 x 4 bytes.
    using gpio_pack = RegisterPack<0xF4000000, 6 * 4>;
    
    // Data output register (PDOR).
    using pdor = PackedRegister<gpio_pack, 0 * 32, 32u>;

    // Set output register (PSOR).
    using psor = PackedRegister<gpio_pack, 1 * 32, 32u>;

    // Clear output register (PCOR).
    using pcor = PackedRegister<gpio_pack, 2 * 32, 32u>;

    // Toggle output register (PTOR).
    using ptor = PackedRegister<gpio_pack, 3 * 32, 32u>;

    // Data input register.
    using pdir = PackedRegister<gpio_pack, 4 * 32, 32u>;

    // Data direction output register.
    using pddr = PackedRegister<gpio_pack, 5 * 32, 32u>;

}
```

For the purpose of this example we further assume that we are only interested in two pins of the GPIO: we have a red LED on pin 14 and a blue LED on pin 16. We can now use the `PackedRegister` types defined above to map these pins to specific `Field`. Because the logic is independent of the pin number we can even define a generic LED interface:

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
            off();
            pin_direction::write(pin_output_dir);
        };

    }

    // Define the types for our two LEDs.
    using red = LED_Interface<14>;
    using blue = LED_Interface<16>;
    
}
```

At this point we have defined an interface to initialize and control the LEDs attached to two GPIO pins. Note that, at no moment we had to deal with masking or shifting operations. Furthermore, we only needed to deal with the register addresses when defining the mapping. At compile time, `cppreg` also makes sure that the field actually fits within the register specifications (a `Field` type cannot overflow its `Register` type). Similarly, `cppreg` also checks that packed registers are properly aligned and fit within the pack.

Using this interface it becomes easy to write very expressive code such as:

```c++
leds::red::init();
leds::blue::init();

// Turn on both LEDs.
leds::red::on();
leds::blue::on();

// Wait a bit.
for (std::size_t i = 0; i < 500000; ++i)
    asm("nop");

// Turn off the blue LED.
leds::blue::off();
```

A quick note: in this example some of the registers are write-only (set, clear and toggle); in genereal extra care has to be taken when dealing with write-only fields or registers but for this example the implementation still work fine due to the nature of the GPIO registers. Check the [API documentation](API.md) for more details.
