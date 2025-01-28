#pragma once
#include "video_frame_analyzer.hpp"
#include "recording_file.hpp"

class AudioDevice;

struct VideoEncoder : winrt::implements<VideoEncoder, IMFCaptureEngineOnSampleCallback2>
{
    VideoEncoder(const winrt::com_ptr<IMFCaptureEngine>& engine);

    static winrt::com_ptr<VideoEncoder> Create(IMFActivate* videoDevice, ID3D11Device* device);

    // IMFCaptureEngineOnSampleCallback2
    HRESULT __stdcall OnSample(IMFSample* sample) noexcept final;
    HRESULT __stdcall OnSynchronizedEvent(IMFMediaEvent* event) noexcept final;
    void OnCaptureEngineEvent(IMFMediaEvent* event);

    void SetOutputFile(RecordingFile* file);
    winrt::com_ptr<IMFMediaType> GetEncoderOutputFormat() noexcept;

    void StartEncoder(AudioDevice* audioDevice);
    void StopEncoder(AudioDevice* audioDevice);
    void TakePhoto();

private:
    HRESULT AwaitReady() const noexcept;
    void SignalStatus(HRESULT stauts);

    winrt::com_ptr<IMFCaptureSource> GetSource() const;
    winrt::com_ptr<IMFCapturePreviewSink> GetPreviewSink() const;
    winrt::com_ptr<IMFCaptureRecordSink> GetRecordSink() const;
    winrt::com_ptr<IMFCapturePhotoSink> GetPhotoSink() const;
    winrt::com_ptr<IMFTransform> GetEncoderMFT(IMFCaptureSink* sink) noexcept;

    

    HRESULT OnInitialized(HRESULT status);
    HRESULT OnSinkPrepared(HRESULT status);

private:
    winrt::com_ptr<RecordingFile>        m_recording;
    winrt::com_ptr<IMFCaptureEngine> m_engine;
    winrt::com_ptr<IMFTransform>     m_encoderMFT;
    winrt::handle                    m_ready;
    winrt::hresult                   m_status{};
    DWORD                            m_photoSink{0xffffffff};
    uint32_t                         m_gopLength{};
};

