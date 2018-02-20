//! Register traits implementation.
/**
 * @file      Traits.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 *
 * This header provides some traits for register type instantiation.
 */


#ifndef CPPREG_TRAITS_H
#define CPPREG_TRAITS_H


#include "cppreg_Defines.h"


//! cppreg namespace.
namespace cppreg {


    //! Register data type default implementation.
    /**
     * @tparam Size Register size.
     *
     * This will fail to compile if the register size is not implemented.
     */
    template <Width_t Size>
    struct RegisterType;

    //!@{ Specializations based on register size.
    template <> struct RegisterType<8u> { using type = std::uint8_t; };
    template <> struct RegisterType<16u> { using type = std::uint16_t; };
    template <> struct RegisterType<32u> { using type = std::uint32_t; };
    //!@}


}


#endif  // CPPREG_TRAITS_H
