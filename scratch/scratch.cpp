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
    RootObject<ndc> ri = NewObject<ndc>(3);
    auto d = ri.detach();
    auto d2 = d;
}
