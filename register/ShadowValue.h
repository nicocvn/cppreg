//! Simple shadow value implementation.
/**
 * @file      ShadowValue.h
 * @author    Nicolas Clauvelin (nclauvelin@sendyne.com)
 * @copyright Copyright 2010-2019 Sendyne Corp. All rights reserved.
 */


#ifndef CPPREG_SHADOWVALUE_H
#define CPPREG_SHADOWVALUE_H


#include "cppreg_Defines.h"


namespace cppreg {


//! Shadow value generic implementation.
/**
 * @tparam Register Register type.
 * @tparam use_shadow Boolean flag indicating if shadow value is required.
 */
template <typename Register, bool use_shadow>
struct Shadow : std::false_type {};


//! Shadow value specialization.
/**
 * @tparam Register Register type.
 *
 * This implementation is for register which do require shadow value.
 */
template <typename Register>
struct Shadow<Register, true> : std::true_type {
    static typename Register::type shadow_value;
};
template <typename Register>
typename Register::type Shadow<Register, true>::shadow_value = Register::reset;


}    // namespace cppreg


#endif    // CPPREG_SHADOWVALUE_H
