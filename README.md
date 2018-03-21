# cppreg #
Copyright Sendyne Corp., 2010-2018. All rights reserved ([LICENSE](LICENSE)).


## Description ##
`cppreg`is a header-only C++11 library to facilitate the manipulation of MMIO registers (*i.e.*, memory-mapped I/O registers) in embedded devices. The idea is to make it possible to write expressive code and minimize the likelihood of ill-defined expressions when dealing with hardware registers on a MCU. The current features are:

* expressive syntax which shows the intent of the code when dealing with registers and fields,
* efficiency and performance on par with traditional C implementations (*e.g.*, CMSIS C code) when *at least some compiler optimizations* are enabled,
* field access policies (*e.g.*, read-only vs read-write) detect ill-defined access at compile-time,
* compile-time detection of overflow,
* easily extendable to support, for example, mock-up.

For a short introduction and how-to see the [quick start guide](QuickStart.md). A more complete and detailed documentation is available [here](API.md).


## Requirements ##
`cppreg` is designed to be usable on virtually any hardware that statisfies the following requirements:

* MMIO register sizes are integral numbers of bytes (*e.g.*, 8 bits, 16 bits, ...),
* registers are properly aligned: a N-bit register is aligned on a N-bit boundary,

GCC (4.8 and above) and Clang (3.3 and above) are supported and it is expected that any other C++11-compliant compiler should work (see the [quick start guide](QuickStart.md) for recommended compiler settings).


## Manifest ##
This project started when looking at this type of C code:

```c
// Now we enable the PLL as source for MCGCLKOUT.
MCG->C6 |= (1u << MCG_C6_PLLS_SHIFT);

// Wait for the MCG to use the PLL as source clock.
while ((MCG->S & MCG_S_PLLST_MASK) == 0)
    __NOP();
```

This piece of code is part of the clock setup on a flavor of the K64F MCU. `MCG` is a peripheral and `MCG->C6` and `MCG->S` are registers containing some fields which are required to be set to specific values to properly clock the MCU. Some of the issues with such code are:

* the intent of the code is poorly expressed, and it requires at least the MCU data sheet or reference manual to be somewhat deciphered/understood,
* since the offsets and masks are known at compile time, it is error prone and somewhat tedious that the code has to manually implement shifting and masking operations,
* the code syntax itself is extremely error prone; for example, forget the `|` in `|=` and you are most likely going to spend some time debugging the code,
* there is no safety at all, that is, you might overflow the field, or you might try to write to a read-only field and no one will tell you (not the compiler, not the linker, and at runtime this could fail in various ways with little, if any, indication of where is the error coming from).

This does not have to be this way, and C++11 brings a lot of features and concepts that make it possible to achieve the same goal while clearly expressing the intent and being aware of any ill-formed instructions. Some will argue this will come at the cost of a massive performance hit, but this is actually not always the case (and more often than not, a C++ implementation can be very efficient; see [Ken Smith paper] and the example below).

This project has been inspired by the following previous works:

* [Ken Smith] work,
* [CMSIS SVD],
* this [blog post](http://blog.salkinium.com/typesafe-register-access-in-c++/).


[Ken Smith]: https://github.com/kensmith/cppmmio
[CMSIS SVD]: https://github.com/posborne/cmsis-svd
