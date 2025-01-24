#include "pch.h"
#include "video_encoder.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

static winrt::IAsyncAction TakePhotosAsync(winrt::com_ptr<VideoEncoder> encoder)
{
    auto cancel{ co_await winrt::get_cancellation_token() };
    co_await winrt::resume_background();
    while (!cancel())
    {
        co_await winrt::resume_after(2s);
        encoder->TakePhoto();
    }
    PrintLine("Done taking photos...");
}

static winrt::IAsyncAction RunEncoderAsync(winrt::com_ptr<VideoEncoder> encoder, winrt::TimeSpan duration)
{
    auto op = TakePhotosAsync(encoder);
    co_await winrt::resume_background();
    encoder->StartEncoder();
    co_await winrt::resume_after(duration);
    encoder->StopEncoder();
    op.Cancel();
}

static winrt::com_ptr<IMFActivate> ShowAvailableVideoDevices()
{
    winrt::com_ptr<IMFActivate> selected;
    winrt::com_ptr<IMFAttributes> attr;
    THROW_IF_FAILED(::MFCreateAttributes(attr.put(), 1));
    WINRT_VERIFY_(S_OK, attr->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));

    IMFActivate** devices{ nullptr };
    uint32_t count{};
    auto cleanup = wil::scope_exit([&]
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                devices[i]->Release();
            }
            ::CoTaskMemFree(devices);
        });

    THROW_IF_FAILED(::MFEnumDeviceSources(attr.get(), &devices, &count));
    for (uint32_t i = 0; i < count; ++i)
    {
        wil::unique_cotaskmem_string name;
        uint32_t length{};
        THROW_IF_FAILED(devices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, name.put(), &length));
        ::printf_s("[%u] - %ws\n", i, name.get());
    }

    if (0 == count)
    {
        ::printf_s("No video devices available\n");
        return {};
    }

    ::printf_s("Please select device to use\n");
    uint32_t index{};
    std::cin >> index;
    THROW_HR_IF(MF_E_INVALIDINDEX, index >= count);
    selected.copy_from(devices[index]);
    return selected;
}

int main()
{
    try
    {
        auto app{ InitializeApp() };
        auto d3ddevice = CreateD3D11Device();
        auto videoDevice = ShowAvailableVideoDevices();
        auto encoder = VideoEncoder::Create(videoDevice.get(), d3ddevice.get());
        RunEncoderAsync(encoder, 100s).get();
    }
    catch (...)
    {
        LOG_CAUGHT_EXCEPTION();
    }
    WaitForQKeyPress();
    return 0;
}
