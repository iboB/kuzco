// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once

#include <type_traits>

namespace kuzco::impl
{
// copied from type_traits.hpp from https://github.com/iboB/itlib
template <template <typename...> class, typename...>
struct instance_of : public std::false_type {};

template <template <typename...> class Template, typename... TArgs>
struct instance_of<Template, Template<TArgs...>> : public std::true_type {};

template <template <typename...> class Template, typename Type>
inline constexpr bool instance_of_v = instance_of<Template, Type>::value;
}
