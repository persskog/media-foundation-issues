#include "framework.hpp"
//#define CREATE_SINK_WRITER_FROM_PATH

static void PrintStreamCapabilities(const winrt::com_ptr<IMFByteStream>& stream)
{
    DWORD caps{};
    WINRT_VERIFY_(S_OK, stream->GetCapabilities(&caps));
    if (caps & MFBYTESTREAM_IS_READABLE)
        Print("MFBYTESTREAM_IS_READABLE");
    if (caps & MFBYTESTREAM_IS_WRITABLE)
        Print("MFBYTESTREAM_IS_WRITABLE");
    if (caps & MFBYTESTREAM_IS_SEEKABLE)
        Print("MFBYTESTREAM_IS_SEEKABLE");
    if (caps & MFBYTESTREAM_IS_REMOTE)
        Print("MFBYTESTREAM_IS_REMOTE");
    if (caps & MFBYTESTREAM_HAS_SLOW_SEEK)
        Print("MFBYTESTREAM_HAS_SLOW_SEEK");
    if (caps & MFBYTESTREAM_IS_DIRECTORY)
        Print("MFBYTESTREAM_IS_DIRECTORY");
    if (caps & MFBYTESTREAM_HAS_SLOW_SEEK)
        Print("MFBYTESTREAM_HAS_SLOW_SEEK");
    if (caps & MFBYTESTREAM_IS_PARTIALLY_DOWNLOADED)
        Print("MFBYTESTREAM_IS_PARTIALLY_DOWNLOADED");
    if (caps & MFBYTESTREAM_SHARE_WRITE)
        Print("MFBYTESTREAM_SHARE_WRITE");
    if (caps & MFBYTESTREAM_DOES_NOT_USE_NETWORK)
        Print("MFBYTESTREAM_DOES_NOT_USE_NETWORK");
}

static auto CreateOutputStream()
{
    const std::wstring dir = GetProcessDirectory();
    wchar_t buffer[MAX_PATH] = {};
    ::swprintf_s(buffer, std::size(buffer), L"%ws\\moov_before_mdat.mp4", dir.data());
    winrt::com_ptr<IMFByteStream> stream;
    winrt::check_hresult(::MFCreateFile(MF_ACCESSMODE_READWRITE, MF_OPENMODE_DELETE_IF_EXIST, MF_FILEFLAGS_ALLOW_WRITE_SHARING, buffer, stream.put()));
    Print("Created: %ws", buffer);
    PrintStreamCapabilities(stream);
    return stream;
}

static auto SetMoovBeforeMdat(const winrt::com_ptr<IMFAttributes>& attr, bool moovBeforeMdat)
{
    // The default behavior of the mpeg4 media sink is to write 'moov' after 'mdat' box.
    // Setting this attribute causes the generated file to write 'moov' before 'mdat' box.
    // In order for the mpeg4 sink to use this attribute, the byte stream passed in must not be slow seek or remote for.
    // This feature involves an additional file copying / remuxing.
    WINRT_VERIFY_(S_OK, attr->SetUINT32(MF_MPEG4SINK_MOOV_BEFORE_MDAT, moovBeforeMdat));
    Print("\nMF_MPEG4SINK_MOOV_BEFORE_MDAT = %u\n", static_cast<uint32_t>(moovBeforeMdat));
}

static auto CreateSinkWriter(const winrt::com_ptr<IMFAttributes>& attr)
{
    const std::wstring dir = GetProcessDirectory();
    wchar_t buffer[MAX_PATH] = {};
    ::swprintf_s(buffer, std::size(buffer), L"%ws\\moov_before_mdat.mp4", dir.data());
    winrt::com_ptr<IMFSinkWriter> writer;
    winrt::check_hresult(::MFCreateSinkWriterFromURL(buffer, nullptr, attr.get(), writer.put()));
    return writer;
}

static void WriteOutput(const winrt::com_ptr<IMFActivate>& activate, bool moovBeforeMdat)
{
    auto reader = CreateSourceReader(activate);

    /*********************************************
        Output format based upon the device type
    **********************************************/
    winrt::com_ptr<IMFMediaType> deviceType;
    winrt::check_hresult(reader->GetCurrentMediaType(0, deviceType.put()));
    winrt::com_ptr<IMFMediaType> h264;
    winrt::check_hresult(::MFCreateMediaType(h264.put()));
    WINRT_VERIFY_(S_OK, h264->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
    WINRT_VERIFY_(S_OK, h264->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264));
    WINRT_VERIFY_(S_OK, h264->SetUINT32(MF_MT_AVG_BITRATE, 1024 * 1024 * 5));
    WINRT_VERIFY_(S_OK, h264->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
    WINRT_VERIFY_(S_OK, h264->SetUINT32(MF_MT_VIDEO_PROFILE, 77)); // Main profile
    uint32_t num{}, den{};
    WINRT_VERIFY_(S_OK, ::MFGetAttributeSize(deviceType.get(), MF_MT_FRAME_SIZE, &num, &den));
    WINRT_VERIFY_(S_OK, ::MFSetAttributeSize(h264.get(), MF_MT_FRAME_SIZE, num, den));
    WINRT_VERIFY_(S_OK, ::MFGetAttributeRatio(deviceType.get(), MF_MT_FRAME_RATE, &num, &den));
    WINRT_VERIFY_(S_OK, ::MFSetAttributeRatio(h264.get(), MF_MT_FRAME_RATE, num, den));
    WINRT_VERIFY_(S_OK, ::MFSetAttributeRatio(h264.get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1));

    winrt::com_ptr<IMFAttributes> writerAttr;
    winrt::check_hresult(::MFCreateAttributes(writerAttr.put(), 5));
    WINRT_VERIFY_(S_OK, writerAttr->SetString(MF_READWRITE_MMCSS_CLASS, L"Capture"));
    WINRT_VERIFY_(S_OK, writerAttr->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE));
    WINRT_VERIFY_(S_OK, writerAttr->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, FALSE));
    WINRT_VERIFY_(S_OK, writerAttr->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE));

#ifdef CREATE_SINK_WRITER_FROM_PATH

    SetMoovBeforeMdat(writerAttr, moovBeforeMdat);
    auto writer = CreateSinkWriter(writerAttr);
    DWORD writerStream{};
    winrt::check_hresult(writer->AddStream(h264.get(), &writerStream));

#else
    auto stream = CreateOutputStream();

    /***********************************
        Mp4 sink/writer
    ***********************************/
    winrt::com_ptr<IMFMediaSink> sink;
    winrt::check_hresult(::MFCreateMPEG4MediaSink(stream.get(), h264.get(), nullptr, sink.put()));
    SetMoovBeforeMdat(sink.as<IMFAttributes>(), moovBeforeMdat);

    winrt::com_ptr<IMFSinkWriter> writer;
    winrt::check_hresult(::MFCreateSinkWriterFromMediaSink(sink.get(), writerAttr.get(), writer.put()));

#endif

    winrt::check_hresult(writer->SetInputMediaType(0, deviceType.get(), nullptr));
    winrt::check_hresult(writer->BeginWriting());
    //SetMoovBeforeMdat(sink.try_as<IMFAttributes>(), moovBeforeMdat);

    /*********************************
        Encode
    **********************************/
    int32_t framesToEncode = 30 * 1;
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
    Print("-- Begin finalizing");
    HRESULT hr = writer->Finalize();
    Print("<- Finalizing done (0x%08lx)", hr);
}

int main()
{
    auto ctx = InitializeApp();
    try
    {
        auto device = SelectVideoDeviceActivate();
        WriteOutput(device, true);
    }
    CATCH_LOG();
    return 0;
}

