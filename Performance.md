# Performance

Cppreg makes use of C++'s "zero overhead" [capabilities](https://www.youtube.com/watch?v=zBkNBP00wJE) in order to optimize away **all** the code that CppReg utilizes to enforce type safety, specifically the very template heavy code. Understandably most would question the validity of such a claim, so this is written as an attempt to prove that a register interface written with Cppreg is just as fast as the corresponding CMSIS based code written in C.

## Test Setup

For a test example, let's use an imaginary Cortex M0 based microcontroller with the intention of having the UART send out a small string and then toggle two LEDs (PIN1 and PIN3) after every full string transmission. Why imaginary? Because a real implimentation will be longer than most screens, but a real implimentation is provided at the end. An example will be written in C using CMSIS style, then in C++ using CPPReg, and a comparison of the assembly output of both using GCC-ARM with links to GodBolt so the examples can be fiddled with.

### Example peripheral

```
# Imaginary super simple GPIO Peripheral
(GPIO_Base) GPIO Peripheral Base Address: 0xF0A03110
    # 16 bits wide (2 bit per pin)
    # 00 = Input, 01 = Output, 10 = Reserved, 11 = Reserved
    # Our LEDS are on PIN1 and PIN3
    GPIO Direction Register: GPIO_Base + 0x00

    # 8 bits wide (1 bit per pin)
    # 0 = Do not toggle, 1 = Toggle
    GPIO Toggle Register: GPIO_Base + 0x02

# Imaginary super simple UART peripheral
(UART_Base) UART Peripheral Base Address: 0xF0A03120
    # 8 bits wide, write bytes to here to insert into the TX FIFO
    UART TX FIFO Register: UART_Base + 0x00

    # 8 bits wide, Status Register
    # BIT 0 (Enable)  = Set to enable UART, Clear to disable.
    # BIT 1 .. 2      = Reserved0, read only
    # Bit 3 (Sending) = Set to send, stays set till TX FIFO empty.
    # BIT 4 .. 7      = Reserved1, read only
    UART Status Register: UART_Base + 0x01
```

## CMSIS Style

This snippet is based on a CMSIS style code, which makes heavy use of preprocessor macros (the defines) and just maps a struct right onto memory.

```c
#include <stdint.h>

// Structs to map onto memory for each peripheral.
#define __IO volatile
typedef struct {
    __IO uint16_t DIRECTION; // Base + 0x00
    __IO uint8_t TOGGLE;    // Base + 0x02
} GPIO_TypeDef;
typedef struct {
    __IO uint8_t TXFIFO;  // Base + 0x00
    __IO uint8_t STATUS;  // Base + 0x01
} UART_TypeDef;

// Memory address for the where peipherals sit.
#define PERIPH_BASE ((uint32_t)0xF0A03110)
#define GPIOA_BASE (PERIPH_BASE + 0x0000)
#define GPIO ((GPIO_TypeDef *) GPIOA_BASE)
#define UART_Base (PERIPH_BASE + 0x0010)
#define UART ((UART_TypeDef *) UART_Base)

void Demo_CMSIS(){
    // Make only PIN1 and PIN3 to output with masking.
    const uint16_t DIRECTION_PIN_MASK = (0b11u << (1 * 2)) | (0b11u << (3 * 2));
     GPIO->DIRECTION = (GPIO->DIRECTION & ~DIRECTION_PIN_MASK) | (1u << (1 * 2)) | (1u << (3 * 2));

    // Enable the UART.
    const uint8_t UART_STATUS_ENABLE = 0b0000'0001u;
    UART->STATUS = UART->STATUS | UART_STATUS_ENABLE;

    // Loop over forever.
    while(true){
        // Put a string into the FIFO.
        // UART->TXFIFO = 'H';
        // UART->TXFIFO = 'i';

        // Start sending out TX FIFO contents.
        const uint8_t UART_STATUS_SENDING = 1u << 3;
        // UART->STATUS = UART->STATUS | UART_STATUS_ENABLE | UART_STATUS_SENDING;

        // Wait till the UART is done.
        while ((UART->STATUS & UART_STATUS_SENDING) != 0u) {}
    }
}
```

## CPPReg style

This is written to mimic the CMSIS example as close as possible.

