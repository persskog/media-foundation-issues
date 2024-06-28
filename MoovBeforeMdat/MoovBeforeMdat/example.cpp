#include "pch.h"
#include "example.hpp"

/*
    Internal
*/

static std::wstring GetProcessDirectory()
{
    unsigned long length = ::GetCurrentDirectoryW(0, nullptr);
    std::wstring dir(static_cast<size_t>(length + 1), L'?');
    ::GetCurrentDirectoryW(length, dir.data());
    return dir;
}

static auto CreateOutputStream()
{
    const std::wstring dir = GetProcessDirectory();
    wchar_t buffer[MAX_PATH] = {};
    ::swprintf_s(buffer, std::size(buffer), L"%ws\\moov_before_mdat.mp4", dir.data());
    winrt::com_ptr<IMFByteStream> stream;
    winrt::check_hresult(::MFCreateFile(MF_ACCESSMODE_READWRITE, MF_OPENMODE_DELETE_IF_EXIST, MF_FILEFLAGS_NONE, buffer, stream.put()));
    Print("Created: %ws", buffer);
    return stream;
}


/*
    Public
*/

winrt::com_ptr<IMFSourceReader> SelectVideoDevice()
{
    winrt::com_ptr<IMFAttributes> attr;
    winrt::check_hresult(::MFCreateAttributes(attr.put(), 5));
    WINRT_VERIFY_(S_OK, attr->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));
    WINRT_VERIFY_(S_OK, attr->SetString(MF_READWRITE_MMCSS_CLASS, L"Capture"));
    WINRT_VERIFY_(S_OK, attr->SetUINT32(MF_READWRITE_D3D_OPTIONAL, TRUE));
    WINRT_VERIFY_(S_OK, attr->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE));

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
    winrt::com_ptr<IMFActivate> videoDevice;
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
            winrt::com_ptr<IMFMediaSource> source;
            winrt::check_hresult(devices[selectedIndex]->ActivateObject(winrt::guid_of<IMFMediaSource>(), source.put_void()));
            winrt::com_ptr<IMFSourceReader> reader;
            winrt::check_hresult(::MFCreateSourceReaderFromMediaSource(source.get(), attr.get(), reader.put()));
            WINRT_VERIFY_(S_OK, reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE));
            WINRT_VERIFY_(S_OK, reader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE));
            return reader;
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

