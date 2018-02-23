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
    template <typename Register, bool UseShadow>
    struct Shadow {
        constexpr static const bool use_shadow = false;
    };


    //! Shadow value implementation.
    /**
     * @tparam Register Register type.
     *
     * This implementation is for register which do require shadow value.
     *
     * See 
     */
    template <typename Register>
    struct Shadow<Register, true> {
        static typename Register::type value;
        constexpr static const bool use_shadow = true;
    };
    template <typename Register>
    typename Register::type Shadow<Register, true>::value =
        Register::reset;
    template <typename Register>
    constexpr const bool Shadow<Register, true>::use_shadow;

}


#endif  // CPPREG_SHADOWVALUE_H
