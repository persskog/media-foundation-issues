#include "pch.h"
#include "capture_engine.h"

using namespace winrt;

/*
    Internal
*/

class StopWatch
{
    winrt::Windows::Foundation::TimeSpan m_begin{};

public:
    StopWatch()
    {
        Restart();
    }

    void Restart() noexcept
    {
        m_begin = GetTimestamp();
    }

    inline static double ToSeconds(int64_t ts) noexcept
    {
        return ts / 10000000.0;
    }

    inline static double ToSeconds(winrt::Windows::Foundation::TimeSpan ts) noexcept
    {
        return ToSeconds(ts.count());
    }

    inline static double ToMilliseconds(int64_t ts) noexcept
    {
        return ts / 10000.0;
    }

    inline static double ToMilliseconds(winrt::Windows::Foundation::TimeSpan ts) noexcept
    {
        return ToMilliseconds(ts.count());
    }

    inline double ElapsedSeconds() const noexcept
    {
        return ToSeconds(Elapsed());
    }

    inline double ElapsedMilliseconds() const noexcept
    {
        return ToMilliseconds(Elapsed());
    }

    inline winrt::Windows::Foundation::TimeSpan Elapsed() const noexcept
    {
        return GetTimestamp() - m_begin;
    }

    static winrt::Windows::Foundation::TimeSpan GetTimestamp() noexcept
    {
        return winrt::Windows::Foundation::TimeSpan(::MFGetSystemTime());
    }
};


static void PrintMediaType(IMFMediaType* type)
{
    uint32_t bps{};
    uint32_t rate{};
    uint32_t channels{};
    type->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bps);
    type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate);
    type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);
    GUID format{};
    type->GetGUID(MF_MT_SUBTYPE, &format);
    if (MFAudioFormat_Float == format)
    {
        PrintLine("- (float) %u-bit, %u channels, %uHz", bps, channels, rate);
    }
    else if (MFAudioFormat_PCM == format)
    {
        PrintLine("- (pcm) %u-bit, %u channels, %uHz", bps, channels, rate);
    }
    else
    {
        PrintLine("- (unknown) %u-bit, %u channels, %uHz", bps, channels, rate);
    }
}

static com_ptr<IMFSinkWriter> CreateWriter(IMFMediaType* audioInputType, const CaptureParams& params)
{
    winrt::com_ptr<IMFAttributes> attr;
    THROW_IF_FAILED(::MFCreateAttributes(attr.put(), 7));
    THROW_IF_FAILED(attr->SetUINT32(MF_LOW_LATENCY, 1));

    // Disabling this in a realtime scenarion should not be a problem
    PrintLine("- MF_SINK_WRITER_DISABLE_THROTTLING (%d)", params.DisableThrottling);
    THROW_IF_FAILED(attr->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, params.DisableThrottling));

    if (params.Fragmented)
    {
        PrintLine("- MFTranscodeContainerType_FMPEG4");
        THROW_IF_FAILED(attr->SetGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_FMPEG4));
    }
    else
    {
        PrintLine("- MFTranscodeContainerType_MPEG4");
        THROW_IF_FAILED(attr->SetGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_MPEG4));
    }

    // Audio processing frames have a bit higher priom to avoid, audio glitches and other artifacts
    THROW_IF_FAILED(attr->SetString(MF_READWRITE_MMCSS_CLASS, L"Capture"));
    THROW_IF_FAILED(attr->SetString(MF_READWRITE_MMCSS_CLASS_AUDIO, L"Audio"));
    THROW_IF_FAILED(attr->SetUINT32(MF_READWRITE_MMCSS_PRIORITY, 0));
    THROW_IF_FAILED(attr->SetUINT32(MF_READWRITE_MMCSS_PRIORITY_AUDIO, 0));

    com_ptr<IMFSinkWriter> writer;
    THROW_IF_FAILED(::MFCreateSinkWriterFromURL(L"output.mp4", nullptr, attr.get(), writer.put()));

    uint32_t channels{};
    audioInputType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);

    com_ptr<IMFMediaType> aac;
    THROW_IF_FAILED(::MFCreateMediaType(aac.put()));
    aac->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    aac->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC);
    aac->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16u);
    aac->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, params.AacSamplePerSecond);
    aac->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, channels);
    aac->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, params.AacAvgBytesPerSecond);
    PrintLine("- Encoding to: %u channel, 16-bit, %u Hz, %u bytes/s (MFAudioFormat_AAC)",
        channels,
        params.AacSamplePerSecond, 
        params.AacAvgBytesPerSecond);

    DWORD stream{};
    THROW_IF_FAILED(writer->AddStream(aac.get(), &stream));
    THROW_IF_FAILED(writer->SetInputMediaType(stream, audioInputType, nullptr));
    THROW_IF_FAILED(writer->BeginWriting());
    return writer;
}

//static com_ptr<IMFMediaType> CreateAacCompatibleInput(IMFMediaType* audioInputType)
//{
//    com_ptr<IMFMediaType> pcm;
//    THROW_IF_FAILED(::MFCreateMediaType(pcm.put()));
//    pcm->SetGUID(MF_MT_MAJOR_TYPE,              MFMediaType_Audio);
//    pcm->SetGUID(MF_MT_SUBTYPE,                 MFAudioFormat_PCM);
//    pcm->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,               16u);
//
//    uint32_t rate{};
//    audioInputType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate);
//    pcm->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, rate);
//
//    uint32_t channels{};
//    audioInputType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);
//    pcm->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, channels);
//
//    uint32_t block = (channels * 16u) / 8u;
//    pcm->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, block);
//
//    return pcm;
//}


