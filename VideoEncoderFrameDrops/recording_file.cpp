#include "pch.h"
#include "recording_file.hpp"

RecordingFile::RecordingFile(IMFSinkWriter* writer, DWORD workQueue, DWORD audioStream, DWORD videoStream, std::wstring_view filePath)
    : m_workQueue{ workQueue }
    , m_audioStream{ audioStream }
    , m_videoStream{ videoStream }
    , m_filePath{ filePath }
{
    m_writer.copy_from(writer);
}

RecordingFile::~RecordingFile()
{
    WINRT_VERIFY_(S_OK, ::MFUnlockWorkQueue(m_workQueue));
}

HRESULT RecordingFile::Create(std::wstring_view filePath, IMFMediaType* videoType, IMFMediaType* audioType, RecordingFile** file)
{
    if (filePath.empty() || !videoType)
    {
        return E_INVALIDARG;
    }
    winrt::com_ptr<IMFAttributes> attr;
    RETURN_IF_FAILED(::MFCreateAttributes(attr.put(), 5));
    attr->SetUINT32(MF_LOW_LATENCY, TRUE);
    attr->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE);
    attr->SetGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_FMPEG4);
    attr->SetString(MF_READWRITE_MMCSS_CLASS, L"Capture");
    attr->SetString(MF_READWRITE_MMCSS_CLASS_AUDIO, L"Audio");

    winrt::com_ptr<IMFSinkWriter> writer;
    DWORD audioStream{ InvalidStream };
    DWORD videoStream{ InvalidStream };
    RETURN_IF_FAILED(::MFCreateSinkWriterFromURL(filePath.data(), nullptr, attr.get(), writer.put()));
    if (audioType)
    {
        winrt::com_ptr<IMFMediaType> aac;
        RETURN_IF_FAILED(::MFCreateMediaType(aac.put()));
        aac->SetGUID(MF_MT_MAJOR_TYPE,        MFMediaType_Audio);
        aac->SetGUID(MF_MT_SUBTYPE,           MFAudioFormat_AAC);
        aac->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,         16u);
        aac->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 24000u);
        aac->SetUINT32(MF_MT_AAC_PAYLOAD_TYPE,              0x0);
        CopyAttribute(MF_MT_AUDIO_NUM_CHANNELS, audioType, aac.get());
        CopyAttribute(MF_MT_AUDIO_SAMPLES_PER_SECOND, audioType, aac.get());
        RETURN_IF_FAILED(writer->AddStream(aac.get(), &audioStream));
        RETURN_IF_FAILED(writer->SetInputMediaType(audioStream, audioType, nullptr));
    }
    RETURN_IF_FAILED(writer->AddStream(videoType, &videoStream));
    DWORD workQueue{};
    RETURN_IF_FAILED(::MFAllocateSerialWorkQueue(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, &workQueue));
    // Create the new instance and do the final preparations
    auto instance = winrt::make_self<RecordingFile>(writer.get(), workQueue, audioStream, videoStream, filePath);
    RETURN_IF_FAILED(instance->Prepare(videoType));
    *file = instance.detach();
    return S_OK;
}

HRESULT __stdcall RecordingFile::GetParameters(DWORD* flags, DWORD* queue) noexcept
{
    *flags = 0;
    *queue = m_workQueue;
    return S_OK;
}

HRESULT __stdcall RecordingFile::Invoke(IMFAsyncResult* result) noexcept
{
    WINRT_ASSERT(m_writer);
    auto state = result->GetStateNoAddRef();
    if (!state)
    {
        return S_OK;
    }

    try
    {
        winrt::com_ptr<IMFSample> sample;
        if (SUCCEEDED(state->QueryInterface(sample.put())))
        {
            uint32_t isAudio{};
            sample->GetUINT32(VidiView_AudioSample, &isAudio);
            if (isAudio)
            {
                if (HasAudioVideoSyncPoint())
                {
                    WriteAudio(sample.get());
                }
            }
            else
            {
                WriteVideo(sample.get());
            }
        }
    }
    catch (...)
    {
        LOG_CAUGHT_EXCEPTION();
    }
    return S_OK;
}

HRESULT RecordingFile::Prepare(IMFMediaType* videoType)
{
    WINRT_ASSERT(m_writer);
    // After this, the file is ready to accept video and audio data
    m_videoAnalyzer.Initialize(videoType);
    return m_writer->BeginWriting();
}

