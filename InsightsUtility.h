/******************************************************************************
 *
 * C++ Insights, copyright (C) by Andreas Fertig
 * Distributed under an MIT license. See LICENSE for details
 *
 ****************************************************************************/

#ifndef INSIGHTS_UTILITY_H
#define INSIGHTS_UTILITY_H

#include "llvm/ADT/STLExtras.h"

#include <type_traits>
#include <utility>
//-----------------------------------------------------------------------------

///! A helper inspired by https://github.com/Microsoft/wil/wiki/Error-handling-helpers
#define RETURN_IF(cond)                                                                                                \
    if(cond) {                                                                                                         \
        return;                                                                                                        \
    }
//-----------------------------------------------------------------------------

using void_func_ref = llvm::function_ref<void()>;
//-----------------------------------------------------------------------------

template<typename T, typename... Ts>
concept same_as_any_of = (std::same_as<T, Ts> or ...);

#endif /* INSIGHTS_UTILITY_H */