void WriteOutput(const winrt::com_ptr<IMFSourceReader>& reader, bool moovBeforeMdat)
{
    auto stream = CreateOutputStream();

    /******************************
        Print stream capabilities
    *******************************/ 
    DWORD caps{};
    WINRT_VERIFY_(S_OK, stream->GetCapabilities(&caps)); 
    PrintIf(caps & MFBYTESTREAM_IS_READABLE, "MFBYTESTREAM_IS_READABLE");
    PrintIf(caps & MFBYTESTREAM_IS_WRITABLE, "MFBYTESTREAM_IS_WRITABLE");
    PrintIf(caps & MFBYTESTREAM_IS_SEEKABLE, "MFBYTESTREAM_IS_SEEKABLE");
    PrintIf(caps & MFBYTESTREAM_IS_REMOTE,   "MFBYTESTREAM_IS_REMOTE");
    PrintIf(caps & MFBYTESTREAM_HAS_SLOW_SEEK, "MFBYTESTREAM_HAS_SLOW_SEEK");
    PrintIf(caps & MFBYTESTREAM_IS_DIRECTORY, "MFBYTESTREAM_IS_DIRECTORY");
    PrintIf(caps & MFBYTESTREAM_HAS_SLOW_SEEK, "MFBYTESTREAM_HAS_SLOW_SEEK");
    PrintIf(caps & MFBYTESTREAM_IS_PARTIALLY_DOWNLOADED, "MFBYTESTREAM_IS_PARTIALLY_DOWNLOADED");
    PrintIf(caps & MFBYTESTREAM_SHARE_WRITE, "MFBYTESTREAM_SHARE_WRITE");
    PrintIf(caps & MFBYTESTREAM_DOES_NOT_USE_NETWORK, "MFBYTESTREAM_DOES_NOT_USE_NETWORK");

    /*********************************************
        Output format based upon the device type
    **********************************************/ 
    winrt::com_ptr<IMFMediaType> deviceType;
    winrt::check_hresult(reader->GetCurrentMediaType(0, deviceType.put()));
    winrt::com_ptr<IMFMediaType> h264;
    winrt::check_hresult(::MFCreateMediaType(h264.put()));
    WINRT_VERIFY_(S_OK, h264->SetGUID(MF_MT_MAJOR_TYPE,        MFMediaType_Video));
    WINRT_VERIFY_(S_OK, h264->SetGUID(MF_MT_SUBTYPE,           MFVideoFormat_H264));
    WINRT_VERIFY_(S_OK, h264->SetUINT32(MF_MT_AVG_BITRATE,     1024 * 1024 * 5));
    WINRT_VERIFY_(S_OK, h264->SetUINT32(MF_MT_INTERLACE_MODE , MFVideoInterlace_Progressive));
    WINRT_VERIFY_(S_OK, h264->SetUINT32(MF_MT_VIDEO_PROFILE,   77)); // Main profile
    uint32_t num{}, den{};
    WINRT_VERIFY_(S_OK, ::MFGetAttributeSize(deviceType.get(),  MF_MT_FRAME_SIZE, &num, &den));
    WINRT_VERIFY_(S_OK, ::MFSetAttributeSize(h264.get(),        MF_MT_FRAME_SIZE, num,   den));
    WINRT_VERIFY_(S_OK, ::MFGetAttributeRatio(deviceType.get(), MF_MT_FRAME_RATE, &num, &den));
    WINRT_VERIFY_(S_OK, ::MFSetAttributeRatio(h264.get(),       MF_MT_FRAME_RATE, num,   den));
    WINRT_VERIFY_(S_OK, ::MFSetAttributeRatio(h264.get(),       MF_MT_PIXEL_ASPECT_RATIO, 1, 1));

    /***********************************
        Mp4 sink/writer
    ***********************************/ 
    winrt::com_ptr<IMFMediaSink> sink;
    winrt::check_hresult(::MFCreateMPEG4MediaSink(stream.get(), h264.get(), nullptr, sink.put()));
    winrt::com_ptr<IMFAttributes> attr;
    if (sink.try_as(attr))
    {
        Print("\nMF_MPEG4SINK_MOOV_BEFORE_MDAT = %u\n", static_cast<uint32_t>(moovBeforeMdat));
        WINRT_VERIFY_(S_OK, attr->SetUINT32(MF_MPEG4SINK_MOOV_BEFORE_MDAT, moovBeforeMdat));
    }
    winrt::com_ptr<IMFSinkWriter> writer;
    winrt::check_hresult(::MFCreateSinkWriterFromMediaSink(sink.get(), nullptr, writer.put()));
    winrt::check_hresult(writer->SetInputMediaType(0, deviceType.get(), nullptr));
    winrt::check_hresult(writer->BeginWriting());

    /*********************************
        Encode
    **********************************/
    int32_t framesToEncode = 30 * 4;
    int64_t time{};
    uint64_t avgTimePerFrame{};
    WINRT_VERIFY_(S_OK, ::MFGetAttributeSize(deviceType.get(), MF_MT_FRAME_RATE, &num, &den));
    WINRT_VERIFY_(S_OK, ::MFFrameRateToAverageTimePerFrame(num, den, &avgTimePerFrame));

    Print("-> Begin reading device frames and encode...");
    while (framesToEncode > 0)
    {
        winrt::com_ptr<IMFSample> sample;
        DWORD flags{};
        int64_t timestamp{};
        HRESULT hr = reader->ReadSample(0, 0, nullptr, &flags, &timestamp, sample.put());
        if (flags & MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED)
        {
            Print("- MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED");
        }
        else if (flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
        {
            Print("- MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED");
        }

        if (sample)
        {
            int64_t duration{};
            WINRT_VERIFY_(S_OK, sample->GetSampleDuration(&duration));
            if (duration == 0)
            {
                // Some devices I tested may not provide the duration
                duration = avgTimePerFrame;
            }
            WINRT_VERIFY_(S_OK, sample->SetSampleTime(time));
            winrt::check_hresult(writer->WriteSample(0, sample.get()));
            time += duration;
            --framesToEncode;
            printf_s("-- %u frames    \r", framesToEncode);
        }
        else
        {
            if (flags & MF_SOURCE_READERF_ERROR)
            {
                Print("-- MF_SOURCE_READERF_ERROR (0x%08lx)", hr);
                THROW_HR(hr);
            }
            else if (flags & MF_SOURCE_READERF_STREAMTICK)
            {
                Print("-- MF_SOURCE_READERF_STREAMTICK (%lld)", timestamp);
                winrt::check_hresult(writer->SendStreamTick(0, time));
            }
            else
            {
                Print("-- Unhandled flags (%u)", flags);
            }
        }
    }
    winrt::check_hresult(writer->Finalize());
    Print("<- Encoding done");
}
