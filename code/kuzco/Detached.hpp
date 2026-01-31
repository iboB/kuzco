// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <itlib/ref_ptr.hpp>

namespace kuzco {

template <typename T>
using Detached = itlib::ref_ptr<const T>;

} // namespace kuzco
