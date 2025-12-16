// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
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
    SharedState<ndc> ri = Node<ndc>(3);
    auto d = ri.detach();
    auto d2 = d;
}
