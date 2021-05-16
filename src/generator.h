#ifndef INCLUDED_ALCHIMIA_WORLD_GENERATOR
#define INCLUDED_ALCHIMIA_WORLD_GENERATOR
#include <coroutine>
#include <exception>
#include <cstdint>
#include <utility>

namespace alc {

template <typename T> class generator;
template <typename T> class generator_iterator;

template <typename T>
class generator_promise
{
public:
    using value_type = std::decay_t<T>;

private:
    value_type*        d_val;
    std::exception_ptr d_exception;

public:
    generator_promise() = default;

    generator<T> get_return_object() noexcept
    {
        return generator<T>{
            std::coroutine_handle<generator_promise<T>>::from_promise(*this)
        };
    }

    constexpr std::suspend_always initial_suspend() const noexcept { return {}; }
    constexpr std::suspend_always final_suspend() const noexcept { return {}; }

    void return_void() {}

    std::suspend_always yield_value(value_type& value) noexcept
    {
        d_val = std::addressof(value);
        return {};
    }

    std::suspend_always yield_value(value_type&& value) noexcept
    {
        d_val = std::addressof(value);
        return {};
    }

    void unhandled_exception() { d_exception = std::current_exception(); }

    value_type& value() const noexcept { return *d_val; }

    void rethrow_if()
    {
        if (d_exception) {
            std::rethrow_exception(d_exception);
        }
    }
};

template <typename T>
class generator
{
public:
    using promise_type = generator_promise<T>;
    using value_type = typename promise_type::value_type;

    using iterator = generator_iterator<T>;

private:
    std::coroutine_handle<promise_type> d_coroutine;

    generator(const generator&) = delete;
    generator& operator=(generator) = delete;

public:
    explicit generator(std::coroutine_handle<promise_type> coroutine) noexcept
        : d_coroutine(coroutine)
    {}

    explicit generator(generator&& other)
        : d_coroutine(std::move(other.d_coroutine))
    {}

    ~generator()
    {
        if (d_coroutine) { d_coroutine.destroy(); }
    }

    [[nodiscard]] iterator begin()
    {
        advance();
        return iterator{*this};
    }

    [[nodiscard]] iterator::sentinel end() noexcept
    {
        return {};
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return d_coroutine && !d_coroutine.done();
    }

    void advance()
    {
        if (d_coroutine) {
            d_coroutine.resume();
            if (d_coroutine.done()) {
                d_coroutine.promise().rethrow_if();
            }
        }
    }

    [[nodiscard]] value_type& value() const
    {
        return d_coroutine.promise().value();
    }
};

template <typename T>
class generator_iterator
{
public:
    using value_type = typename generator<T>::value_type;

    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;

    struct sentinel {};

private:
    generator<T>* d_owner;

public:
    explicit generator_iterator(generator<T>& owner) noexcept
        : d_owner(&owner)
    {
        static_assert(std::is_copy_assignable_v<generator_iterator<T>>);
        static_assert(std::is_trivially_destructible_v<generator_iterator<T>>);
    }

    generator_iterator& operator=(const generator_iterator& other)
    {
        d_owner = other.d_owner;
        return *this;
    }

    friend bool operator==(const generator_iterator& it, sentinel) noexcept
    {
        return !it.d_owner->valid();
    }

    generator_iterator& operator++()
    {
        d_owner->advance();
        return *this;
    }

    generator_iterator& operator++(int) { return operator++(); }

    [[nodiscard]] value_type& operator*() const noexcept
    {
        return d_owner->value();
    }

    [[nodiscard]] value_type* operator->() const noexcept
    {
        return std::addressof(operator*());
    }
};

}

#endif // INCLUDED_ALCHIMIA_WORLD_GENERATOR