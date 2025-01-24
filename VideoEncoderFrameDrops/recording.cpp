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
    winrt::com_ptr<IMFSample> frame;
    result->GetStateNoAddRef()->QueryInterface(frame.put());

    if (frame)
    {
        Write(frame.get());
    }

    return S_OK;
}

HRESULT Recording::Initialize(IMFMediaType* type)
{
    winrt::com_ptr<IMFAttributes> attr;
    RETURN_IF_FAILED(::MFCreateAttributes(attr.put(), 7));
    attr->SetUINT32(MF_LOW_LATENCY, TRUE);
    attr->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE);
    attr->SetGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_FMPEG4);
    attr->SetString(MF_READWRITE_MMCSS_CLASS, L"Capture");
    attr->SetString(MF_READWRITE_MMCSS_CLASS_AUDIO, L"Audio");

    RETURN_IF_FAILED(::MFCreateSinkWriterFromURL(L"recording.mp4", nullptr, attr.get(), m_writer.put()));

    DWORD stream{};
    RETURN_IF_FAILED(m_writer->AddStream(type, &stream));
    RETURN_IF_FAILED(m_writer->BeginWriting());

    m_analyzer.Initialize(type);
    m_analyzer.Reset();
    return S_OK;
}

HRESULT Recording::Done()
{
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

    frame->SetSampleTime(m_videoTime.count());
    m_videoTime += VideoFrameAnalyzer::GetSampleDuration(frame);

    auto begin = ::MFGetSystemTime();
    HRESULT hr = m_writer->WriteSample(0, frame);
    auto elapsed = winrt::TimeSpan(::MFGetSystemTime() - begin);

    if (elapsed > 5ms)
    {
        PrintLine("Write took %lldms", duration_cast<milliseconds>(elapsed));
    }
}