/*
    CaptureEngine
*/

com_ptr<IMFActivate> CaptureEngine::ShowAvailableAudioDevices()
{
    com_ptr<IMFActivate> selected;

    com_ptr<IMFAttributes> attr;
    THROW_IF_FAILED(::MFCreateAttributes(attr.put(), 1));
    WINRT_VERIFY_(S_OK, attr->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID));

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
        PrintLine("[%u] - %ws", i, name.get());
    }
    PrintLine("Please select device to use");
    uint32_t audioDeviceIndex{};
    std::cin >> audioDeviceIndex;
    THROW_HR_IF(MF_E_INVALIDINDEX, audioDeviceIndex >= count);
    selected.copy_from(devices[audioDeviceIndex]);
    return selected;
}

void CaptureEngine::Run(const com_ptr<IMFActivate>& device, const CaptureParams& params)
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    wil::unique_cotaskmem_string name;
    uint32_t length{};
    THROW_IF_FAILED(device->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, name.put(), &length));
    PrintLine("\nInitializing: %ws", name.get());

    com_ptr<IMFMediaSource> source;
    THROW_IF_FAILED(device->ActivateObject(guid_of<IMFMediaSource>(), source.put_void()));

    com_ptr<IMFSourceReader> reader;
    THROW_IF_FAILED(::MFCreateSourceReaderFromMediaSource(source.get(), nullptr, reader.put()));

    PrintLine("- Cuurent type:");
    com_ptr<IMFMediaType> currentType;
    THROW_IF_FAILED(reader->GetCurrentMediaType(0, currentType.put()));
    PrintMediaType(currentType.get());

    PrintLine("- Native type:");
    com_ptr<IMFMediaType> nativeType;
    DWORD nativeIndex{};
    auto rex = reader.as<IMFSourceReaderEx>();
    while (SUCCEEDED(rex->GetNativeMediaType(0, nativeIndex, nativeType.put())))
    {
        PrintMediaType(nativeType.get());
        break;
    }

    //auto compat = CreateAacCompatibleInput(currentType.get());
    auto writer = CreateWriter(nativeType.get(), params);
    constexpr auto MaxDuration = duration_cast<winrt::Windows::Foundation::TimeSpan>(5 * 60s);

    HRESULT hr{};
    winrt::Windows::Foundation::TimeSpan totalDuration{};
    int64_t st{};
    uint32_t samplesReceived{};

    PrintLine("Start capturing audio for %llds", duration_cast<seconds>(MaxDuration).count());

    StopWatch watch;
    while (SUCCEEDED(hr) && watch.Elapsed() <= MaxDuration)
    {
        com_ptr<IMFSample> sample;
        DWORD flags{};
        int64_t timestamp{};
        hr = reader->ReadSample(0, 0, nullptr, &flags, &timestamp, sample.put());
        if (sample)
        {
            int64_t duration{};
            sample->GetSampleDuration(&duration);
            totalDuration += winrt::Windows::Foundation::TimeSpan(duration);
            if ((++samplesReceived % 48) == 0)
            {
                auto clockElapsed = watch.Elapsed();
                const auto diff = clockElapsed - totalDuration;

                PrintLine("clk %lfs - cap %lfs (%lldms)", 
                    StopWatch::ToSeconds(clockElapsed),
                    StopWatch::ToSeconds(totalDuration),
                    duration_cast<milliseconds>(clockElapsed - totalDuration).count());
            }

            if (st == 0)
            {
                writer->SendStreamTick(0, 0);
            }

            sample->SetSampleTime(st);
            st += duration;
            hr = writer->WriteSample(0, sample.get());
        }
        else
        {
            if (SUCCEEDED(hr))
            {
                if (MF_SOURCE_READERF_STREAMTICK & flags)
                {
                    PrintLine("* MF_SOURCE_READERF_STREAMTICK");
                    watch.Restart();
                }
                else if (MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED & flags)
                {
                    PrintLine("* MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED");
                }
                else if (MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED & flags)
                {
                    PrintLine("* MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED");
                }
            }
        }
    }
    THROW_IF_FAILED(hr);

    PrintLine("Accumulated samples %lld (%lfs), elapsed clock %lfs",
        totalDuration, 
        StopWatch::ToSeconds(totalDuration),
        watch.ElapsedSeconds());

    THROW_IF_FAILED(writer->Finalize());
    MF_SINK_WRITER_STATISTICS stats{ .cb = sizeof(MF_SINK_WRITER_STATISTICS) };
    THROW_IF_FAILED(writer->GetStatistics(0, &stats));
    PrintLine(
        "Sink rcvd=%lld (%lfs), proc=%lld (%lfs), enc=%lld (%lfs)",
        stats.llLastTimestampReceived,
        StopWatch::ToSeconds(stats.llLastTimestampReceived),
        stats.llLastTimestampProcessed,
        StopWatch::ToSeconds(stats.llLastTimestampProcessed),
        stats.llLastTimestampEncoded,
        StopWatch::ToSeconds(stats.llLastTimestampEncoded)
    );
}
