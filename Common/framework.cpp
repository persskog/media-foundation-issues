#include "framework.hpp"
#include <iostream>

winrt::com_ptr<IMFActivate> SelectVideoDeviceActivate()
{
    winrt::com_ptr<IMFAttributes> attr;
    winrt::check_hresult(::MFCreateAttributes(attr.put(), 1));
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

    winrt::check_hresult(::MFEnumDeviceSources(attr.get(), &devices, &count));
    for (uint32_t i = 0; i < count; ++i)
    {
        wil::unique_cotaskmem_string name;
        uint32_t length{};
        winrt::check_hresult(devices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, name.put(), &length));
        Print("[%u] - %ws", i, name.get());
    }

    if (count > 0)
    {
        uint32_t selectedIndex{};
        std::cout << "Enter the device index: ";
        std::cin >> selectedIndex;
        if (selectedIndex < count)
        {
            devices[selectedIndex]->AddRef();
            return { devices[selectedIndex], winrt::take_ownership_from_abi };
        }
        else
        {
            Print("Invalid device index");
        }
    }
    else
    {
        Print("No video devices found");
    }
    return nullptr;
}

winrt::com_ptr<IMFSourceReader> CreateSourceReader(const winrt::com_ptr<IMFActivate>& activate,
                                                   const winrt::com_ptr<::IUnknown>& d3dManager)
{
    winrt::com_ptr<IMFAttributes> attr;
    winrt::check_hresult(::MFCreateAttributes(attr.put(), 5));
    WINRT_VERIFY_(S_OK, attr->SetString(MF_READWRITE_MMCSS_CLASS, L"Capture"));
    WINRT_VERIFY_(S_OK, attr->SetUINT32(MF_READWRITE_D3D_OPTIONAL, TRUE));
    WINRT_VERIFY_(S_OK, attr->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE));
    WINRT_VERIFY_(S_OK, attr->SetUINT32(MF_SOURCE_READER_DISABLE_CAMERA_PLUGINS, TRUE));
    if (d3dManager)
    {
        WINRT_VERIFY_(S_OK, attr->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, d3dManager.get()));
    }
    winrt::com_ptr<IMFMediaSource> source;
    winrt::check_hresult(activate->ActivateObject(winrt::guid_of<IMFMediaSource>(), source.put_void()));
    winrt::com_ptr<IMFSourceReader> reader;
    winrt::check_hresult(::MFCreateSourceReaderFromMediaSource(source.get(), attr.get(), reader.put()));
    WINRT_VERIFY_(S_OK, reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE));
    WINRT_VERIFY_(S_OK, reader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE));
    return reader;
}

std::wstring GetProcessDirectory()
{
    auto length = ::GetCurrentDirectoryW(0, nullptr);
    std::wstring dir(static_cast<size_t>(length + 1), L'?');
    ::GetCurrentDirectoryW(length, dir.data());
    return dir;
}
