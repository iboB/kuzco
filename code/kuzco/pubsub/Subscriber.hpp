// kuzco
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
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
