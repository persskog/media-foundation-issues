#pragma once
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mf")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "avrt")
#pragma comment(lib, "dcomp")
#ifdef _DEBUG
#pragma comment(lib, "dxguid")
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WINRT_LEAN_AND_MEAN
#define WINRT_LEAN_AND_MEAN
#endif
#define WINRT_NO_SOURCE_LOCATION

#include <mfidl.h>
#include <mfapi.h>
#include <mfcaptureengine.h>
#include <mfreadwrite.h>
#include <mftransform.h>
#include <mferror.h>
#include <codecapi.h>

#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <dxgicommon.h>

#include "wil/cppwinrt.h"
#include "wil/resource.h"

// C++ /WinRT headers
#pragma push_macro("GetCurrentTime")
#pragma push_macro("X64")
#undef GetCurrentTime
#undef X64
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Foundation.Collections.h"
#include "winrt/Windows.System.h"
#pragma pop_macro("X64")
#pragma pop_macro("GetCurrentTime")

#include <dispatcherqueue.h>
#include <cstdio>

namespace winrt {
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Collections;
    using namespace Windows::System;
}

/// <summary>
/// Prints formatted output to the console.
/// </summary>
template <typename ... Args>
static void PrintLine(char const* const format, Args ... args) noexcept
{
    char buffer[1024] = {};
    ::sprintf_s(buffer, std::size(buffer), format, args ...);
    ::printf_s("%s\n", buffer);
}



static auto Startup()
{
    winrt::check_hresult(::MFStartup(MF_VERSION, MFSTARTUP_LITE));
    return wil::scope_exit([]
        {
            WINRT_VERIFY_(S_OK, ::MFUnlockDXGIDeviceManager());
            WINRT_VERIFY_(S_OK, ::MFShutdown());
        });
}

inline winrt::Windows::System::DispatcherQueueController CreateDispatcherQueueControllerOnCurrentThread()
{
    const DispatcherQueueOptions options
    {
        sizeof(DispatcherQueueOptions),
        DQTYPE_THREAD_CURRENT,
        DQTAT_COM_STA
    };
    ABI::Windows::System::IDispatcherQueueController* ptr{};
    winrt::check_hresult(::CreateDispatcherQueueController(options, &ptr));
    return { ptr, winrt::take_ownership_from_abi };
}

static winrt::com_ptr<IMFActivate> SelectVideoDeviceActivate(IMFAttributes* attr)
{
    attr->DeleteAllItems();
    WINRT_VERIFY_(S_OK, attr->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));

    IMFActivate** devices{ nullptr };
    uint32_t count{};
    auto cleanup = wil::scope_exit([=]
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                devices[i]->Release();
            }
            ::CoTaskMemFree(devices);
        });

    ::MFEnumDeviceSources(attr, &devices, &count);
    for (uint32_t i = 0; i < count; ++i)
    {
        uint32_t length{};
        wchar_t* name{ nullptr };
        devices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &name, &length);
        if (name)
        {
            PrintLine("[%u] - %ws", i, name);
            ::CoTaskMemFree(name);
        }
    }

    THROW_HR_IF(MF_E_NO_CAPTURE_DEVICES_AVAILABLE, count == 0);

    uint32_t si{};
    ::printf_s("Enter the device index: ");
    ::scanf_s("%u", &si);
    if (si < count)
    {
        devices[si]->AddRef();
        return { devices[si], winrt::take_ownership_from_abi };
    }
    else
    {
        PrintLine("Invalid device index");
    }
    return nullptr;
}


static auto CreateMFDXGIDeviceManager()
{
    constexpr D3D_FEATURE_LEVEL FeatureLevels[3] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1
    };

#ifdef _DEBUG
    constexpr auto Flags =
        D3D11_CREATE_DEVICE_BGRA_SUPPORT |
        /* D3D11_CREATE_DEVICE_VIDEO_SUPPORT | */
        D3D11_CREATE_DEVICE_DEBUG |
        D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS;
#else
    constexpr auto Flags =
        D3D11_CREATE_DEVICE_BGRA_SUPPORT |
        D3D11_CREATE_DEVICE_VIDEO_SUPPORT |
        D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS;
#endif

    winrt::com_ptr<ID3D11DeviceContext> ctx;
    winrt::com_ptr<ID3D11Device> dev;
    D3D_FEATURE_LEVEL actual{};
    winrt::check_hresult(::D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        Flags,
        FeatureLevels,
        3,
        D3D11_SDK_VERSION,
        dev.put(),
        &actual,
        ctx.put()));

    // Enable multithread protection, required for Media Foundation
    ctx.as<ID3D11Multithread>()->SetMultithreadProtected(TRUE);

    winrt::com_ptr<IMFDXGIDeviceManager> dxgiDeviceManager;
    uint32_t token{};
    winrt::check_hresult(::MFLockDXGIDeviceManager(&token, dxgiDeviceManager.put()));
    winrt::check_hresult(dxgiDeviceManager->ResetDevice(dev.get(), token));
    PrintLine("Created DXGI Device Manager with D3D11 Feature Level 0x%X", actual);
    return dxgiDeviceManager;
}

static auto CreateCaptureEngine()
{
    winrt::com_ptr<IMFCaptureEngine> engine;
    auto factory = winrt::create_instance<IMFCaptureEngineClassFactory>(CLSID_MFCaptureEngineClassFactory);
    winrt::check_hresult(factory->CreateInstance(CLSID_MFCaptureEngine, __uuidof(IMFCaptureEngine), engine.put_void()));
    return engine;
}

static auto CopyAttribute(const GUID&               key,
                   IMFAttributes* __restrict src,
                   IMFAttributes* __restrict dst) noexcept
{
    PROPVARIANT value{};
    auto cleanup = wil::scope_exit([&] { ::PropVariantClear(&value); });
    if (SUCCEEDED(src->GetItem(key, &value)))
    {
        WINRT_VERIFY_(S_OK, dst->SetItem(key, value));
        return true;
    }
    // Source did not contain the requested attribute
    return false;
}
