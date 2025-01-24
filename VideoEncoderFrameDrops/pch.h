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
#include <mfreadwrite.h>
#include <mferror.h>
#include <icodecapi.h>
#include <codecapi.h>
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <wincodec.h>

#include <iostream>
#include "wil/resource.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Foundation.Collections.h"

#define OUTPUT_TO_DEBUG

namespace winrt {

    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Collections;

    template<>
    inline bool is_guid_of<IMFCaptureEngineOnSampleCallback2>(guid const& id) noexcept
    {
        return is_guid_of<IMFCaptureEngineOnSampleCallback2, IMFCaptureEngineOnSampleCallback>(id);
    }

}


template <typename ... Args>
static void PrintLine(char const* const format, Args ... args) noexcept
{
    char buffer[1024] = {};
    ::sprintf_s(buffer, std::size(buffer), format, args ...);
#ifdef OUTPUT_TO_DEBUG
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
#ifdef OUTPUT_TO_DEBUG
                ::OutputDebugStringW(msg.data());
                ::OutputDebugStringW(L"\n");
#else
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

static constexpr uint32_t GetDX11DeviceCreationFlags() noexcept
{
#ifdef _DEBUG
    // Use D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS to request
    // that the runtime and video driver not create any additional threads that
    // might interfere with the application.
    // Does this have any effect on performance? 
    // Sometimes Microsoft uses this flags, sometimes they don't...
    return D3D11_CREATE_DEVICE_BGRA_SUPPORT |
           D3D11_CREATE_DEVICE_VIDEO_SUPPORT |
           D3D11_CREATE_DEVICE_DEBUG |
           D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS;
#else
    return D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT;
#endif
}

static void ConfigureDX11DebugLayer(ID3D11Device* device, ID3D11DeviceContext* ctx) noexcept
{
    winrt::com_ptr<ID3D11Debug> debug;
    winrt::com_ptr<ID3D11InfoQueue> iq;
    device->QueryInterface(debug.put());
    if (!debug || !debug.try_as(iq))
    {
        return;
    }

    D3D11_MESSAGE_ID hide[] =
    {
        // The media foundation teams generates
        // these quite often so filter them out
        D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
        D3D11_MESSAGE_ID_CREATETEXTURE2D_INVALIDARG_RETURN,
        D3D11_MESSAGE_ID_CREATETEXTURE2D_UNSUPPORTEDFORMAT,
        D3D11_MESSAGE_ID_CREATETEXTURE2D_INVALIDDIMENSIONS,
        D3D11_MESSAGE_ID_CREATETEXTURE2D_INVALIDMIPLEVELS,
        D3D11_MESSAGE_ID_CREATETEXTURE2D_INVALIDBINDFLAGS,
        D3D11_MESSAGE_ID_OMSETRENDERTARGETS_UNBINDDELETINGOBJECT,
        D3D11_MESSAGE_ID_GETVIDEOPROCESSORFILTERRANGE_UNSUPPORTED,
        D3D11_MESSAGE_ID_CREATEVIDEODECODER_INVALIDFORMAT
    };

    D3D11_INFO_QUEUE_FILTER filter{};
    filter.DenyList.NumIDs = static_cast<uint32_t>(std::size(hide));
    filter.DenyList.pIDList = hide;
    WINRT_VERIFY_(S_OK, iq->AddStorageFilterEntries(&filter));
    WINRT_VERIFY_(S_OK, iq->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, FALSE));
    WINRT_VERIFY_(S_OK, iq->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE));
    WINRT_VERIFY_(S_OK, iq->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, FALSE));
    WINRT_VERIFY_(S_OK, iq->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_INFO, FALSE));
    WINRT_VERIFY_(S_OK, iq->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_MESSAGE, FALSE));
}

static winrt::com_ptr<ID3D11Device> CreateD3D11Device()
{
    constexpr D3D_FEATURE_LEVEL levels[2u] = { D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1 };
    constexpr auto flags = GetDX11DeviceCreationFlags();

    winrt::com_ptr<ID3D11DeviceContext> ctx;
    winrt::com_ptr<ID3D11Device> device;
    D3D_FEATURE_LEVEL actual{};

    THROW_IF_FAILED(::D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        levels,
        2u,
        D3D11_SDK_VERSION,
        device.put(),
        &actual,
        ctx.put()));

    // This is required for the Media Foundation Dx11 pipeline
    ctx.as<ID3D11Multithread>()->SetMultithreadProtected(1);
    ConfigureDX11DebugLayer(device.get(), ctx.get());
    return device;
}

