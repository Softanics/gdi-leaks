// Copyright (c) Softanics

#pragma once

#include <windows.h>
#include <stdexcept>

namespace Api
{
    /// Stores a function pointer and a checker
    template<class Ret, class E, class... Args>
    struct WinApiCallSite
    {
        typedef Ret (WINAPI* TApiPtr)(Args...);

        const TApiPtr _apiPtr;
        const E _e;

        WinApiCallSite(TApiPtr apiPtr, E e) : _apiPtr(apiPtr), _e(e) {}

        Ret operator() (Args... args) const
        {
            auto const res = _apiPtr(args...);
            auto const lastError = GetLastError();

            if (!_e(res, lastError))
                // todo
                throw std::runtime_error("todo failed");

            return res;
        }
    };

    /// `WinApiCallSite` that has no return value
    template<class E, class... Args>
    struct WinApiCallSite<void, E, Args...>
    {
        typedef void (WINAPI* TApiPtr)(Args...);

        const TApiPtr _apiPtr;
        const E _e;

        WinApiCallSite(TApiPtr apiPtr, E e) : _apiPtr(apiPtr), _e(e) {}

        void operator() (Args... args) const
        {
            _apiPtr(args...);
            auto const lastError = GetLastError();

            if (!_e(lastError))
                // todo
                throw std::runtime_error("todo failed");
        }
    };

    /// Stores a function pointer and a checker
    template<class Ret, class E, class... Args>
    struct NTApiCallSite
    {
        typedef Ret(WINAPI* TApiPtr)(Args...);

        const TApiPtr _apiPtr;
        const E _e;

        NTApiCallSite(TApiPtr apiPtr, E e) : _apiPtr(apiPtr), _e(e) {}

        Ret operator() (Args... args) const
        {
            auto const status = _apiPtr(args...);

            if (!_e(status))
                throw std::runtime_error("failed");

            return status;
        }
    };

    /// Creates `WinApiCallSite` from a concrete function pointer and a checker instance
    template <typename R, typename E, typename... A>
    decltype(auto) MakeWinApiCallSite(R (WINAPI* ptr)(A...), E e)
    {
        return WinApiCallSite<R, E, A...>(ptr, e);
    }

    /// Creates `NTApiCallSite` from a concrete function pointer and a checker instance
    template <typename R, typename E, typename... A>
    decltype(auto) MakeNTApiCallSite(R (WINAPI* ptr)(A...), E e)
    {
        return NTApiCallSite<R, E, A...>(ptr, e);
    }

    auto const CheckNTApiFunctionResult = [](NTSTATUS status)
    {
        return 0 /* STATUS_SUCCESS */ == status;
    };
}
