#pragma once
#include <kuzco/Node.hpp>

#include <doctest/util/lifetime_counter.hpp>

#include <vector>
#include <string>
#include <string_view>

struct PersonData : public doctest::util::lifetime_counter<PersonData> {
    PersonData() = default;
    PersonData(std::string_view n, int a) : name(n), age(a) {}
    std::string name;
    int age = 0;
};

struct Employee : public doctest::util::lifetime_counter<Employee> {
    kuzco::Node<PersonData> data;
    kuzco::Node<std::string> department;
    double salary = 0;
};

struct Pair : public doctest::util::lifetime_counter<Pair> {
    kuzco::Node<Employee> a;
    kuzco::Node<Employee> b;
    std::string type;
};

using Blob = std::unique_ptr<std::vector<int>>;

struct Boss : public doctest::util::lifetime_counter<Boss> {
    kuzco::Node<PersonData> data;
    kuzco::Node<Blob> blob;
};

struct Company : public doctest::util::lifetime_counter<Company> {
    std::vector<kuzco::Node<Employee>> staff;
    kuzco::Node<Boss> ceo;
    kuzco::OptNode<Boss> cto;
};
