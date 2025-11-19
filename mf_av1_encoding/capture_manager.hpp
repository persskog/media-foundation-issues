#pragma once
#include "winrt/base.h"
#include <icodecapi.h>

struct CaptureManager : winrt::implements<CaptureManager, IMFCaptureEngineOnSampleCallback2>
{
    friend struct CaptureEngineCb;

    CaptureManager();
    winrt::com_ptr<IMFCaptureEngine> Engine;

    // IMFCaptureEngineOnSampleCallback2
    HRESULT __stdcall OnSample(IMFSample* sample) noexcept final;
    HRESULT __stdcall OnSynchronizedEvent(IMFMediaEvent* event) noexcept final;

    void Initialize(IMFAttributes* attr, ::IUnknown* d3dManager, ::IUnknown* videoSource);

private:
    bool CreateSampleDescriptorBoxIfNeeded(std::span<const uint8_t> payload) noexcept;
    void PrepareAv1Encoding() noexcept;
    void OnRecordSinkPrepared() noexcept;
    void ForceKeyFrame() const noexcept;
    void CreateWriter();

    void OnEvent(IMFMediaEvent* event) noexcept;

    winrt::com_ptr<IMFSinkWriter> m_writer;
    winrt::com_ptr<IMFMediaType> m_mediaType;
    winrt::com_ptr<ICodecAPI> m_encoder;
    bool m_sampleDescriptorBoxWritten = false;
    int64_t m_duration{};
};

// This class makes cleanup much easier and decouples the  from the IMFCaptureEngine
struct CaptureEngineCb : winrt::implements<CaptureEngineCb, IMFCaptureEngineOnEventCallback>
{
    explicit CaptureEngineCb(CaptureManager* manager) noexcept
        : m_manager{ manager->get_weak() }
    {
    }

    HRESULT __stdcall OnEvent(IMFMediaEvent* event) noexcept final
    {
        if (auto m = m_manager.get())
        {
            PrintLine("\n");
            m->OnEvent(event);
        }
        return S_OK;
    }

    winrt::weak_ref<CaptureManager> m_manager;
};