```c++
#include "cppreg-all.h"

struct GPIO {
    static constexpr uintptr_t GPIO_Base = 0xF0A03110;

    struct Direction : cppreg::Register<GPIO_Base + 0x00, 16u> {
        using PIN0 = cppreg::Field<Direction, 2u, 0 * 2, cppreg::read_write>;
        using PIN1 = cppreg::Field<Direction, 2u, 1 * 2, cppreg::read_write>;
        using PIN2 = cppreg::Field<Direction, 2u, 2 * 2, cppreg::read_write>;
        using PIN3 = cppreg::Field<Direction, 2u, 3 * 2, cppreg::read_write>;
        using PIN4 = cppreg::Field<Direction, 2u, 4 * 2, cppreg::read_write>;
        using PIN5 = cppreg::Field<Direction, 2u, 5 * 2, cppreg::read_write>;
        using PIN6 = cppreg::Field<Direction, 2u, 6 * 2, cppreg::read_write>;
        using PIN7 = cppreg::Field<Direction, 2u, 7 * 2, cppreg::read_write>;
    };

    struct Toggle : cppreg::Register<GPIO_Base + 0x02, 8u> {
        using PIN0 = cppreg::Field<Direction, 1u, 0, cppreg::read_only>;
        using PIN1 = cppreg::Field<Direction, 1u, 1, cppreg::read_only>;
        using PIN2 = cppreg::Field<Direction, 1u, 2, cppreg::read_only>;
        using PIN3 = cppreg::Field<Direction, 1u, 3, cppreg::read_only>;
        using PIN4 = cppreg::Field<Direction, 1u, 3, cppreg::read_only>;
        using PIN5 = cppreg::Field<Direction, 1u, 3, cppreg::read_only>;
        using PIN6 = cppreg::Field<Direction, 1u, 3, cppreg::read_only>;
        using PIN7 = cppreg::Field<Direction, 1u, 3, cppreg::read_only>;
    };
};

struct UART {
    static constexpr uintptr_t UART_Base = 0xF0A03120;

    struct TXFIFO : cppreg::Register<UART_Base + 0x00, 8u> {
        using DATA = cppreg::Field<TXFIFO, 8u, 0 * 2, cppreg::write_only>;
    };

    struct STATUS : cppreg::Register<UART_Base + 0x01, 8u> {
        using Enable = cppreg::Field<STATUS, 1u, 0, cppreg::read_write>;
        using Reserved0 = cppreg::Field<STATUS, 2u, 1, cppreg::read_only>;
        using Sending = cppreg::Field<STATUS, 1u, 3, cppreg::read_write>;
        using Reserved1 = cppreg::Field<STATUS, 4u, 4, cppreg::read_only>;
    };
};

void Demo_CPPReg(void){
    // Make the pins be an output.
    GPIO::Direction::merge_write<GPIO::Direction::PIN1>(1)
        .with<GPIO::Direction::PIN3>(1).done();

    // Enable the UART.
    UART::STATUS::Enable::set();

    // Loop over forever.
    while(true){
        // Put a string into the FIFO.
        UART::TXFIFO::DATA::write<'H'>();
        UART::TXFIFO::DATA::write<'i'>();

        // Start sending out TX FIFO contents.
        UART::STATUS::merge_write<UART::STATUS::Enable>(1)
            .with<UART::STATUS::Sending>(1).done();

        // Wait till the UART is done.
        while(UART::STATUS::Sending::is_set()) {}
    }
}
```

## Assembly results

