// kuzco
// Copyright (c) 2020-2022 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include "NodeVector.hpp"
#include <vector>

// shorthand typedef
namespace kuzco
{
template <typename T>
using NodeStdVector = NodeVector<T, std::vector>;
}
