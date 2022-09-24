// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once

namespace kuzco
{

template <typename T>
class Subscriber
{
public:
    virtual ~Subscriber();
    virtual void notify(const T& t) = 0;
};

} // namespace kuzco
