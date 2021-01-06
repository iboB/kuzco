// kuzco
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include "Vector.hpp"
#include <vector>

// shorthand typedef
namespace kuzco
{
template <typename T>
using StdVector = Vector<std::vector<T>>;
}
