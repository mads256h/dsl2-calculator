#pragma once

#include <type_traits>
#include <utility>
#include <vector>
#include <string>
#include <stdexcept>
#include <ostream>

using state_t = std::vector<double>;

enum class operation_t {
    assign,
    plus,
    minus,
    mul,
    div,
};

template<typename T>
struct node_t {
    [[nodiscard]] constexpr double operator()(state_t &state) const;

    [[nodiscard]] constexpr virtual const T &get() const noexcept = 0;

    constexpr virtual ~node_t() = default;
};

template<typename T>
concept Node = std::is_base_of_v<node_t<T>, T>;

template<Node T>
struct unary_t final : node_t<unary_t<T>> {
    using value_type = T;

    const operation_t m_operation;
    const value_type m_value;

    constexpr unary_t(const operation_t operation, const value_type &value) : m_operation(operation), m_value(value) {}

    [[nodiscard]] constexpr const unary_t<T> &get() const noexcept override {
        return *this;
    }
};

template<Node First, Node Second>
struct binary_t final : node_t<binary_t<First, Second>> {
    using first_type = First;
    using second_type = Second;

    const operation_t m_operation;
    const first_type m_first;
    const second_type m_second;

    constexpr binary_t(const operation_t operation, const First &first, const Second &second) : m_operation(operation),
                                                                                                m_first(first),
                                                                                                m_second(second) {}

    [[nodiscard]] constexpr const binary_t<First, Second> &get() const noexcept override {
        return *this;
    }
};

struct variable_t final : node_t<variable_t> {
    const std::size_t m_id;

    constexpr explicit variable_t(const std::size_t id) noexcept: m_id(id) {}

    [[nodiscard]] constexpr const variable_t &get() const noexcept override {
        return *this;
    }
};

template<Node Second>
struct assign_t final : node_t<assign_t<Second>> {
    using second_type = Second;

    const operation_t m_operation;
    const variable_t m_first;
    const second_type m_second;

    constexpr assign_t(const operation_t operation, const variable_t &first, const second_type second) : m_operation(
            operation), m_first(first), m_second(second) {}

    [[nodiscard]] constexpr const assign_t<Second> &get() const noexcept override {
        return *this;
    }
};

struct symbol_table_t {
    std::vector<std::string> m_names;
    state_t m_state;

    [[nodiscard]] constexpr variable_t variable(std::string name, const double init) {
        const auto id = m_state.size();
        m_names.push_back(std::move(name));
        m_state.push_back(init);
        return variable_t(id);
    }

    [[nodiscard]] constexpr const std::string &name(const std::size_t id) const noexcept {
        return m_names[id];
    }
};

struct constant_t final : node_t<constant_t> {
    const double m_value;

    constexpr constant_t(const double value) noexcept: m_value(value) {}

    [[nodiscard]] constexpr const constant_t &get() const noexcept override {
        return *this;
    }
};

struct eval_visitor_t {
    state_t &m_state;

    constexpr explicit eval_visitor_t(state_t &state) : m_state(state) {}

    template<Node T>
    [[nodiscard]] constexpr double visit(const unary_t<T> &node) const {
        switch (node.m_operation) {
            case operation_t::plus:
                return visit(node.m_value);
            case operation_t::minus:
                return -visit(node.m_value);
        }
    }

    template<Node First, Node Second>
    [[nodiscard]] constexpr double visit(const binary_t<First, Second> &node) const {
        switch (node.m_operation) {
            case operation_t::plus:
                return visit(node.m_first) + visit(node.m_second);
            case operation_t::minus:
                return visit(node.m_first) - visit(node.m_second);
            case operation_t::mul:
                return visit(node.m_first) * visit(node.m_second);
            case operation_t::div:
                auto second = visit(node.m_second);
                if (second == 0) {
                    throw std::logic_error{"division by zero"};
                }
                return visit(node.m_first) / second;
        }
    }

    [[nodiscard]] constexpr double visit(const variable_t &node) const noexcept {
        return m_state[node.m_id];
    }

    template<Node Second>
    [[nodiscard]] constexpr double visit(const assign_t<Second> &node) const {
        const auto value = visit(node.m_second);
        auto &variable = m_state[node.m_first.m_id];
        switch (node.m_operation) {
            case operation_t::assign:
                variable = value;
                break;
            case operation_t::plus:
                variable += value;
                break;
            case operation_t::minus:
                variable -= value;
                break;
            case operation_t::mul:
                variable *= value;
                break;
            case operation_t::div:
                variable /= value;
                break;
        }
        return variable;
    }

    [[nodiscard]] constexpr double visit(const constant_t &node) const noexcept {
        return node.m_value;
    }
};

struct print_visitor {
    std::ostream &m_out;
    const symbol_table_t &m_symbol_table;

    print_visitor(std::ostream &out, const symbol_table_t &symbol_table) : m_out(out), m_symbol_table(symbol_table) {}

    template<Node T>
    void visit(const unary_t<T> &node) {
        switch (node.m_operation) {
            case operation_t::plus:
                break;
            case operation_t::minus:
                m_out << '-';
                break;
        }

        visit(node.m_value);
    }

    template<Node First, Node Second>
    void visit(const binary_t<First, Second> &node) {
        visit(node.m_first);

        switch (node.m_operation) {
            case operation_t::plus:
                m_out << '+';
                break;
            case operation_t::minus:
                m_out << '-';
                break;
            case operation_t::mul:
                m_out << '*';
                break;
            case operation_t::div:
                m_out << '/';
                break;
        }

        visit(node.m_second);
    }