HRESULT RecordingFile::Finalize() const
{
    WINRT_ASSERT(m_writer);
    if (HRESULT hr = m_writer->Finalize(); FAILED(hr))
    {
        return hr;
    }

    winrt::file_handle file{ OpenFile(m_filePath) };
    if (!file)
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    const FILETIME filetime = winrt::clock::to_FILETIME(m_acquisitionTime);
    if (!::SetFileTime(file.get(), &filetime, nullptr, nullptr))
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }
    return S_OK;
}

void RecordingFile::Close(MF_SINK_WRITER_STATISTICS* videoStats,
                          MF_SINK_WRITER_STATISTICS* audioStats)
{
    WINRT_ASSERT(m_writer);
    if (videoStats)
    {
        GetStatistics(m_videoStream, videoStats);
    }
    if (audioStats)
    {
        GetStatistics(m_audioStream, audioStats);
    }
    m_writer = nullptr;
}

void RecordingFile::WriteVideo(IMFSample* video)
{
    const auto missing = m_videoAnalyzer.Analyze(video);
    if (ReceivedFirstKeyFrame())
    {
        if (missing > winrt::TimeSpan::zero())
        {
            // Frames has been dropped by the encoder
            SendStreamTick(m_videoStream, m_videoTime);
            m_videoTime += missing;
        }
    }
    else
    {
        if (!WaitForFirstKeyFrame(video))
        {
            return;
        }
    }
    WINRT_VERIFY_(S_OK, video->SetSampleTime(m_videoTime.count()));
    m_videoTime += WriteInternal(video, m_videoStream);
}

void RecordingFile::WriteAudio(IMFSample* audio)
{
    if (FirstAudioSample())
    {
        auto dts = VideoFrameAnalyzer::GetDts(audio);
        WINRT_ASSERT(dts >= m_audioVideoSyncPoint);
        auto offset = dts - m_audioVideoSyncPoint;
        SendStreamTick(m_audioStream, winrt::TimeSpan::zero());
        m_audioTime = offset;
        PrintLine("[First audio] %lldms", VideoFrameAnalyzer::AsMilliseconds(offset));
    }
    //PrintLine("audio: %lld", m_audioTime.count());
    WINRT_VERIFY_(S_OK, audio->SetSampleTime(m_audioTime.count()));
    m_audioTime += WriteInternal(audio, m_audioStream);
}

winrt::TimeSpan RecordingFile::WriteInternal(IMFSample* sample, DWORD stream) const
{
    LOG_IF_FAILED(m_writer->WriteSample(stream, sample));
    return VideoFrameAnalyzer::GetSampleDuration(sample);
}

void RecordingFile::SendStreamTick(DWORD stream, winrt::TimeSpan timestamp) const
{
    LOG_IF_FAILED(m_writer->SendStreamTick(stream, timestamp.count()));
}

bool RecordingFile::WaitForFirstKeyFrame(IMFSample* video)
{
    uint32_t keyframe{};
    video->GetUINT32(MFSampleExtension_CleanPoint, &keyframe);
    if (!keyframe)
    {
        // Need a proper cleanpoint
        return false;
    }
    const auto dts = VideoFrameAnalyzer::GetDts(video);
    const auto elapsedSinceCaptured = VideoFrameAnalyzer::Now() - dts;
    m_videoTime = winrt::TimeSpan::zero();
    m_acquisitionTime = winrt::clock::now() - elapsedSinceCaptured;
    m_audioVideoSyncPoint = dts;
    return true;
}

void RecordingFile::GetStatistics(DWORD stream, MF_SINK_WRITER_STATISTICS* stats) const
{
    if (stream != InvalidStream)
    {
        stats->cb = sizeof(MF_SINK_WRITER_STATISTICS);
        WINRT_VERIFY_(S_OK, m_writer->GetStatistics(stream, stats));
    }
}

HANDLE RecordingFile::OpenFile(std::wstring_view path)
{
    constexpr DWORD DesiredAccess = FILE_WRITE_ATTRIBUTES;
    constexpr DWORD ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    return ::CreateFileW(
        path.data(),
        DesiredAccess,
        ShareMode,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
}
