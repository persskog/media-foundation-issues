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

template <typename ... Args>
static void Print(char const* const format, Args ... args) noexcept
{
    char buffer[1024] = {};
    ::sprintf_s(buffer, std::size(buffer), format, args ...);
    ::printf_s("%s\n", buffer);
}

/// <summary>
/// Initializes the application with the specified flags.
/// </summary>
/// <param name="flags">The flags to initialize the application with.</param>
/// <returns>A scope exit object that ensures proper shutdown of the application.</returns>
static auto InitializeApp(DWORD flags = MFSTARTUP_FULL) noexcept
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
    WINRT_VERIFY_(S_OK, ::MFStartup(MF_VERSION, flags));
    return wil::scope_exit([]
    {
        WINRT_VERIFY_(S_OK, ::MFShutdown());
    });
}

/// <summary>
/// Let's the user select the video device.
/// </summary>
/// <returns>The selected video device as an IMFActivate.</returns>
winrt::com_ptr<IMFActivate> SelectVideoDeviceActivate();

/// <summary>
/// Creates an IMFSourceReader with the specified IMFActivate and optional D3D manager.
/// </summary>
/// <param name="activate">The IMFActivate representing the video device.</param>
/// <param name="d3dManager">The optional D3D manager.</param>
/// <returns>The created IMFSourceReader.</returns>
winrt::com_ptr<IMFSourceReader> CreateSourceReader(const winrt::com_ptr<IMFActivate>& activate,
                                                   const winrt::com_ptr<::IUnknown>& d3dManager = {});

/// <summary>
/// Gets the directory of the current process.
/// </summary>
/// <returns>The directory of the current process as a wstring.</returns>
std::wstring GetProcessDirectory();
