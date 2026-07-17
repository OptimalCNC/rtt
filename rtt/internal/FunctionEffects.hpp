/***************************************************************************
  tag: MetaNC  Fri Jul 17 09:20:00 CST 2026  FunctionEffects.hpp

                        FunctionEffects.hpp -  description
                           -------------------
    begin                : Fri July 17 2026
    copyright            : (C) 2026 MetaNC

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ORO_FUNCTION_EFFECTS_HPP
#define ORO_FUNCTION_EFFECTS_HPP

namespace RTT
{
    namespace internal
    {
        /**
         * Remove Clang performance constraints before passing function types
         * and pointers to Boost. This weakens the constraint without changing
         * the target function or adding a constraint to RTT's wrapper.
         */
        template<class FunctionT>
        struct RemoveFunctionEffects
        {
            typedef FunctionT type;
        };

#if defined(__clang__)
// noexcept is intentionally excluded: RTT's noexcept callable compatibility
// is a separate concern and its exception specification must not be erased.
#define ORO_REMOVE_CLANG_FUNCTION_EFFECT(EFFECT, QUALIFIERS)                    \
        template<class R, class C, class... Args>                              \
        struct RemoveFunctionEffects<                                          \
            __attribute__((EFFECT)) R (C::*)(Args...) QUALIFIERS>              \
        {                                                                       \
            typedef R (C::*type)(Args...) QUALIFIERS;                          \
        };

#if __has_attribute(nonblocking)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonblocking, )
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonblocking, const)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonblocking, volatile)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonblocking, const volatile)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonblocking, &)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonblocking, const &)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonblocking, volatile &)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonblocking, const volatile &)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonblocking, &&)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonblocking, const &&)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonblocking, volatile &&)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonblocking, const volatile &&)
#endif

#if __has_attribute(nonallocating)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonallocating, )
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonallocating, const)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonallocating, volatile)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonallocating, const volatile)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonallocating, &)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonallocating, const &)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonallocating, volatile &)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonallocating, const volatile &)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonallocating, &&)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonallocating, const &&)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonallocating, volatile &&)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(nonallocating, const volatile &&)
#endif

#undef ORO_REMOVE_CLANG_FUNCTION_EFFECT
#endif

        template<class FunctionT>
        inline typename RemoveFunctionEffects<FunctionT>::type
        removeFunctionEffects(FunctionT function)
        {
            return function;
        }
    }
}

#endif
