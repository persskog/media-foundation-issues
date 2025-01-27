#pragma once
#include <Unknwnbase.h>
#include "winrt/Windows.Media.Audio.h"
#include "winrt/Windows.Media.MediaProperties.h"

class AudioDevice : public winrt::implements<AudioDevice, ::IUnknown>
{
    uint64_t                         m_sampleRate{};
    mutable int64_t                  m_expectedTimestamp{};
    mutable winrt::slim_mutex        m_callbackMutex;
    winrt::com_ptr<IMFAsyncCallback> m_callback;
    winrt::AudioGraph                m_graph{ nullptr };
    winrt::AudioDeviceInputNode      m_inputNode{ nullptr };
    winrt::AudioFrameOutputNode      m_outputNode{ nullptr };

public:
    winrt::IAsyncAction InitializeAsync(winrt::com_ptr<IMFActivate> device);
    void QuantumStarted(const winrt::AudioGraph& graph, const winrt::IInspectable& object);
    void SetDestination(IMFAsyncCallback* callback);
    void DeliverSample(IMFSample* sample) const noexcept;
    void StartCapture();
    void StopCapture();
    void GetOutputType(IMFMediaType** type);

private:
    static int64_t CalculateGraphLatency(const winrt::AudioGraph& graph, const uint64_t sampleRate);
    static winrt::com_ptr<IMFSample> ToMFSample(const winrt::AudioFrame& frame, int64_t* timestamp, int64_t* duration);
};

