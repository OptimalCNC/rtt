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
#define ORO_REMOVE_CLANG_FUNCTION_EFFECT(EFFECT, QUALIFIERS, EXCEPTION_SPEC)    \
        template<class R, class C, class... Args>                              \
        struct RemoveFunctionEffects<                                          \
            __attribute__((EFFECT)) R (C::*)(Args...) QUALIFIERS EXCEPTION_SPEC> \
        {                                                                       \
            typedef R (C::*type)(Args...) QUALIFIERS EXCEPTION_SPEC;           \
        };

#define ORO_REMOVE_CLANG_FUNCTION_EFFECT_QUALIFIERS(EFFECT, EXCEPTION_SPEC)     \
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(EFFECT, , EXCEPTION_SPEC)             \
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(EFFECT, const, EXCEPTION_SPEC)        \
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(EFFECT, volatile, EXCEPTION_SPEC)     \
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(EFFECT, const volatile, EXCEPTION_SPEC) \
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(EFFECT, &, EXCEPTION_SPEC)            \
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(EFFECT, const &, EXCEPTION_SPEC)      \
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(EFFECT, volatile &, EXCEPTION_SPEC)   \
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(EFFECT, const volatile &, EXCEPTION_SPEC) \
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(EFFECT, &&, EXCEPTION_SPEC)           \
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(EFFECT, const &&, EXCEPTION_SPEC)     \
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(EFFECT, volatile &&, EXCEPTION_SPEC)  \
        ORO_REMOVE_CLANG_FUNCTION_EFFECT(EFFECT, const volatile &&, EXCEPTION_SPEC)

#if __has_attribute(nonblocking)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT_QUALIFIERS(nonblocking, )
#if defined(__cpp_noexcept_function_type) && __cpp_noexcept_function_type >= 201510L
        ORO_REMOVE_CLANG_FUNCTION_EFFECT_QUALIFIERS(nonblocking, noexcept)
#endif
#endif

#if __has_attribute(nonallocating)
        ORO_REMOVE_CLANG_FUNCTION_EFFECT_QUALIFIERS(nonallocating, )
#if defined(__cpp_noexcept_function_type) && __cpp_noexcept_function_type >= 201510L
        ORO_REMOVE_CLANG_FUNCTION_EFFECT_QUALIFIERS(nonallocating, noexcept)
#endif
#endif

#undef ORO_REMOVE_CLANG_FUNCTION_EFFECT_QUALIFIERS
#undef ORO_REMOVE_CLANG_FUNCTION_EFFECT
#endif

        template<class FunctionT>
        struct RemoveNoexcept
        {
            typedef FunctionT type;
        };

#if defined(__cpp_noexcept_function_type) && __cpp_noexcept_function_type >= 201510L
#define ORO_REMOVE_NOEXCEPT(QUALIFIERS)                                         \
        template<class R, class C, class... Args>                              \
        struct RemoveNoexcept<R (C::*)(Args...) QUALIFIERS noexcept>           \
        {                                                                       \
            typedef R (C::*type)(Args...) QUALIFIERS;                          \
        };

        ORO_REMOVE_NOEXCEPT()
        ORO_REMOVE_NOEXCEPT(const)
        ORO_REMOVE_NOEXCEPT(volatile)
        ORO_REMOVE_NOEXCEPT(const volatile)
        ORO_REMOVE_NOEXCEPT(&)
        ORO_REMOVE_NOEXCEPT(const &)
        ORO_REMOVE_NOEXCEPT(volatile &)
        ORO_REMOVE_NOEXCEPT(const volatile &)
        ORO_REMOVE_NOEXCEPT(&&)
        ORO_REMOVE_NOEXCEPT(const &&)
        ORO_REMOVE_NOEXCEPT(volatile &&)
        ORO_REMOVE_NOEXCEPT(const volatile &&)

#undef ORO_REMOVE_NOEXCEPT
#endif

        /**
         * Produce the ordinary member-function pointer type understood by
         * Boost. RTT's wrapper does not inherit the target's constraints.
         */
        template<class FunctionT>
        struct BoostCompatibleFunction
        {
            typedef typename RemoveFunctionEffects<FunctionT>::type WithoutEffects;
            typedef typename RemoveNoexcept<WithoutEffects>::type type;
        };

        template<class FunctionT>
        inline typename BoostCompatibleFunction<FunctionT>::type
        boostCompatibleFunction(FunctionT function)
        {
            typedef typename RemoveFunctionEffects<FunctionT>::type WithoutEffects;
            WithoutEffects without_effects = function;
            return without_effects;
        }
    }
}

#endif