[This](https://godbolt.org/#z:OYLghAFBqd5TKALEBjA9gEwKYFFMCWALugE4A0BIEAViAIzkA2AhgHaioCkATAEK8%2B5AM7oArqVTYQAcgD0csGADUqAA5rS2YMqYEARqRakAngDouABgCCcgFR2r15XeUABAGYEm2ZX//qmtoAtCxMTGZITi7uLGJESGT%2BygByBBiswsoAwqxiAG7YemzKEGyoeYXFbsLYbJgmbNhmGAC2AJTRrm4YaiakBMBIRDnofQNDIzyW9JbB0/QAHMoAynUNTaOkambK1uHKE8NZWrWkhZgWNi5yTnc8AMwE5UxiOMpcD9mowkSEbERPrh7k8Xm9fJ9skQTGpsAB9IhGYjCIEg54VcEfL4eMTlIgEdBsMKomy8UEY96QvStZEk6xOBSqDRaYBwgAi2C8TWEkTRHnqnJyAAUhQAlXAAcTh2RF4qlbNwADEAJIpXArOEACRBOC5vhlYsl0tlRoVKrVGu1NiJrWwwjULCkTKCOi4AHYBNd/GJhM8dNZMJhTsIEVi2cpfpgQCAxM8iGpEQjPp7nN7fRxlAB1AiYBKhz7hyPR2MAxZJh4p5I%2Bv3KADyHg8tSI%2BYehb%2BxbjZcBFei/iI2FaalY/axUJhdRYtuUABU6clfqQxKgRtDYXDWixhABrD4e3vJPwYNi/bAAD00EaILHxqFUhN%2BM%2BU%2BTCYghrcvUZAbDEtoGqDh1K0l8s4PLg0YbqeECdD2Xp%2BO6bLJncboIaSjzrAQHhwaSNiMtYqBSMIwhCugeioOYUSoU8/K6sKhpStY2TZOqKxCrWAAyyrZAAmlqOqcs8%2BomvRjHMaxHHcbx1qTnaDpOoELK7pWfYDkO15vmOsI2r4ACy2nKrWCLkMoq4TlO05GdOygbtuRn1o22DNiM6ANk2c7%2BAuS4jKK2gECepDeSwmCKfuB5Hie56kJe17pHex4jPo6AkcovkIgM%2BQEGEYYhQe/gQNZO4FgWxnjuum5bpCIFgSAz6vNg7QfDwABsvCNaUzn2SMhXvpYYjQUpOX9oOw7qSZWnKAAqmGT7oDmbk5c8xS%2BL80W3pZWiBRA2U5bFD66fpCKuGFIytDS6Bwjg6VSOQW0DeOY1FiAE76D4cIYZCSgpYiBDpWERnjY4oHRiZU3fuECakDd21Qzl9VsOgZ5SAmwWwdDWhEBIJTLTecKoJu3bZJVUB2CdBLndgl0Qk1VllfVqKgco7VNn1kPwYhKPJINqkjpCo3SRNU35DNmBzQeC0CVFN6PutmCbez21Hcoe0GUQh33sdp1kxT11y7dml8w9T0vW9XyfWlGVMH9ANVcDRWg0w4OQ9DTvKLD8OnojnV7jrB5oxjEvpDjeMVUCEDExrF3pHVbOpjlrMwTHu4IfHHMqcNo681OSuGcVetmRZ1M2XWLkOaGjMOSLHnLso3nAL5/akJmAzc17CfJEdZ4XljMUKwlSWm995tZd7yR5WVWXviZpXbsHgPVS%2BdUNc1VMQGXnWtkVPXM97nNpzzd185NRWC7NoGQ2Lmxd7ex9BQA7k32Cy6321ZyrO3q6TEdXY7/iWTVr7a0/XWpklrtkekSZ68JjbZA%2BiGL6P0LYTStkDccIMxBg0RN/Z2ztXYI2wEjd0/VoZhw/uTSOU1L6B1%2BDPXAj8sEj2IWdT%2BlNWoAD98q0zdNkUoEA/7qUhAzYuRBaZU3YZgl20coZx0ISnIaal077ynIfd818RbJHPiAla00czKDvsQB%2BYiX6qzilZcOpCv7Dx/k%2BeeACsEZxAZ%2BQ2kCPCQn7vAy2QJkGwlQegiGFi6HQxwe7PBntpHbQYZrMhR954SO2lIkKcSUY7zkXvXOOk9LKyMnYmc%2Bd8q2UEaXQR%2BdeEV0RJ5auPk/KNz0dkNW7AQmQ3bhFf2t4e6JSYMlWBZtMqFT8aUfK49wyT3ytQ6MvDhFL1aivQRAzlCbxiQNVOySvhZKUeGFRp9vbqOaVo2%2B99aHO0MW/ExJCtZiKyQbcBRtnFfBgalAev1EEeJADbd8dsHa9P8TDZQcNcH4Jblg8JTDyFXmxrjKhwEQ5iPoSTRhZjmHKDYTTXcXCoC8NHPw1e4yC5bk6L0regDE7zIPEk7myyFG%2BFWTs1R/gtmXx2TovZBj0kHSOTCiJ5iCUc3JR%2BaMjjXrXOyK4827jZ4vPDG8jBHzPku2%2BW7D2yNOX%2BEBXCgW0Tk6xOQkShJCdK4jGlnCQkTATAKqhiSka3KX6ZO5eZR8uSi4dQKR1alfhaUgpimtbAG0FaHIVmyphgT5UELEb7UgJQa512wP5T1wsviWuydioyq8nnSzyqYim%2BLJGavVXBLNhDdWHGjXCXRI4QAFsCgatgRqTXbTNfI1JitmVECtfWm1lk7V2SbI61yGyCWus0dfBleiIA%2BrVsc2FpypXSoCKO3%2B88A3BOrU7cNlT76QjjTau1SbZ7Fofn6uFRkxlEuSNq01izSUaWAQ2/aTac6XtbQm%2B1nanKFMfMUntUM%2B2SwHTu4djajEPj3em2Vvz6m9OXfXKp/YalxTqWuxtFkjKbpfW%2B4EvTow/sA5HDNGqk6hL8LWlJl641ZPvcM9921P0xQHU2X917/3v3HVhyGPygl/LwwecDkbIPYGg8tAEcHr0IYfT1RDZVnUHnQ3szDUhsOx1zZDAjZL63EetTksT5H5qVvFnSgdFRPWkFo8rejY72VR29ixwN/znacYbvfXjV5%2BOxvg/Gu1ImEVkdQ4qvwkmh3Sajtmg8J6a1nvNcp5zJG1PTw06LLTF83VXyFsZdAwBgA%2BEMyy31aamPmblQuoNvTlUU2BStSh%2BNCahz87TAArLgbFsnAvyZRkFvw%2Bad0VqrflglimL1jRUy2yLW48kOufU66LajYsaK/Yln9I7jF%2BZsVOz5CtZ21XnWxsRNnuMCYyfGoZ6mCZPN4UZHqTyf1Qv8PN7haLIQYsEVi0Rk7FtPelfV49jWushbrUR8LqnbVlSG0%2BgRHUilqs8x%2Bib2zv17NmwBrLMngOsdA158ptcV3VNqY57I52/DrubfCMjB3Z5HdmWIIyqL57oq%2BEDpmi86vY8W%2BJ5IPn%2ByppOUxgLb3cPxPe6zSi6FMIfGwrYOQM4kREB5BRekjwMICkwgaOUcJpyimsMqaclo%2BJ6logrpXKu1eSWsFpe0jpfDyW0Iu4yn3ITZlzEgUMKwCAAC9sAlMXFXGz05xxEu60CS8ruvIVPrh72EkJFhiB9wQ5Q1YMxip5TGTs5Y%2BCEoC97%2Bm%2Bb3ee6%2BPQRqYf6YR6jzoGPD0SxECzwnpPhCU%2B1bTwHyNQfndfAeDwHPtW8/pgLygoqRe4yN7L9q3nUuHj86wvSHCIvayFFIB4Jg6Ab68kojLmi8ujS1gAGq4FFIqNitZMz67JLqcWS%2BpSr/X5v7f%2BvDeyRN8yM3nWL/G%2BSgCSNRImBZE6zIrmfDvbW7zCMTMC2gFjTp7B5fCZhPLAxHYKbcpAH17ZCgGiooKASAhNbRb5qoBIDYCoBbgGoT5T4z7m5Vht4FrCBoJrwoRdbcoPRxjaBGBMA4wY74y9wILXZfBFSIFgEZ6EJ96aqkjcED5D6C4j7C6KxlRz4D4L6CiH5wjaTWArAADSu%2Bjw%2B%2Bmwkh0hch5%2B0kRuck1%2BroVm%2BGluSmRGZU5YYO06cUHckU2kRhx0LAW4%2BOZUEACs3%2BtuIwN8OYCQa2SOUMIaJQrhNuMylgWIio2OJ2j2TsIA2OFCYK%2BMlh24xh%2ByU6o8thU85UsaVhIcThCIEAvhCQygwQyg9A7QtMLBVOBRyK%2BR2O9WzWFusi56WSMRWB3YJhh4as5hwhsR1hSRwgSAGE/YmAyRDho6GRLhbhSA/%2Bz20qCsHaJcI2NOFmeWuhPsDkfskRQcqR7RIcG4SRBO9RcR2RSARRWOVOmKWqPOvBZI/Bdwo%2BqwSAgUM%2BK%2B88ohZI4hcuQkcIKwmo1gbI2%2BK%2B1gbE40uAChDwShgkdEbxHxXxmYPxfxAJVoBuGhl%2BzoCkb%2BykNRoWl6NmRkjBE0tQKwNxmAM%2BLuZSuJtxN8%2BBph4Unc8WRyWJPo8IXRJJU0HgYQtQJxXOiS%2BhPWfMNmhJVcxJ%2BJN8kIGJxki49eLeCx84VJWSNmnivgb67GjSlJmirSSUtJcI9J/JU0pS9enB727%2Bu8BhgBNeEM0WUpRpMp1xJJgpRpmSIph2FOPSBK0pIApw5cyeHJppqO9cIsCpkUyp7SfJBJXwQpWpTyqp6pgZKY/e5x9QGEw%2BDIIu2kkawA2A3Gjx0u1EEhrx2k6%2BEouAmYooquMJGuB%2BWZOZeZBZ04RZUktomhV%2BLo5u3u28UB1pkB9aTpwMuSkMUxjk1ODkYxXKbZZpzyKCxSyBtWFQm4WQiZpAyZ3GCIQ05uagYgz06Q4R3s%2Beyg%2Bgm48IMeHpEapAMpRKmg30aka5BKFCk5kaRAEASgTp4ZN8xYtQapeJM%2B/ZU6vAPAv4yZg6I4vksqIwLAz43gLAECygHgSQ95VitUBanpkaH5r25J/YTSdKPc25CIKCcIjoqAP4aCakfRzBZBWC5O0FN2Rxd2tOoiHOiFrRKFo6W5T5wMdBrQ%2BgAkfR/SRUZG7G05s59885Q4UEwUup/gS5K5qAZ54Oi02y3FKZvFRAC5mxD8HhikBa6MoagluG5ekM0lc5cl/F3qSZMleifFTALURR4YOAPg/YR6/g2lslQ0LUDMsIRgJAxpbIAxxitlRlulJlTUZlygFlDk2pWlBlOl9lVMYwka14ZABYw6IVdlQ4LUplU0AVVlVFOOcVXlC5EVzl0VrYsVM5hl/YxlflKVQV3snlRV3l%2BVPFmVCVTUSVRUpV1lLqEOA6%2BJTQAlCsiVVMcx62vSe5fk0YMODGpmQ8yOyQTppAN864A4cIagM0j%2BBmCFOUm2dm9BN2j2A19cQ18G9OW1kaMpb5TsTFLFTQbF/29OIm9OmF%2BEOFw0%2BF0SoR0qp2UmcO/m7GVRepSyWO/V3KioR1B49FO5KCTQ01EBv1LaU0FVO59lLBj2GJ9OEAJ1rFyRZRio4ENMANUMV1j2SNWFd1eFcI12VMLC6NIA7CZRUAoNRN9pxRXCZNmKi89O4x/iZND241LNgWY2/g4m6irZl6FyIFVyG1yOEAVBoazJ0YaBGBWB6AOB0%2BApcNHNfgd5ju2ARk1NxOEAbNY8dMtWDNd2zN4xyadoJBoyc6WNP8RtdC7Bwe9V9V3srhCQnVo63VrUvVnhzs3hserQctD804GNthUEy1mlTWQlehqJ6KTZg5sFFA/Nhpsd5pwyY5IUE5hEisGVzchCIlpE4l20G5QN6FXiRU%2B1B5w5wB2daUp5DSLRyFVJqFDFGFbQp12A5124U0nFIUOdq5NdZhddmi0N1MQdDdwNXiYyCOlm7GyQ3t0NPCc6zVodBKs90NbtVkN8HhkMpaN12FrQuFvRNNtUeUs%2B29BN%2B949Qa4dB4s9%2BlBVW2vlyVRQgVC9K94VTlUVrl7lu0mdzu99jVj9qVXF39Dl2V79MVL9EyJV/9ZVvarViW7VilRyq9HtZJABXJQ5w1JmQKDp0qk101torQc1C19cUEC9E1RpW2St0qpdO1gm9OhdJkltOUyNZ1yRjDB4IRytTsL1vmb1RkJ9u991B9r4Idn1KJH%2BX2Y0QR3NLVklK9QZ1pygzDrdqN7o9NGNUWtWTtkuRFI9Rdspc6TNPVuWfVyOF5hEV5EAlBIYwg0kItnNNaf10YDoWgAIcILI%2B5bDi23JT1z2dptUnjNtPAPAXgRQQUf5cMIwHgpA6ArQxk6BEYfM7jfkD%2BVkBlRaq6QTIdyQZN81pEJgQMn2O6djny9D44ATfgpTsI5TygOtNk9OBtTqPjDOoEm0TU/De9yjxOh6aVixqlbA9OD0vthQ8RU6cj2QQpSNzdKN7FnCNT6jOKjOzsgdD87Tgjh6TTB4Ijl9x5z4/YedOUZjZw15t5Q595j5dJL5N81TscQTX5vgO6HS/5yggFLAwFoF4FkUkFaKSTXpmTC9PpGdt9ey9UW9%2BNAjhNvCEAm86l/ztdF419dFaF4BILijYLHTD1h9%2BQHCAgl9yQlTvgqzELaqOpbJ9IZxaEMZAulxQhNmaZVEsuWuRocoyoKwlZoogJwJjLUozLrL6%2B6hNZCJpuOhFeHJkMAYQYdosCKO4rwYANQxKOThANpddeVpwAThttvg3kTY9xJF3Up4lgANWJ40OJlzjJzJzuKdKM1esdKDkehBu5zZsdKr8jarIxGrC9G5L8AsJE0UPgt6sLfdipksCsMrkroYhdgUEr6dRUNcobhEAbFJnz9dgxIxoYvoTuU0Nc6rPTALtFxiwMLppB5S2rxLkMG5kFRUAZit4zCjxr2AVbdM7GdKhyU1M1BD814tAlyJTs3tWg4tQQzYUR22GWasIcEbgYwYWzpLUMebu0f6hwZ0%2BDhDnb2LwaSxalfbi1A7pWkI3q87R0Y7aFkbk7C9ojEd4jhGkjvNEOzbQDBpfMOTxgdQzYPzkaRktTKRTRB4dz6TQ6CsWSZN4BBjyD3bqM67/Tj2YzAHTjT7rjr7FAcz5N6mX7WCyzUAc9urhxajIAjNDlH7hRp7uLYj%2BpP1yOAHbDQH/jZy1qUN39xlxT/i0HIAzjz7bjLZj2H71TDTTY1To5nD9j4m17kll88d%2BsoCfKUCYi4tz%2BwgUt6BmB2BkauB1bAzatB61ipQH7WIwI9M3HDkuK/Hvjs8pwZtc8VHvSs4GzXD8BdtEykMP7P6SloHXh4HPtftEAAdSHQdBHPTZ7zSmF5jpA15WbIxygSgG8pO1tPNQTShNYLAKO%2B5t6AiOioXPUjzETzz4QM%2Brd8FrJ0cUZFLhAVLQujIioBAoTdLzxXLcIPLbLKouAbEbIHL/EyhrxtXJ%2ByoDXTXsJd%2BWh9ZoHjZ5B9afA25CNX%2BqbIwZXoTirXZ%2BSk35XTAmA3ZSr3KeEBEREJE6QJg8SKBpSVcU3i3trG5LHcHRpU0I3tQ3JPTG5Dr9aJ3L7Q5JkHrhBXrJd3K93bHidIAL8z3NYuTW3U0a3krxEeTCbSFQb3cKbNuoYexU0B3mA2b8pcLSbSpo63ZXaDkcPC3S3giYPNFyb%2BbKC7F74Clz5PRyjBOT3LTex1z9jdPvZQieP/dwbdFbSygNx1jpr2DzsH38H0YZzMYT595C93WlHr44mqB8nst8teBzn8syP2yfpRBJBY10q0nktaAUvink%2BCtDHU6vPpzanUX4Nhnfgo87detDPBn9PnyJtxBTARA5ttUhH07FGN7kpKCKaTn4p203t/3ZEBTkd0sw7N6DDD6W6NC9OBvX30Tbby7i1JDVnNvT2U7Iv7pNHb3wBKHNK7v/aQskMP6/7FB4nlyTi70YAHPz5JJFkGrUFwjzGxjntTs/v%2BTzyhTq6Tmgmt6omhcEfIzWC0f%2B50YrbS7HbCf7QvHj1pvyfnyqfPTovGfE8Ge2fMj2mVJKijteyRf9agtEC/KkIlf95NfNn%2Bjq2DfIGtrZDsdFDF6eOPfg2DPiz3mIAZ2TUg/g1IAAvXT89PTyQLfgf4jIpp3x2xh89sUWC9H43/gk5IUj2d/ttWdJ4NZqY/YhhPyj6wcHuX3L/lPxn44Cp0c/djAvzHoacSM3rE%2BCvwfzCd4sonKcLv2FqUNnYSgQ/pczERu0xE0tBTn7R16BlsgKGYfqbQd5sNLO0/Z2O6035DpveU9YSptwD5t9I6QA7IH1nVrh9kMoOfvjz3QGfch%2BCAuPsgMjSJ9hBuAvAS7zT6R1CMdfJQSQKPj59pGFAtfitGoH2JeUpfffvQKdhMDLSvSVgb0nYHS8lOuvL4LwOdL8Cm0YiIQbgNEEEpHOE9eYpIL8A/pumH1Ijqvzix59tENGCQWIn/6yCP8YibtAoJ%2BxVNsUIcOAQdW0Gj8iGegnzokNd6aZKBqQoKHpmMACUQOPvHKFkLNRsCfAxgEPnfyKEtMShZdEfkgIqFLV8BZ8XPlNm0QkAUsaWDIb0naEhYzkyWVLDAUUH39ihGgvnmUOGErsxhmyCYTFCxIpR0h0Qkxlgm9ra1sefAjaGZSKjm8Cop8emJij2EwM6hksI4SGEaFLVThTfMDn0005XCghNw/wr1GME9NDmFjAYfzzVracioNPKLnBCCYhNDusPP8qwAKqRQEg7AZQPdxgqJd02P9HgCHQhFBcIAUIz/jCLpjhhYeggBnrT2PRBNV4DURPKiKyDojkymIm4iUFxHwcIwatXLuCPiwBcjmlw6bqF3C7dRIuVnD8rFwzDxd4eSXZyClz8Jpdwm6AACllxvg5c/mAWLgmQWjJFc4yNgfNBKCFD6Rzcs7cHpFGLzgxQwpo/aBdzfDhhLAp4RUDYEsAPB6AswfLlaz24jA2QBALQMuAJAlBS0QraMFd2yD2iDIjo5kbMn1YGt8i2ecPK0LtY1gzRKQAIkVHDEgB4ekIAMUGPxCEgjITeY7DEB4BGQcx%2BqIATp3YwbkMx9AKaDmLzFfACxGBIsWwBLGk58i5YysdoWuF9Eaxv3DMBmJ4BNj%2BxuY7HvmMDHtiQxXYksb2MRLaABxv7VKrWLLaEEMxDwccS6GjAtjsgbY4McWOUCljlA241wBWKXGgAgRg4jvuuPXKbjVQAAFh3Esg9xU41sTOKPGdiTx3Yl8ReL7G7ibxq40UsOJ0AZiqsr45cZONCbTjCxc438UZEgkASrxK4ocdd0fEpBWo2YicfuMPEdj5xygVqChKrGFp0JdYzCW6CgnXi8JX4giYhOUBUSSJE46sXeLy7xwQo%2BaacMsL9ZhiJxkY6MXCFjG0iXRlgS8aHhTEUT0xqoLMe%2BGbEfiDxdEhCfQG7GJjSJ5aQ1Nt0eFSSRxqoRsThKAm0T4Jx4lSUZEYCoTgJmkxthuOkkpAxxBkt8TBMW5wTZxJk7sZePUl9ErJ2kmybpJSDbiHJ0Eoya5J/GmSzxgExyfqm8n3iCU9Y58dRPfGwTPxxk0Kd2IeARToJUUytFpJilQw4pKQSCYFJokKT8JyktKRlOvFZSjU1kh8bZOwlyTcJJUpSW5KMjpSLJVUnKWBOUAZiqJRUxKc5OSkhSzJ5U9qYWminsTIyPOY0X6ImjWBRQlkUDpaNaI2jEwIwcaHNOnBCTtyU0F0W6OsAej6A0wH0TqhmnTgAAGiqEVC1hlAfEoCZGPWnzStptQOMaJMTESTc8qYjcmyGsDThnAfUpyTGgJgXTlQV0oyKHjLHMSgJbWcabqKmknS/cqwX6dOHGgrAbpFk%2B6RtKemUxE8ok8ye9LFI6SdAuAUvglIBmQhWWP0lGcNOOwVS0JbEjCTWC1aRoLgsk8MPJKSnZAKZyMlYIRPMmeT2snUhmRmDWAxkMw/0/cVzKpn5ERp/M8ib5J0BMzzgrdfSQ1MMkKTJZPM5QE%2BL/G0zgJcssOqSz7w2AB0HIX2saDFDaAeEQsToAsUZCWFbCcTXwGoGeBZB9AvgbEeIHjDxArgCcaMdGFKmEhwIaTeQX7JAABy2A0YBsSHAKJmAtGkIUOeHMjmqgHg0c9oGYHgYkMOJKMRkMTKFq%2BAEgFKDaT7OSAPTPOGs6MLnIgT88HImcyMtnJFxsREoagBmBPjApkByYkaYuf4BvjdE0sWpG2exkZBCh4gzzX3DWDjDoBHZNTEGbWC7k5RS50Yc6ZdNrD%2Byfp1gZnDATAAyBNQW8kOCHQXkgAl5M81eb9I3nvQZABAXeS03xSQxGQKwK8EFwjDoQMwnsmcGdOnlXTYo/YAEDyFvki5pYDMbKWBWx5ZAn2qgLoVoEwBGRRAo8vgJxAfy6oQxf8x5pG1bpGR0C5QfOd0SyD4k7QAIamEQDQJtRNgLsv0SGJQWBhW6acyGAfPLnk1g5HfbILQqRkoyK5pfVObHOIBRAvgzCymSsGjAiz/gwADhRnJvnexGQmYV5iuG8DtIC5s0%2BaY83gZzyDwPc7wA/F4XcyBFz868ccJrmFEYWBspCGQRkAT8mAsgKrDIHIARyZAlgSxegFkBY5%2BAtI0QBICdBkh6Alix3jYpMUT8twIAKrJYDMBPi3QiwN0G6EagLBPRAATiqyxLmAsgJ8ZYtaD%2BKDW1i2xeQHsUyBLFsnA1l4tsUT84AsAFABgBwD4BiAZASgNQDoAVjWAHANAIIEEAiBxAkgaQDICeJghKQXwSMHGF5A9p4yqwP0VkBIDUxm5hIYZfgzIDGoPmygT1EQqcoEA1A6BGgj7L3wtcCWcIc0YLGHBqKnAJkGiPmlA5wgNl104vKXn9HKhxQBMfSCkGTDKBGQwk/gPGMsCWAQoRy80cXi7AzhawEoCUGxFrHJB7l20kSfqx4BGLlAgkuvByGuQpg9lgoA5QsTeUnL48IwI%2BVdNuV3KRcDynGQmNeXHLI8yKxGXwvRWArnpwKmYGCtLmK5xwUK46XbIHCTLnmE7SVm3M5H3MllTs7AAsqWXMk%2BRRAFZYoTWXdT18yoIUJqCEkyFasUAYvD3iES7T3Rno2YAZ1WWa5ox1gcVSsElVChhVoq9VS3keWvTnlSqgVSqrNHXSoAEK6lYKDsD1RVVuqo1UCUFWUrYxEALVQWR1V8AJVL0hMYquLKbBS53CSlZCqtX1QnV25JVcbMSymyzoCglYCyxIa2yEyNhXwJpO6l6Tnm9QVNf5KSwMx4gS5YYjkXyh%2BhlFCsU5Y1FDBsgLluAK5bWBSBwgMxUhGQrISmhQt9AXosQJThRSNiLx%2Bi1RqUEsCtqVJHa0oOeJPFVDso0Y4IECArWXLpw1y5tROqnWVrq1KQWnCwmnVVrZ1NautaqAbVyEOEna9taRU7Xlie1szCAIOqPXDqT1YihODnJJlyLS5xa0dB8tDCUqNZcIXACkGsB8A/lO0/QM8ueVbyANKkolKXMnWgQNZU0MDUCEg29q31LCjUJ%2Bu/V/LjpAKhuU3JbmRoWVHc0gMotUV9yRSA85BcPIApjyMwE8qecvOUUlyNp4G3AKiuulFQt5O8tpT02g2gQGNU0LeZfNY11yCUd8h%2BSMFqCiydAr886R/OulHhv54uajYhXxWlhX1mM99RqpSAVqUgEoKaBerppniF67G9UAhqg20aYNBmuDUpoQ0fqv1P6sUlwng18K3in6tTRKFQ3bQJFUi4yDIqnn%2Bq/ySigvr3N8BQA9NsGqmLZu5n2bVNqoCUPVAlHOjeoBihOFGV4ImL4lMgCxVYtkDpLMljigQI8pcUtKGojeTxelp8XkB0CgUA6lBGS2JLyAySgJWlu8UZLZA2SkALkqK2FFyAfip8U%2BLMAPBLAboSwFVkWA8B6AboJ8fQAeBugalsgB4JYrSV2LGt5APJUlsKUIB4AEAYpTE2dk%2BAKAVAAYoODUUHljArQN0LMHIBeAHekaWThAH0BFbyAp1YwPkxkAeLyAbQW0ACFrDZSbtOADcBwB8CfalJhQWTvVrPAYF4grSp7VQTMX1avoyS7xaYvYCcAGl/AZgAYFk6QAJ%2BYwDsYDuCC1gsgwQVoAkB/D6BslzSqQAwFMXmKZtN2zJXNO0jKBgA%2BERiWYB4BmBGxZQQkAvAgBlKXK%2BW8yTUj21baed9ULLYIEK2w6J%2BpWnAAeQq0daolZgKrENqCbjaoliwRqIsEsBPiqslWpJSkrq0Zb5tOShba1vIDLa1taADbftsqW7bNtB1Q7cdoNZnb64l267fVru2mBZAT2l7c%2B3e1GpPtA4eHb9vq2EB4JAOm7cDuwp7NHtliiHTduh1G7alCOpxUjr0D6BUdFWjHSGKx047ci%2BOpAITuJ2uLpARI5Laltm0NaZANOunQzrdBM6WdpQOGE0Hqic7CA3OskJeL53W7IoreoXYjr4Ci78l4u6NOVvJ0yAqtNW1JVTv13NbDdYu9rSAB4DV7GoT4sSc8rdAPBGoVWJ8TwEG1uhkt023XXNqyXT78lxuxAKbqK4eBLdTAJALJ0YCkBr9c%2B4fSXon0yAAxDYPYKKFp307bw1e5nazvr0Lx8gWQCvV/sZ2/6697O9oH3uK1%2BKt9ZgfrfPtG3q7Go6uh4MEsm0j7Kd9WzJU1pa0z7IdPATA3rsP2La2tE%2BX0IHKfFAA%3D%3D%3D) is how GodBolt compares the CMSIS and CPPReg versions. Looking at the assembly, it's pretty darn close!

![Assembly Comparison](Assembly_Comparison.PNG)

## Full working example

Lets say we are using a Cortex M0 based microcontroller like the [CY8C4013SXI-400](https://www.digikey.com/product-detail/en/cypress-semiconductor-corp/CY8C4013SXI-400/CY8C4013SXI-400-ND/4842995), the smallest ARM based microcontroller avaliable on Digikey at $1.32 in single unity quantities. It has a meager 8 kB of flash, 2kB of RAM, and runs at 16 Mhz.