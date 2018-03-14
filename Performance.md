# Performance

Cppreg makes use of C++'s "zero overhead" [capabilities](https://www.youtube.com/watch?v=zBkNBP00wJE) in order to optimize away **all** the code that CppReg utilizes to enforce type safety, specifically the very template heavy code. Understandably most would question the validity of such a claim, so this is written as an attempt to prove that a register interface written with Cppreg is just as fast as the corresponding CMSIS based code written in C.

## Test Setup

For a test example, let's use an imaginary Cortex M0 based microcontroller with the intention of having the UART send out a small string and then toggle two LEDs (PIN1 and PIN3) after every full string transmission. Why imaginary? Because a real implimentation will be longer than most screens, but a real implimentation is provided at the end. An example will be written in C using CMSIS style, then in C++ using CPPReg, and a comparison of the assembly output of both using GCC-ARM with links to GodBolt so the examples can be fiddled with.

### Example peripheral

```
# Imaginary super simple GPIO Peripheral
(GPIO_Base) GPIO Peripheral Base Address: 0xF0A03110
    # 8 bits wide (2 bit per pin)
    # 00 = Input, 01 = Output, 10 = Reserved, 11 = Reserved
    # Our LEDS are on PIN1 and PIN3
    GPIO Direction Register: GPIO_Base + 0x00

    # 8 bits wide (1 bit per pin)
    # 0 = Do not toggle, 1 = Toggle
    GPIO Toggle Register: GPIO_Base + 0x01

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
    __IO uint8_t DIRECTION; // Base + 0x00
    __IO uint8_t TOGGLE;    // Base + 0x01
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
    const uint8_t UART_STATUS_ENABLE = 0x01u;
    UART->STATUS = UART->STATUS | UART_STATUS_ENABLE;

    // Loop over forever.
    while(true){
        // Put a string into the FIFO.
        UART->TXFIFO = 'H';
        UART->TXFIFO = 'i';

        // Start sending out TX FIFO contents.
        const uint8_t UART_STATUS_SENDING = 1u << 3;
        UART->STATUS = UART->STATUS | UART_STATUS_ENABLE | UART_STATUS_SENDING;

        // Wait till the UART is done.
        while ((UART->STATUS & UART_STATUS_SENDING) != 0u) {}

        // Toggle the GPIO.
        GPIO->TOGGLE = GPIO->TOGGLE | (1u << 0) | (1u << 3);
    }
}
```

## CPPReg style

This is written to mimic the CMSIS example as close as possible.

```c++
#include "cppreg-all.h"

struct GPIO {
    struct GPIO_Cluster : cppreg::RegisterPack<0xF0A03110, 4u> {};

    struct Direction : cppreg::PackedRegister<GPIO_Cluster, 8u, 0> {
        using PIN0   = cppreg::Field<Direction, 2u, 0u, cppreg::read_write>;
        using PIN1   = cppreg::Field<Direction, 2u, 2u, cppreg::read_write>;
        using PIN2   = cppreg::Field<Direction, 2u, 4u, cppreg::read_write>;
        using PIN3   = cppreg::Field<Direction, 2u, 6u, cppreg::read_write>;
    };

    struct Toggle : cppreg::PackedRegister<GPIO_Cluster, 8u, 1> {
        using PIN0 = cppreg::Field<Toggle, 1u, 0u, cppreg::write_only>;
        using PIN1 = cppreg::Field<Toggle, 1u, 1u, cppreg::write_only>;
        using PIN2 = cppreg::Field<Toggle, 1u, 2u, cppreg::write_only>;
        using PIN3 = cppreg::Field<Toggle, 1u, 3u, cppreg::write_only>;
        using Reserved = cppreg::Field<Toggle, 3u, 4u, cppreg::read_only>;
    };
};

struct UART {
    struct UART_Cluster : cppreg::RegisterPack<0xF0A03120, 2u> {};

    struct TXFIFO : cppreg::PackedRegister<UART_Cluster, 8u, 0> {
        using DATA = cppreg::Field<TXFIFO, 8u, 0 * 2, cppreg::write_only>;
    };

    struct STATUS : cppreg::PackedRegister<UART_Cluster, 8u, 1u> {
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
        UART::STATUS::merge_write<UART::STATUS::Enable, 1>()
            .with<UART::STATUS::Sending, 1>().done();

        // Wait till the UART is done.
        while(UART::STATUS::Sending::is_set()) {}

        // Toggle the GPIO.
        GPIO::Toggle::merge_write<GPIO::Toggle::PIN0, 1>()
            .with<GPIO::Toggle::PIN3, 1>().done();
    }
}
```

