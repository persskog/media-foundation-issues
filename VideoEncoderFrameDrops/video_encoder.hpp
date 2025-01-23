#pragma once


struct VideoEncoder : winrt::implements<VideoEncoder, IMFCaptureEngineOnSampleCallback2>
{
    static winrt::com_ptr<VideoEncoder> Create();

    // IMFCaptureEngineOnSampleCallback2
    HRESULT __stdcall OnSample(IMFSample* sample) noexcept final;
    HRESULT __stdcall OnSynchronizedEvent(IMFMediaEvent* event) noexcept final;

    void OnCaptureEngineEvent(IMFMediaEvent* event);
};

