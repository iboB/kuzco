// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
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
