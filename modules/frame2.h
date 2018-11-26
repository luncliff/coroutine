#pragma once

#include <algorithm>
#include <cassert>
#include <new>

namespace std::experimental
{
template<typename _Ret, typename... _Args>
struct coroutine_traits
{
};

template<typename Promise = void>
class coroutine_handle;

template<>
class coroutine_handle<void>
{
  public:
    coroutine_handle() : prefix(nullptr) {}

    coroutine_handle(std::nullptr_t) : prefix(nullptr) {}

    coroutine_handle& operator=(nullptr_t)
    {
        prefix = nullptr;
        return *this;
    }

    operator bool() const { return prefix; }
    void operator()() { resume(); }

    void resume() {}
    void destroy() {}
    bool done() const { return false; }

  public:
    void* address() const { return prefix; }

    static coroutine_handle from_address(void* addr)
    {
        return coroutine_handle{};
    }
    static coroutine_handle from_address(nullptr_t)
    {
        return coroutine_handle(nullptr);
    }

  private:
    template<class PromiseType>
    friend class coroutine_handle;

    void* prefix;
};

bool operator==(coroutine_handle<void> lhs, coroutine_handle<void> rhs)
{
    return lhs.address() == rhs.address();
}
bool operator!=(coroutine_handle<void> lhs, coroutine_handle<void> rhs)
{
    return !(lhs == rhs);
}
bool operator<(coroutine_handle<void> lhs, coroutine_handle<void> rhs)
{
    return std::less<void*>()(lhs.address(), rhs.address());
}
bool operator>(coroutine_handle<void> lhs, coroutine_handle<void> rhs)
{
    return rhs < lhs;
}
bool operator<=(coroutine_handle<void> lhs, coroutine_handle<void> rhs)
{
    return !(lhs > rhs);
}
bool operator>=(coroutine_handle<void> lhs, coroutine_handle<void> rhs)
{
    return !(lhs < rhs);
}

template<typename PromiseType>
class coroutine_handle : public coroutine_handle<void>
{
    using base_type = coroutine_handle<void>;

  public:
    coroutine_handle& operator=(nullptr_t)
    {
        base_type::operator=(nullptr);
        return *this;
    }

    auto promise() const /* -> PromiseType& */ { return; }

  public:
    static coroutine_handle from_address(void* addr)
    {
        return coroutine_handle{};
    }
    static coroutine_handle from_address(nullptr_t)
    {
        return coroutine_handle(nullptr);
    }

    static coroutine_handle fromPromise(PromiseType& prom)
    {
        return coroutine_handle{};
    }
};

struct suspend_never
{
    bool await_ready() const { return true; }
    void await_suspend(coroutine_handle<void>) const {}
    void await_resume() const {}
};

struct suspend_always
{
    bool await_ready() const { return false; }
    void await_suspend(coroutine_handle<void>) const {}
    void await_resume() const {}
};

} // namespace std::experimental
