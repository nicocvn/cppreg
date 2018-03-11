//! Simple shadow value implementation.
/**
 * @file      ShadowValue.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2018 Sendyne Corp. All rights reserved.
 */


#ifndef CPPREG_SHADOWVALUE_H
#define CPPREG_SHADOWVALUE_H


#include "cppreg_Defines.h"


//! cppreg namespace.
namespace cppreg {


    //! Shadow value generic implementation.
    /**
     * @tparam Register Register type.
     * @tparam UseShadow Boolean flag indicating if shadow value is required.
     *
     * This implementation is for register which do not require shadow value.
     */
    template <typename Register, bool UseShadow> struct Shadow : std::false_type {};


    //! Shadow value implementation.
    /**
     * @tparam Register Register type.
     *
     * This implementation is for register which do require shadow value.
     *
     * See 
     */
    template <typename Register>
    struct Shadow<Register, true> : std::true_type {
        static typename Register::type shadow_value;
    };
    template <typename Register>
    typename Register::type Shadow<Register, true>::shadow_value =
        Register::reset;


}


#endif  // CPPREG_SHADOWVALUE_H
