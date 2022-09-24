// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
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
