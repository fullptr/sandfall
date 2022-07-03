#pragma once

namespace sand {

template <typename Func>
class scope_exit
{
    Func d_func;

    scope_exit(const scope_exit&) = delete;
    scope_exit& operator=(const scope_exit&) = delete;

    scope_exit(scope_exit&&) = delete;
    scope_exit& operator=(scope_exit&&) = delete;

public:
    scope_exit(Func&& func) : d_func(std::move(func)) {}
    ~scope_exit() { d_func(); }
};

}