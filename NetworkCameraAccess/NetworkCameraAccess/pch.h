#pragma once
#pragma comment(lib, "mf")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WINRT_LEAN_AND_MEAN
#define WINRT_LEAN_AND_MEAN
#endif
#include <mfidl.h>
#include <mfapi.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <mfcaptureengine.h>

#include "wil/resource.h"
#include "wil/winrt.h"

#include "winrt/base.h"

#include "wil/cppwinrt.h"
#include "wil/cppwinrt_helpers.h"

#include <iostream>

template <typename ... Args>
static void Print(char const* const format, Args ... args) noexcept
{
    char buffer[1024] = {};
    ::sprintf_s(buffer, std::size(buffer), format, args ...);
    ::printf_s("%s\n", buffer);
}

template <typename ... Args>
static void PrintIf(bool condition, char const* const format, Args ... args) noexcept
{
    if (condition)
    {
        Print(format, std::forward<Args>(args)...);
    }
}

static auto InitializeApp() noexcept
{
    wil::SetResultLoggingCallback([](wil::FailureInfo const& failure) noexcept
        {
            std::array<wchar_t, 2048> msg{};
            if (SUCCEEDED(wil::GetFailureLogString(msg.data(), msg.max_size(), failure)))
            {
                std::fputws(msg.data(), stderr);
            }
        });

    winrt::init_apartment();
    WINRT_VERIFY_(S_OK, ::MFStartup(MF_VERSION, MFSTARTUP_FULL));
    return wil::scope_exit([]
        {
            WINRT_VERIFY_(S_OK, ::MFShutdown());
        });
}
