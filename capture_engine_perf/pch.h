#pragma once
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "mf")
#pragma comment(lib, "d3d11")
#ifdef _DEBUG
#pragma comment(lib, "dxguid")
#endif
#include <cstdint>
#include <conio.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mferror.h>
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <mfcaptureengine.h>
#include <DispatcherQueue.h>
#include <chrono>

// WinRT headers
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Foundation.Collections.h"
#include "winrt/Windows.System.h"



/*
    Some helper macros for logging.
*/

#define PRINTLN(fmt, ...) do { \
    std::printf(fmt "\n", __VA_ARGS__); \
} while ((void)0, 0)

#define PRINT(fmt, ...) do { \
    std::printf(fmt "\r", __VA_ARGS__); \
    std::fflush(stdout); \
} while ((void)0, 0)

#define PRINTLN_ERROR(fmt, ...) do { \
    std::printf("%s(%d): " fmt "\n", __FILE__, __LINE__, __VA_ARGS__); \
} while ((void)0, 0)

#define LOG_ERROR() do { \
    PRINTLN_ERROR("%ws (0x%08lx)", winrt::to_message().data(), winrt::to_hresult().value);\
} while ((void)0, 0)


using namespace std::chrono;
using namespace std::chrono_literals;

namespace winrt {

    using namespace Windows::Foundation;
    using namespace Windows::System;

    static DispatcherQueueController CreateDispatcherQueueController()
    {
        DispatcherQueueOptions options
        {
            sizeof(DispatcherQueueOptions),
            DQTYPE_THREAD_CURRENT,
            DQTAT_COM_STA
        };

        ABI::Windows::System::IDispatcherQueueController* ptr{};
        check_hresult(CreateDispatcherQueueController(options, &ptr));
        return { ptr, take_ownership_from_abi };
    }

    template<> inline bool is_guid_of<IMFCaptureEngineOnSampleCallback2>(guid const& id) noexcept
    {
        return is_guid_of<IMFCaptureEngineOnSampleCallback2, IMFCaptureEngineOnSampleCallback>(id);
    }
}
