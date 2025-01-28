#include "pch.h"
#include "audio_device.hpp"
#include <windows.media.core.interop.h>

winrt::IAsyncAction AudioDevice::InitializeAsync(winrt::com_ptr<IMFActivate> device)
{
    co_await winrt::resume_background();

    winrt::DeviceInformation deviceInfo{ nullptr };

    wchar_t* link{ nullptr };
    wchar_t* fname{ nullptr };
    uint32_t len{};
    device->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_SYMBOLIC_LINK, &link, &len);
    device->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &fname, &len);
    auto free_link = wil::scope_exit([=]
        {
            ::CoTaskMemFree(link);
            ::CoTaskMemFree(fname);
        });

    PrintLine("Initializing '%ws'", fname);

    winrt::hstring id(link);
    auto dev_info = co_await winrt::DeviceInformation::CreateFromIdAsync(id);

    // Create the audio graph
    winrt::AudioGraphSettings settings{ winrt::Render::AudioRenderCategory::Media };
    //settings.EncodingProperties(winrt::AudioEncodingProperties::CreatePcm(48000, 2, 16));
    //settings.QuantumSizeSelectionMode(winrt::QuantumSizeSelectionMode::LowestLatency);

    auto graphResult{ co_await winrt::AudioGraph::CreateAsync(settings) };
    if (graphResult.Status() != winrt::AudioGraphCreationStatus::Success)
    {
        auto status = graphResult.Status();
        auto hr = graphResult.ExtendedError();
        throw winrt::hresult_error(graphResult.ExtendedError());
    }
    m_graph = graphResult.Graph();
    auto audioFormat{ m_graph.EncodingProperties() };
    m_sampleRate = audioFormat.SampleRate();

    // Create device input node
    auto inputNodeResult = co_await m_graph.CreateDeviceInputNodeAsync(
        winrt::Capture::MediaCategory::GameChat, audioFormat, deviceInfo);
  
    if (inputNodeResult.Status() != winrt::AudioDeviceNodeCreationStatus::Success)
    {
        throw winrt::hresult_error(inputNodeResult.ExtendedError());
    }
    m_inputNode = inputNodeResult.DeviceInputNode();

    // Create frame output node
    m_outputNode = m_graph.CreateFrameOutputNode(audioFormat);
    m_inputNode.AddOutgoingConnection(m_outputNode);
    m_graph.QuantumStarted({ get_strong(), &AudioDevice::QuantumStarted });
}

void AudioDevice::QuantumStarted(const winrt::AudioGraph& graph, const winrt::IInspectable& object)
{
    const int64_t dts = ::MFGetSystemTime() - CalculateGraphLatency(graph, m_sampleRate);
    auto audioFrame = m_outputNode.GetFrame();
    int64_t timestamp{};
    int64_t duration{};
    auto sample = ToMFSample(audioFrame, &timestamp, &duration);
    if (duration == 0)
    {
        return;
    }
    if (!timestamp || m_expectedTimestamp != timestamp)
    {
        WINRT_VERIFY_(S_OK, sample->SetUINT32(MFSampleExtension_Discontinuity, 1));
        m_expectedTimestamp = timestamp;
    }
    WINRT_VERIFY_(S_OK, sample->SetUINT32(VidiView_AudioSample, 1));
    WINRT_VERIFY_(S_OK, sample->SetUINT64(MFSampleExtension_DeviceTimestamp, dts));
    DeliverSample(sample.get());
    m_expectedTimestamp += duration;
}

void AudioDevice::SetOutputFile(IMFAsyncCallback* file)
{
    winrt::slim_lock_guard guard{ m_callbackMutex };
    m_callback.copy_from(file);
}

void AudioDevice::DeliverSample(IMFSample* sample) const noexcept
{
    winrt::slim_lock_guard guard{ m_callbackMutex };
    if (auto cb = m_callback.get())
    {
        DWORD workQueue{};
        DWORD flags{};
        cb->GetParameters(&flags, &workQueue);
        ::MFPutWorkItem2(workQueue, 0, cb, sample);
    }
}

void AudioDevice::StartCapture()
{
    m_graph.Start();
}

void AudioDevice::StopCapture()
{
    m_graph.Stop();
}

winrt::com_ptr<IMFMediaType> AudioDevice::GetOutputFormat()
{
    auto format{ m_graph.EncodingProperties() };
    winrt::com_ptr<IMFMediaType> type;
    THROW_IF_FAILED(::MFCreateMediaTypeFromProperties(format.try_as<::IUnknown>().get(), type.put()));
    return type;
}

int64_t AudioDevice::CalculateGraphLatency(const winrt::AudioGraph& graph, const uint64_t sampleRate)
{
    using namespace std::chrono_literals;
    constexpr auto ONE_SECOND = std::chrono::duration_cast<winrt::TimeSpan>(1s).count();
    const auto value = graph.LatencyInSamples() * ONE_SECOND;
    const auto duration = value / static_cast<int64_t>(sampleRate);
    return duration;
}

winrt::com_ptr<IMFSample> AudioDevice::ToMFSample(const winrt::AudioFrame& frame, int64_t* timestamp, int64_t* duration)
{
    winrt::com_ptr<IMFSample> sample;
    if (auto native = frame.try_as<IAudioFrameNative>())
    {
        WINRT_VERIFY_(S_OK, native->GetData(__uuidof(IMFSample), sample.put_void()));
    }
    if (sample)
    {
        WINRT_VERIFY_(S_OK, sample->GetSampleTime(timestamp));
        WINRT_VERIFY_(S_OK, sample->GetSampleDuration(duration));
    }
    return sample;
}
