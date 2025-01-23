#pragma once
#pragma comment(lib, "mf")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "d3d11")
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WINRT_LEAN_AND_MEAN
#define WINRT_LEAN_AND_MEAN
#endif

#define VV_STRINGIFY(val) #val
#define VV_VAR_TO_STRING(var, val) \
    if (var == val)                \
        return VV_STRINGIFY(val)

#include <mfidl.h>
#include <mfapi.h>
#include <mfcaptureengine.h>
#include <mferror.h>
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <iostream>
#include "wil/resource.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Foundation.Collections.h"

template <typename ... Args>
static void PrintLine(char const* const format, Args ... args) noexcept
{
    char buffer[1024] = {};
    ::sprintf_s(buffer, std::size(buffer), format, args ...);
#ifdef _DEBUG
    ::OutputDebugStringA(buffer);
    ::OutputDebugStringA("\n");
#else
    ::printf_s("%s\n", buffer);
#endif
}

static auto InitializeApp(DWORD flags = MFSTARTUP_LITE) noexcept
{
    wil::SetResultLoggingCallback([](wil::FailureInfo const& failure) noexcept
        {
            std::array<wchar_t, 2048> msg{};
            if (SUCCEEDED(wil::GetFailureLogString(msg.data(), msg.max_size(), failure)))
            {
#ifdef _DEBUG
                ::OutputDebugStringW(msg.data());
                ::OutputDebugStringW(L"\n");
#else
                ::printf_s("%s\n", buffer);
                std::fputws(msg.data(), stderr);
#endif
            }
        });

    winrt::init_apartment();
    WINRT_VERIFY_(S_OK, ::MFStartup(MF_VERSION, flags));
    return wil::scope_exit([]
        {
            WINRT_VERIFY_(S_OK, ::MFShutdown());
        });
}

static void WaitForQKeyPress()
{
    std::cout << "Press 'Q' to exit..." << std::endl;
    while (true)
    {
        char ch;
        std::cin >> ch;
        if (ch == 'Q' || ch == 'q')
        {
            break;
        }
    }
}
