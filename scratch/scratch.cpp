// kuzco
// Copyright (c) 2020-2022 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include <kuzco/Kuzco.hpp>

#include <iostream>
#include <thread>
#include <optional>

using namespace kuzco;
using namespace std;

struct ndc
{
    ndc(int) {}
};

int main()
{
    State<ndc> ri = Node<ndc>(3);
    auto d = ri.detach();
    auto d2 = d;
}
