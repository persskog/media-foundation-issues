#include "pch.h"
#include "recording.hpp"

HRESULT __stdcall Recording::GetParameters(DWORD* flags, DWORD* queue) noexcept
{
    *flags = 0;
    *queue = MFASYNC_CALLBACK_QUEUE_STANDARD;
    return S_OK;
}

HRESULT __stdcall Recording::Invoke(IMFAsyncResult* result) noexcept
{
    winrt::com_ptr<IMFSample> sample;
    result->GetStateNoAddRef()->QueryInterface(sample.put());
    if (sample)
    {
        uint32_t isAudio{};
        sample->GetUINT32(VidiView_AudioSample, &isAudio);
        if (!isAudio)
        {
            Write(sample.get());
        }
        else
        {
            if (m_audioVideoSyncPoint >= winrt::TimeSpan::zero())
            {
                WriteAudio(sample.get());
            }
        }
    }

    return S_OK;
}

HRESULT Recording::Initialize(IMFMediaType* videoType, IMFMediaType* audioType)
{
    winrt::com_ptr<IMFAttributes> attr;
    RETURN_IF_FAILED(::MFCreateAttributes(attr.put(), 7));
    attr->SetUINT32(MF_LOW_LATENCY, TRUE);
    attr->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE);
    attr->SetGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_FMPEG4);
    attr->SetString(MF_READWRITE_MMCSS_CLASS, L"Capture");
    attr->SetString(MF_READWRITE_MMCSS_CLASS_AUDIO, L"Audio");

    RETURN_IF_FAILED(::MFCreateSinkWriterFromURL(L"recording.mp4", nullptr, attr.get(), m_writer.put()));
    if (audioType)
    {
        winrt::com_ptr<IMFMediaType> aac;
        RETURN_IF_FAILED(::MFCreateMediaType(aac.put()));
        WINRT_VERIFY_(S_OK, aac->SetGUID(MF_MT_MAJOR_TYPE,        MFMediaType_Audio));
        WINRT_VERIFY_(S_OK, aac->SetGUID(MF_MT_SUBTYPE,           MFAudioFormat_AAC));
        WINRT_VERIFY_(S_OK, aac->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,         16u));
        WINRT_VERIFY_(S_OK, aac->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 24000u));
        WINRT_VERIFY_(S_OK, aac->SetUINT32(MF_MT_AAC_PAYLOAD_TYPE,              0x0));
        CopyAttribute(MF_MT_AUDIO_NUM_CHANNELS, audioType,                 aac.get());
        CopyAttribute(MF_MT_AUDIO_SAMPLES_PER_SECOND, audioType,           aac.get());
        RETURN_IF_FAILED(m_writer->AddStream(aac.get(), &m_audioStream));
        RETURN_IF_FAILED(m_writer->SetInputMediaType(m_audioStream, audioType, nullptr));
    }
    RETURN_IF_FAILED(m_writer->AddStream(videoType, &m_videoStream));
    m_analyzer.Initialize(videoType);
    m_analyzer.Reset();
    RETURN_IF_FAILED(m_writer->BeginWriting());
    return S_OK;
}

HRESULT Recording::Done()
{
    m_done.store(true);
    return m_writer->Finalize();
}

void Recording::Write(IMFSample* frame)
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    auto missing = m_analyzer.Analyze(frame);
    if (missing > winrt::TimeSpan::zero())
    {
        m_writer->SendStreamTick(0, m_videoTime.count());
        m_videoTime += missing;
    }
    else
    {
        if (m_videoTime < winrt::TimeSpan::zero())
        {
            m_audioVideoSyncPoint = m_analyzer.GetPrevDts();
        }
    }

    WriteVideo(frame);
}

void Recording::WriteAudio(IMFSample* audio)
{
    if (m_audioTime < winrt::TimeSpan::zero())
    {
        auto dts = VideoFrameAnalyzer::GetDts(audio);
        auto offset = dts - m_audioVideoSyncPoint;
        m_writer->SendStreamTick(m_audioStream, 0);
        m_audioTime = offset;
        PrintLine("[First audio] %lldms", VideoFrameAnalyzer::AsMilliseconds(offset));
    }

    //PrintLine("audio: %lld", m_audioTime.count());
    audio->SetSampleTime(m_audioTime.count());
    WriteInternal(audio, m_audioStream);
    m_audioTime += VideoFrameAnalyzer::GetSampleDuration(audio);
    
}

void Recording::WriteVideo(IMFSample* video)
{
    //PrintLine("VIDEO: %lld", m_videoTime.count());
    WriteInternal(video, m_videoStream);
    m_videoTime += VideoFrameAnalyzer::GetSampleDuration(video);
}

void Recording::WriteInternal(IMFSample* sample, DWORD stream)
{
    if (!m_done.load(std::memory_order_relaxed))
    {
        LOG_IF_FAILED(m_writer->WriteSample(stream, sample));
    }
}
