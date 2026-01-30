// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <kuzco/Detached.hpp>
#include <doctest/doctest.h>
#include <string>

struct Animal {
    Animal(std::string n) : name(std::move(n)) {}
    std::string name;
    virtual std::string speak() const = 0;
};

struct Dog : public Animal {
    using Animal::Animal;
    std::string speak() const override {
        return name + ": woof";
    }
};

TEST_CASE("Detached") {
    auto d = kuzco::Create_Detached(Dog("Buddy"));
    CHECK(d->speak() == "Buddy: woof");
    auto d2 = d;
    CHECK(d2 == d);

    kuzco::Detached<Animal> a = d;
    CHECK(a->speak() == "Buddy: woof");

    CHECK(a == d);
    CHECK(a >= d);
    CHECK_FALSE(a < d);
}
