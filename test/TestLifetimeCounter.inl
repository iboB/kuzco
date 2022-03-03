// kuzco
// Copyright (c) 2020-2022 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//

template <typename T>
struct LC
{
    LC()
    {
        ++alive;
        ++dc;
    }

    LC(const LC&)
    {
        ++alive;
        ++cc;
    }

    LC& operator=(const LC&)
    {
        ++ca;
        return *this;
    }

    LC(LC&&) noexcept
    {
        ++alive;
        ++mc;
    }

    LC& operator=(LC&&) noexcept
    {
        ++ma;
        return *this;
    }


    ~LC()
    {
        --alive;
    }

    static int alive;
    static int dc; // default constructed
    static int cc; // copy constructed
    static int mc; // move constructed
    static int ca; // copy assigned
    static int ma; // move assigned
};

template <typename T> int LC<T>::alive = 0;

std::vector<void(*)()>& allCountersClearFuncs()
{
    static std::vector<void(*)()> vec;
    return vec;
}
template <typename T> int LC<T>::dc = []() -> int {
    allCountersClearFuncs().emplace_back([]() {
        LC<T>::alive = 0;
        LC<T>::dc = 0;
        LC<T>::cc = 0;
        LC<T>::mc = 0;
        LC<T>::ca = 0;
        LC<T>::ma = 0;
        });
    return 0;
}();
template <typename T> int LC<T>::cc = 0;
template <typename T> int LC<T>::mc = 0;
template <typename T> int LC<T>::ca = 0;
template <typename T> int LC<T>::ma = 0;

void clearAllCounters()
{
    for(auto& c : allCountersClearFuncs()) c();
}

struct LCScope
{
    ~LCScope()
    {
        clearAllCounters();
    }
};