    void visit(const variable_t &node) {
        m_out << m_symbol_table.name(node.m_id);
    }

    template<Node Second>
    void visit(const assign_t<Second> &node) {
        visit(node.m_first);
        switch (node.m_operation) {
            case operation_t::assign:
                m_out << "<<=";
                break;
            case operation_t::plus:
                m_out << "+=";
                break;
            case operation_t::minus:
                m_out << "-=";
                break;
            case operation_t::mul:
                m_out << "*=";
                break;
            case operation_t::div:
                m_out << "/=";
                break;
        }
        visit(node.m_second);
    }

    void visit(const constant_t &node) {
        m_out << node.m_value;
    }
};

template<Node T>
struct printer {
    const symbol_table_t &m_symbol_table;
    const T &m_node;

    printer(const symbol_table_t &symbol_table, const T &node) : m_symbol_table(symbol_table), m_node(node) {}

    friend std::ostream &operator<<(std::ostream &out, const printer &printer) {
        print_visitor visitor{out, printer.m_symbol_table};
        visitor.visit(printer.m_node);
        return out;
    }
};

template<Node First, Node Second>
constexpr binary_t<First, Second> operator+(const First &first, const Second &second) {
    return binary_t(operation_t::plus, first, second);
}

template<Node Second>
constexpr binary_t<constant_t, Second> operator+(const constant_t &first, const Second &second) {
    return binary_t(operation_t::plus, first, second);
}

template<Node First>
constexpr binary_t<First, constant_t> operator+(const First &first, const constant_t &second) {
    return binary_t(operation_t::plus, first, second);
}

template<Node First, Node Second>
constexpr binary_t<First, Second> operator-(const First &first, const Second &second) {
    return binary_t(operation_t::minus, first, second);
}

template<Node Second>
constexpr binary_t<constant_t, Second> operator-(const constant_t &first, const Second &second) {
    return binary_t(operation_t::minus, first, second);
}

template<Node First>
constexpr binary_t<First, constant_t> operator-(const First &first, const constant_t &second) {
    return binary_t(operation_t::minus, first, second);
}

template<Node First, Node Second>
constexpr binary_t<First, Second> operator*(const First &first, const Second &second) {
    return binary_t(operation_t::mul, first, second);
}

template<Node Second>
constexpr binary_t<constant_t, Second> operator*(const constant_t &first, const Second &second) {
    return binary_t(operation_t::mul, first, second);
}

template<Node First>
constexpr binary_t<First, constant_t> operator*(const First &first, const constant_t &second) {
    return binary_t(operation_t::mul, first, second);
}

template<Node First, Node Second>
constexpr binary_t<First, Second> operator/(const First &first, const Second &second) {
    return binary_t(operation_t::div, first, second);
}

template<Node Second>
constexpr binary_t<constant_t, Second> operator/(const constant_t &first, const Second &second) {
    return binary_t(operation_t::div, first, second);
}

template<Node First>
constexpr binary_t<First, constant_t> operator/(const First &first, const constant_t &second) {
    return binary_t(operation_t::div, first, second);
}

template<Node T>
constexpr unary_t<T> operator+(const T &node) {
    return unary_t(operation_t::plus, node);
}

constexpr unary_t<constant_t> operator+(const constant_t &node) {
    return unary_t(operation_t::plus, node);
}

template<Node T>
constexpr unary_t<T> operator-(const T &node) {
    return unary_t(operation_t::minus, node);
}

constexpr unary_t<constant_t> operator-(const constant_t &node) {
    return unary_t(operation_t::minus, node);
}

template<Node Second>
constexpr assign_t<Second> operator<<=(const variable_t &first, const Second &second) {
    return assign_t(operation_t::assign, first, second);
}

constexpr assign_t<constant_t> operator<<=(const variable_t &first, const constant_t &second) {
    return assign_t(operation_t::assign, first, second);
}

template<Node Second>
constexpr assign_t<Second> operator+=(const variable_t &first, const Second &second) {
    return assign_t(operation_t::plus, first, second);
}

constexpr assign_t<constant_t> operator+=(const variable_t &first, const constant_t &second) {
    return assign_t(operation_t::plus, first, second);
}

template<Node Second>
constexpr assign_t<Second> operator-=(const variable_t &first, const Second &second) {
    return assign_t(operation_t::minus, first, second);
}

constexpr assign_t<constant_t> operator-=(const variable_t &first, const constant_t &second) {
    return assign_t(operation_t::minus, first, second);
}

template<Node Second>
constexpr assign_t<Second> operator*=(const variable_t &first, const Second &second) {
    return assign_t(operation_t::mul, first, second);
}

constexpr assign_t<constant_t> operator*=(const variable_t &first, const constant_t &second) {
    return assign_t(operation_t::mul, first, second);
}

template<Node Second>
constexpr assign_t<Second> operator/=(const variable_t &first, const Second &second) {
    return assign_t(operation_t::div, first, second);
}

constexpr assign_t<constant_t> operator/=(const variable_t &first, const constant_t &second) {
    return assign_t(operation_t::div, first, second);
}

template<typename T>
constexpr double node_t<T>::operator()(state_t &state) const {
    eval_visitor_t visitor{state};
    return visitor.visit(get());
}