## Assembly results

[This](https://godbolt.org/#z:OYLghAFBqd5TKALEBjA9gEwKYFFMCWALugE4A0BIEAViAIzkA2AhgHaioCkATAEK8%2B5AM7oArqVTYQAcgD0csGADUqAA5rS2YMqYEARqRakAngDouABgCCcgFR2r15XeUABAGYEm2ZX//qmtoAtCxMTGZITi7uLGJESGT%2BygByBBiswsoAwqxiAG7YemzKEGyoeYXFbsLYbJgmbNhmGAC2AJTRrm4YaiakBMBIRDnofQNDIzyW9JbB0/QAHMoAynUNTaOkambK1uHKE8NZWrWkhZgWNi5yTnc8AMwE5UxiOMpcD9mowkSEbERPrh7k8Xm9fJ9skQTGpsAB9IhGYjCIEg54VcEfL4eMTlIgEdBsMKomy8UEY96QvStZEk6xk9GvSlfIhiNQ%2BOlOBSqDRaYBwgAi2C8TWEkTRHnqwpyAAUZQAlXAAcTh2TlipVAtwADEAJIpXArOEACRBOBFvjVCuVqvVNq1eoNRtNNiJrWwwjULCkPKCOi4AHYBNd/GJhM8dNZMJhTsIEViBcpfpgQCAxM8iGpEQjPsHnKHwxxlAB1AiYBLxz6J5Op9MAxY5h555JhiPKADyHg8tSIlYe1b%2BtYzDcBTei/iI2Fa7JYk6xUJhdRY7uUABU6clfqQxKgRtDYXDWixhABrD5B8fJPwYNi/bAAD00SaIs/SqkJvzXynyYTEEP7z4piAbBiO6AyoHC1K0l864PLgqZHveECdGOIZ%2BIGAq5ncAaYaSjzrAQHjoaSNjctYqBSMIwgyugeioOYUR4U8krmrK1oqtY2TZIaKwyu2AAyurZAAmiaZrCs8lp2hxXE8XxgkiWJrrLh6Xo%2BoEfLns2E5TjOc6QvuS4rgAssZurtgi5DKIZbq%2BKuVmrsoR6nlZnbdtgvYjOgXY9hu/hbjuIzytoBB3qQwUsJgWmXleN53o%2BpDPq%2BqDvreIz6OgtHKKFCIDPkBBhAmMVXv4EDOWeVZVtZi6HseJ6QrB8EgD%2BrzYO0Hw8AAbLwnWlN57kjJVAGWGIKHaSVk7Tqw%2BksoutnKAAqgm37oGWfklc8xS%2BL8yVflokUQMVJWpZ%2BpnmTmXVOTS6Bwjg%2BVSOQR0TXNKmAamS76D4cKEZCSg5YiBD5WEVkLY4cGpoZy0geEWakE9x0IyV7VsOgD5SFm0VoYjWisqQJQ7fiEGoMeo7ZI1UCtNdt3YPdEKXeV7WonByj9T2Y3wxhWFY8kk16f%2BC6wvNS1Vfkq2YOtV6bZJSWE3t2AHfDyRxSMZ0WYC9NU3d6TYI93PHTZr01iAH1fT9Xz/XlBVMCDYNNZDVXQ0wsOK4jrvI6j97o4NF56yVOMSPjL6E3CxO/A1QJlZrNPa%2BzvvnphqH5uhOFc0n1m6dN/MGyZZlq1Z2d2Q5Tl1a5PkefGrMeRLAW7sowXAKFk6kMWAz6T7adKx%2Bk4JTLb7K8oGVZRbgNW0VcfJGVdVFQBhm1ae4fg81v5tR13WXRAleDf2VUjbHHc6VNs5Zy9K7CwBotrXB8NS5sBNvhfUUAO6t9gh3j/4qsXb1lMEtTtO6/vEqjkWp/gAa7PwBc3rGyJJ9eEZtsh/TjADIG1tFq2whouKGYgYaIhduA8B7s0bYAxoGcaiMf43S1j6Kqd8iYkwXrgN%2BgDEaR1/lQumvUAB%2BDNzzZFKBAEB/NIQszLkQRm9M6qdHfn4PeCNOaJwRrzTO85IFn0TA/CWyQb7bSDvfMWyhn7EFfng5Qn91bfyjv/ExwDl5gPAZAo2Js4EeEhMPFBNsgQYNhFgnBcNpH4MRoQz2xDvZkIRhQv%2B2tlqCNTnIlOCj/DyLIUoo%2BKiT6%2BDMfndJa4i7lVLgNCuoii4xKvljGuQUQphRbkY7IXd2ChPhsrB8T5aEnXSplJg2UkGW0KpVfxk9TzT0TLPcqDDUyCPEWvXqG9RFDOULvWJ%2BsM6pIMtktRK1L7Ajjto3uKUH4GJfkw/BZiepXTYdHB6JiHGDmgSwWB30XFfEQblEewM0GeJAPbACjtnb%2BICcdIJXtMbMOSBE9hy1aEh3oTBCOJiJ5gouRw5Q3DJG8P4YI%2BcwjN6TOLqeKRIL/CyOOkk%2BGKSZoCyMr4dZGjSnMJ2a0/ZhjJxHPAScjW5yrH%2BOuUBJxDzXHdNeag0GHyvmJh%2Bbgv5/yZHKBRkQkh7d8EItptE5eiySokqxhqtO5TDjy0wHCQkTATDAsUcs8lkDMnVUFq9eyX48kdlEYUgamj/D0t0SlRy%2B1MAQH7myixHKY4yo9kC0hJj/Z4zrpUpuEVxZfEtba%2B1m8PletYZQxFRL1XxLIVqzciJAq6sinCJlvgQAFv1Ya41oa45kuPtanO50iBZLrYXO1JcHUFK8qIl1fg3W7UZYcv1Zy02coJVKhG/cbGtUBSEk14D66N2wM3F%2BkJ41WUTV2xexbU2RIet%2BZeGarw5qWYfc12TLWQNtY5e1bkexOp7MU1VtKEa9tlv2oxEBB1KsDbK4J8qwnHXnVUl%2BtS0r1JXbnSyOTcUnispvKyJStmjuUKmLdX6pAHuSEeiaZra2UtMRBxtVq8OXug927KbAtq7I2VFHsH6COnLQ21eGP6Q0KrnVGxd1TJwgZ2gCcDDaHJrrbSNIT88n3gJQ4cxjGHElZtJThtJzb8MCaI/NEjozxMbQo9LBl%2BiKjy1IHRhtDHLEx2Y8GmdVakOAablx7APGXx8bjQRwT0GrIieRRpxD%2BDJPvuk2qw9cnq0KdWUp892T1N1TIy%2BvRZZrLoGAMAHwRm1YmYDeh8zcqGn%2BMYxC91UKw4wrghTUz6GPgAFZcDQZk8nBO2agvarzbXYtBqKOVrY9hk9uH5rhaU5Fly7bb2dudZpyW2nb7uuowc99n7Ss6zhWOzuaUvwTKDVl2drsbOceXc5lTtqRlRaK01QR7mxAfK3QtvwjGrJQAxZCLFoicUM0u4t17r2avxwCzzELs0wsudUza3Jbab3l2G/elbj7vPHRi3s/RW7ZvpZXixyzHXEZbaXTUupTnsgvdXURue9UjvjNsfMsQN2BHL0xV8ERA0nuSJe4tsjyRfPMv8wkzNdWYpas5kxAiREPgkVsHINcSIiBikYvSR4hEpREStBqOEq55TWF1KuZ04kLRsXl4r5XqulLWFsp6b0vgNLaA2%2BnLr85SzliQPGFYBAABe2Bq5NYqQ3MKq5FwBZrVTqrOr0ce9hJCRYZ3makOUK2IsoqoF1iICOXMn32fm75j75825a7%2B8918egnUQ9VbDxHnQUejYx%2Bz42PgCfkk/Zx8zP3HHSAB6d18B4PBc9aXD4WQvmCaE3Jj83svFfsK4Ulw8PnxF6SkWF%2B2QopAPBMHQI/cUTFpesTlzadsAA1XA8ptT8XbMWPXZJzTS1XyqDfW%2Bd9771wbtSxveSm6s9fo35Gm5EiYFkKz32Lf3bjlbisIxix2LPRKYZ6B5fDFgiqYInakrZIgGN7ZDgGLyQxQSAiaqjY6qoBIDYCoAngGrT6z7z5m4tgd66rCDYJbxD6KLZJGwZjaBGBMAhxY6kyDyoJ3ZfBVTIEQGgH1ac6kgpy8FD5kij4C7j5C6mJ1SL7D7L7Sgn5wjGTWArAADSB%2BjwR%2BmwMhchihV%2BKkhu6kd%2B/oqO3uoWeGxkdUjYUO14XczSiUJhp48YR4J48I5UPqXcJYZYf%2BBibhSA06f68M4aJQj8nhcylgWI2ol2I0DOiMIAl2kKocpMNhOBo4jCERpQ9hjhh22Q8RZhEAv%2BNuRAEAAR1uygwQyg9A7QjMbB1OpRaK9Al2B6WGSeyiRhPWphiRMUTSPcmRIwqRcIwgSAhEk4%2BqTh/cOR8YBRCQgBb2i2/cIOnkNObMa2v62WzCfhuyBWcRLREc3RGmGRGxxWYxXhVOfC2KAW3OfBw%2BQhdwE%2BqwSAkU8%2B6%2By8EhDILE0h0kcIKwxo1gAoe%2B6%2B1g/EC0uAyhDwqhUk7EbxHxXxxYPxfxAJLo%2Bu2hN%2BvomkH%2BB8yeTRr06OVkzBi0tQKwNxmA8%2BQIqe%2BauJtxj8yGUCHgYQtQCIXeF4DWn%2BqJv2eG6OzuaeIwJJ%2BJj8kIGJ1k24jeVWpaRsea8I9sqOuau0kC6OXi20eJ8%2BcIJS3BXuVekpdeEsKpbuTc0p1xpJ3Jde%2BcfJHyvRpJ8plOfSzCUpIApwVcicPO5x9QhEY%2BXIwuxki6wA2AdmjxUuzxsurxxkW%2BSouAxY8oKuMJ6ux%2Bvp/pgZwZq4oZyk7oOht%2BfoZuhhbB1aMBep0BwBdeWpeS8MMxd6HkExPM6ZGpi6WpMSqBVWFQx4WQLppAbpdmCIU0ZuagYgn06QURccBeA8x4IpXeM8JZC6pA0pAWmggMR8nZzCkKNZi6eRSgFpRpnJxOrURZjOPAPAYEbp02c4oUQaIwLAP43gdyPgygHgSQi5BBGKfIQ5vAPAH27RLSk2/c%2BgvZNJ3icI3oqAoE2CR8%2BqrBFB4Ct2ppFRRxj2q81WX2AQlhPcrSz5r5kMDBrQ%2BgkkgxU8VUGm/6fgdZDZL8TZ7IyE0U9J/grZ7ZqAk5z642Oiu02F7puFRAzZqRBFyOf6uquMJQoaWaA%2BccNFjZ9F%2BFvqrptFRieFTAPU5RiYOAPgk4kFWFglvFU0pyYwi6s4ZAVYzhy2PFdFClXU4lygklHkjemFpiclWl7IilsIRgJAcM/YH6JlwlfFolXUYly0%2Bl0lieyQml9lzZSlllqlNlnlk4Ilulrlhl8MAVIpU0tl9ZQlgVDlPUzlVUIVMl5GlG%2By%2BJTQBF/c8Vl0zFSx%2BC6pQ5qYCOw6USZpY6FppAj8h4U4cIagq0AIi6yEyVyQ6OdmDmYGqZSGxZWZpZw5IAmSL2BVYU0pq5rsiFyFTQqFLkL2ImL2H5FE35mcf5qqkqb252Umc2H29RDJjRnV%2BV2S2oo1V4L51JkMTQVVUBXKEWy04VIl3%2BXV/gGJL2EA41KFBOaK2oCEkiR1CMs1q1pQ81X5rQP5AxJprU4FnCn1IAPCgYfCUA51YNf4hxygUN2Kq8yRkx%2BCUNz2/1mNiSo2mGBNrqlFmZeGjiMCpsjyOO/SNBeMVJqYGBWBOB6AeBc%2BXJe1UqC5DuOsMq2AF1JOEA2NaFV8zMqNj2GNr2yaHoZBy5f47QP1/g64uN/ynBTuTlTlccARCQmVLh2VvUuVhBx0KxRsrQLNr8q4X1DhyEW1RFtWSpX%2BHNQBzJGZaZPVhVnymCoylZbRmQtZdlbcZCJFdE5Fx03ZJ1fZ3iVUQ1mpHtXBMUY5P4k4IdJUD5iUsFLh4db5vgr1k1716Fh2gdbZwdjS0Fj51F/tvgjFcFp1kB%2B6CxrGRlyQKx4VFOU6yV21H8FdUVOFRietTkj83hKBzCpagNi1v5iNr8rQC%2Bo9wNS1E9jMdJPBzCLdAl0VdmCVAESV7lnda9y6l0PlKl1lAo6lp0FdG9ElRQBlyV4V5lylVlalN96tnUwVl9blRlOyaVhIr87UWVT99dKOjdE4g5w1/V9G7KJV1C28/1FVVV7orQtV9VTcTV29JUrVO21ND1ECwDMdA1/1mdhkCtJUOd2AU1MGM1Z2ytxKm6G1iOVkM9INJD897dttV4KZFK80oRRNPalFxlu9vdXwPJxDpDH1X1YmVWWtEu%2BCJ96U8FtdU66NOVFmPh/i05VEs5EA1BcYwgKk91eNlBSmUNXoWgAIcI15YUhDb2LJlDa1i8l1mD4Ct5XgRQUUu5KMIwHgpA6ArQ1kmBSYr0ZjTc5GTkglRae9d5zV/ghjtE6QJgEMCmxaujUq%2BDi4FjyQyTsIqTkTojZD/1Ytzq1jjOxW9Dc9J2e6bdKDV4fhL2JtZtLKr2N9AjepANbQE1jD5UIj0N9OXDASltr8xT49EyEtyQNtS9yQCdE58MqjZwc5YAC5spj8st82BTHUG5gl25vgrj6A%2B5h5rAsCp5558zZTSNATi6t595pd1hXd7UI9n5Y9oNgiEAu8hFozx0qdvDPdzK1dEdvg%2BQ1zyg/T9zy8AiC9AgLDaTsj75tzs9Azj6ipNpZxgh9p/Olxoh6OnpzEMumuNoGouoKwMZ8ogJwJWLKoOLeLW%2BWh8ZCJJu%2BhleDtGDCMUYMYHoSCkawAjLsYR1IxruORR10di6DeupwAORqtkaPY9x4NO894lgR1WJC0OJhzVUlJb%2BTu3tZSLurLQ5htfg3ZUefL9eme2Q9cwriBmeRl3ZZi0StEr4J5hkyVbz6dy27LzL8YmdkUTLVEy09cTrVEdrFzVGwxnh8Y4Yjunr2gxrRl9rT5LhkMVp5BorHk4rf4yV3ZF5ZJVUHJBJjTvVIM8rOpItRlrSg6lV1V8DdVtNBFyJrsKxdgEAWgtNQQvYsR/GasrgysEcrr0YsYZRzDLzJUDrp0YDvUnjJbCD5bILYaHkAcLgtb2A9bOMaxkIvqBGrbXc7br5brXbIzyVbDkCnD5hY2lGhbZ9TJHDqYRjdQvYJzFAKN2TZGm58IW6/cu75ZddBtlb2Mk7eMuOx77Dr0hjxgF7pjTTQtYjL2vTUArdSN921OeT8xpyIHJ43bFTHdECVeVyB1FjkMdj%2B1fWN1Fdd1jtASz7IA57JjV7mTfgCHFHN7IAcGL2FZ9jejWI%2B7hNLHxNh77qpN805Nx5ziiTrstNr%2BwgDNmB2BuBi6%2BB7N9LUqwbPNpTgt2TzHhJsHHk%2BKTHASUtpBTARAizFjStjHKtJrgeT98M97oT76g9WrlTn7gcQEpthQEAFtnTVtSHRlKHqxM5pAeRRrgRSgO8ZOQzt5qhbYLAGrYURGIiHhhRI0XSe5ygYQbNJDZzJxLDbD8M3rLLMo3oJ4fAvZR1xeGYfeIwWX2BfAJgk4dujuXONe6r6OJXFUYpBYbYcDH5pARgJgfYA4QExg7XkIos003gOiQEMeI4Vk9XZXFX3NTMEbfr/bIwGX8YakOB4dy043vZvraUVh/rLhBXAIRXSY3N30bAcI%2Bg5XHoq32XE32AlXoVccrSLXPXLAHX5i/zD3bXT3KXvbDRKyhHyQC3xXl3eXkzPehXPA8Y43Z3N31X%2B7er9XkIa3tQY3l3kPU3i8b37XX88MdX2X8PgPiPygEPk3VXi8I7j3laUDzCNbE7c75cTbv37GvVcPXwCPPNhP13qPTU6PT3CIK7aUL2Ec2P2BuPpXeXBPyPRP/JZ72XJ3vZanAK9tjJ0nTt80gvOTzCXLrLPLwPw3oP8YMxuobAcPqrjWbJYv2BJD6OyGMULJWbQ59XUvYnK3ggg2HkBv9XMQiwVkPn1uhJ77RBbYmdSJAENvSvaOde9vJH0vTv/ALvRAbv2XHvXvYbnhQzDj%2BbwPEp2Sgf2gRVg7hwsDNVZbDVhm47/iKxqvqYpP73JgXA5WfA%2BvhvOP5WX3mGYL/kUby2kC2foAoDxml0w7LXRfSDpfSG5f4f2XlfXPNfdfDfcPzfPbyVUz6jEAs/CfzvEA3vCQygwuweZRVOAFkxFfIAsnR3J3Z3wg1H6q65S3JDuqvVsXZtM%2BbNbYCQvgS3yXie9RO76SZgv/rLKIaBdVvVwN44B7wbYP3iiV2rZAjYJ/EYCkDIw6t%2BywyKgjclZDsh4Ql9d0NjhSBWQhSbIDkFmzFC/8mYdsU1vJjpYwDDucAhAcQT5Dv4BySmPAegLhCYCL2kIHAVAjQEEDDW2gIgRYBFqkC46mqBrLaURaEBkWgubkNqAIDON0WUhH0qCVJb4s9QuAfiAKEJYSQ1CrxJQefl1CqD1BsJR/LoSTLvs0urtPDLl1qBPUf8gbEYNIOcZa844%2BZOwTIKYCYAZivLbJOREojURom9EaHjFB1T2C3BVnbsqR0vZ15lolg7ADbzNbEFdW2ScIUB16ojkKm5rAjMtEgRJCr2ufYzGkOIJ1U6I5PRMN4OZY0QihG3eKGXVlgBtrcoxQIlVGCGYBw2JdTbjBQ76fhnBcxDyMtCaEzFKh3caoX3Gjae1haiYbor0X6JtN0itrPYp4Uv7qd1OxxCppG12jPkOkygG4lowVYU98E2Q7Msf3maLNt2ypORkm26boFROzNVmgQQgEp1ZuHQ9pFlFOBkEx49jQTvTTQBXDxOT/TNqHzHT7CUhx/bmgsL8DYc3sAyCqAIO6FiJU%2BktReC8O07HDkObfbhhx0z7eIU0lnO4dZzYrKBChMTOJhbi9TNtIMBDNzDCNhT/VAR7tAfoX0QaNU5eiw5kQEi3YVNv%2BuHKOpnjY5oidMk2DRJrUORPsUBPKCmnxyeRgAthPReZg5BFaDM44b7RriVAJH0QiRyeBJrtjzhEZRMMGSkcVhew0iQGxbQfgyJL7UcGOLIy0VeDZFGUORxGTIdyOvg8NdMa0QUe%2BmFGMCbkvKeBFKNTayijOPzfdJlkWJWcWqdedekySbQ8110I2HkVeBZyvxDRMdVNqmFTYT14MQYipmMz8GxNPk8TdBr1h5oHYxMFKD5KU3CL6jqRAHMjgcONH0ix21HJMWWUOGklUx8zCenCKtF40bR5AxXlhxJwXpLWmyJ0eiMJhcdDYXosUXynp4lQlAvo%2BZiYj1omJGaYnR/pJz66Q5UwiIxtNYk7HqoAx8MLdNiKVFXgVRuY73BqIyL/ZyRMY8HAhjqYBImxfVOsaW1NFNVlmXYsdD2OCx0t%2BxK5AHCuEcgiwxY0WZ0Zx3MHcdJxvHacf8OOjzidS/iJcf4hXHXCJObNDca1E05kE9Oe4w9AeLdHMpjxgDPwFukGYoiW%2B7HPkX2n0S0YiJJiM8WqMzgmJfImoskSk1IzFYnxW4gvq%2BLHbfi6UYE6iXFn0zGAmKSjPKuAgYl5iusy4nwMYFJGEYbxh2RhFxMtI8TR2xfa2gvwqYw4psJARLMljon%2BIpJNaK5AliSxwFCxOoiOKpJfEaTh%2B/EiiqOLfBYkcotE/%2Bso1H42dSgTQrcXqgrbbwAIkIpTszGxSOTocgk2WK5LjAiSS%2BHkiSVW28mC1XBQELEUEVGjaSC2%2BWTznkVUmycQpiYfYqCMSTrknGIQ/YrF1YDRVEoCQdgPiOrEjAr2B3KruuQ%2BxL8vOEAPKdzQKlRct%2BzvOjss1vKbwOo5eCqbuSqlukapNxEoOELv6atZOH/LKclA/JqMOpTQnIsoD87DQAug09csFyLChcmhEXbyL1KQCk5YubjeLuEHnxJdWpn3VOKIPwhItHSNgHVEqBlDmQzcb0j6RZFyBhhAmpaalqmFV6QhLA94bUDYEsAPB6AswKyAABZW8HFJeoEPVYCgCAWgXcASBKCAy9CqYerhb1VJfB3p50P6eY2UDB53MvvE8d2Q%2BkpBgi6EACEDJABNDIQaMjGfiEJBWQW8p2KyEzK9Tmc3KwIfIW2Fpn0B8aiYJmSzK%2BBsysCHMtgFzLJzKBuZiJHPpaT1QCz%2BSybYgrTJ4DiyVZPfKWdkBlmYzOZSsxWQjN5m4y1ZhaDUULLiEiz9QDwPWZLJSmsz0ZssrGQrKsg55LZfoPyTbJ2x2yucIgtVqb1XDmSTyOMv2SAHxmYAQ%2BxM36a8DJkUySiVM%2B2UWFpnBEqoLs5xg1Ajk816AisjzEzJawVppu8MGmfqDFnZyrZhs8OQZILmKzC5vsvkAmNaxGpy5XZbWfqF1k1zo5dc/OVZGblmyW5qs0uW1k7nMJK5KQJ2X3NbnMzXZMEQeSUUVkPBFZJc3CmXPT5dy2wwUM4BcGWg5y3BechuVZDXnwz15Vs/mVvKDnCDkZIc6wDqgWjWB5QQE1HE/JfmrhVQScgGfrOBnj8heXwMGRDOsBQz6A0wBWWnO5yhz80q4AABp6htQ7YckkzNjkh9n5r87%2Bf9MXRWQU5lgKBTvKLAChrAq4ZwHPNVl1yEFuoJBbgqLkxAeAo8nvuPI7nby040Ck3sSVIWrgFoKwFBVbLQWEzsgGCr%2BaTKbi0Kh5iM6mcQVwBijD5tcxedkDxYkKeFEi9zIwv9n6pbZWs3eR6EXQXAs5jM%2BRbnK%2BBKLuFKwL2SUXUXWzy0E81hQjG7JrB7SRYchQbIUWmKVFK8s%2BVYv5laLhZRYPeXopIbVzDF/ctxVwo8UWzlAcM7xerJvn3T4WyMmwPsiFCm1bQCobQAIjFidBUc3IEwg4R8Zv9ngWQfQL4DqniBMw8QK4GnATmphjZcshCCE0vE1KQAdSrGXjKrkRxSi8MMwBI0hDNLWlhIdpTPM6XtAzA6Vb%2Bg9KxjcgZFvHApYtE/lVLkgwi1MO4pWCpgZlsCVMR5GQZ5gYo3IfiJlDUAsxp8%2BzLQNPkWX%2BBH4fRZLMKWyVGVuQMoeIPF1TxtgMw6AOZYgvbAXKSoyykAPAs%2BW1KSF1gBMb9BkDGgwAMgCOB9l%2BX/LqF7YQFaQpBVPIZABACFVCsmXMJuQKwF8F5yTAEQiw5StcHApRpwrUok4AEGKHhi/LVlDS6KhrMhDUrwlaykABsp8BDz0V/iHpcQCiBfBGVyi5lY4v%2BDAB2VxWUZeMp2UosEY3IYsCwGIDWRvAnSV/vMtfmxdxl3yq8FcsG4QA%2BVZilZfip75uTtle/JGZKuOjch65FkuZQnPVXJBmlFqnwLSrdL0qiZP01MPaukAxz9Q0rVOaKpMRcqEgfS11X8vzlDKHgIqxhGKq/oSq75g%2BJwDIHlpMBZA5WGQOQDYCyBLAKa9ALIBxz8BneogCQNQkeD0AU1OnGQBmrKLkATwIAcrJYDMBwyAwiwAMAGE6gLBoZAATnKydrmAsgOGSmtaDVrpWaastZmtkAprhO0rUteWvIBwBYAKADADgHwDEAyAlAagHQAYWsAOAaAQQIIBEDiBJA0gSFVLjBDMhoBfwDMOKFpROlVgLuLICQGLhHLCQd6uBmQGNRnlEo8sDAviMXQEA1AmBOglUsPyaDs6cIT6f12tYqtrAhkViDqnfZwgQNyCkbvGAFC6hFQZMcyPAKbDb9hc0QkafMilb4KsYcGz6YhpGCrh2wSoJUPxDtnJBuQOG53mDJmCD5lACchXIuCFCPI8wUG6UDBtRxEaENw4eMLCqQXx4sNygOjTHwY0Ea04fG8PAJvZJMqRNtG3srhoY01F%2BCyqr%2BQ3nY0YrclU4F9fF07bMtTlcyq5Yujf6ztf1ylN/AdyIAAaVCQGgnlvl1AyhjQcIPgPISqxQBe8YPMRMAshnQzZg%2BKQDRrgTnWA3NHm0oDKCc0ubwtKwPPBJvw2WBLAQW%2BzSFp%2Bn8IWNWm6UHYHaihbYtuAFLUCQc3CK3NymiAFFuDIxb3NcWlTfhsC1hlNgwi/hCVqy1EQctGm0rbUCC1JL9EKSm6BkRWC4smqOS50iwHyUVoCeVc%2BLvUEm0zz4sLMeIK2RGASNoMEYdVf3BLydQkNKG3AGhvbApA4QtM2QvIQULLRHm%2BgGGWIGRoQAxZrgO8gvThqWALthc67U7Lu2udioCc4IECGQ2obVw6Gs7V9p%2B07a9tKQCGr9t23/b9th2/UMdsUIPbSgL26DnDVu1KzjVAYFHVduR2lA3taOveHsuFysrfASq4RetpcIkaOtqyuELgBSDWA%2BAVG5aKppDxkJhF32uCKsuWis6gQHO2GpTqZXU7ad9O2%2BSIRo3C4DlYwY5YulOU0xF06qzVTcr5J3L4YDyp5aFy3CvKAQ7ypVZ8ptX%2BAudcEITcgqqgQrwVkKipvrtwCG7loEK1FWbt2VxwsVOKkYLUCcU6BCV8CklUgrJUXtKVccDbXJr538q3iNO5DSkCVDLQkdIFZQA8GSoW6edAEOPUyrRQlaqdNOunQzt50p7%2BdcWlIKHqVAYqpVwuGVXKvxAHASdn81VV/V10kTrlvgKAInv5XgUs9QenPXnvahbTEwTzE1YLkL1rh85Vqn6dXuY0/S2dlu8jZRrzwAQgdBu8fRnox2I6sdUe5LWihu2L7q8fCB4HUQRY4R413amQMmtTXpqR1MgHNQIBj75r91HUZvCWqP0VrMCkUMsshD329ryA/amtYfuHXkAs1MgMdSAAnW375aVauGXDLMAPBLAAYSwOVkWA8B6AAYOGfQAeABh11sgB4CmqHUZqv9o68gJOt30zqEA8ACAHOq8ZqBBuFAKgM4WnCkGQAxgVoMAE6hgHyAXgbToumE4QB9AR%2B8gBNWMCxMZAxa8gG0CwFEB2wbWDgzgCPAcAHVn%2BwgOzMBgegODD4LAvEAPV8GaCiaz/QDH7XDqE17ATgNuv4DMADAwnSAPLTGByzhOMgYIMmCrDcBc1/AGGUUXbBZBggrQBIKBH0Bjq91UgBgAmqTVoGOD3%2Bl%2BcZGUDAAKIygTqKAbMDBEIAxQMQPeHagQBF1VlK/YwFGCUGfAiUMkFUVP2CAb9Wh%2BWvfpwDDkn9QBttWYHKwwH1yiBttYsE6iLBLAcM8rM/r7UDqP9GB7/b/v/1aHp1iAQg2gGIOkGV1FBkg%2BkeoOkBaD9B6VkwabisH2Dn%2Brg6YFkB8GBDF7YQ0alENTgdDkhjA9IY9mFBzDGBhQ1%2BSTq8GU1qhjgxodv3MAdDW62w0ID0D6AjDT%2B0w1jPMOWG/g1hvQ3wHsPBBHDRRFw0gDcMeGC10gO8nvoP3oHj9gR4I6EfCMPBIjpQGI3EdKCJGkgZIBhakeGNS60T7UbI/wFyPlr8jeqR/T4ZkAv639g6/w1gfHXYGADlakADwADBmAeA5WNtQGHKxwzm89amYIsEWDRK1DqB1o8fvxO4GejKAcQR4EGNMAkAwnRgKQGlP0mST4JykzIDRldg9g8oIIyEZSiwn4T0R54LEfaj5AsgUJrU2EYiNRHET7QYUxWqrU8BFgdah4NDMbUNGG1vJuGcgdJN%2BHP97RkQH/ppN5G99PAL020awM4GK10%2BcMIMrhlAA) is how GodBolt compares the CMSIS and CPPReg versions. Looking at the assembly, it's pretty darn close, with no performance diffirence (only minor instruction re-ordering).

![Assembly Comparison](Assembly_Comparison.PNG)

## Full working example

Lets say we are using a Cortex M0 based microcontroller like the [CY8C4013SXI-400](https://www.digikey.com/product-detail/en/cypress-semiconductor-corp/CY8C4013SXI-400/CY8C4013SXI-400-ND/4842995), the smallest ARM based microcontroller avaliable on Digikey at $1.32 in single unity quantities. It has a meager 8 kB of flash, 2kB of RAM, and runs at 16 Mhz.