#pragma once


struct VideoFrameAnalyzer
{
    int32_t Analyze(IMFSample* frame);
    void Reset();
    void OnFirstFrame(IMFSample* frame);
    winrt::TimeSpan ElapsedTimeSinceFirstFrame() const;
    bool IsDtsDeltaOk(winrt::TimeSpan delta, winrt::TimeSpan frameDuration);
    void ValidateFrameCounter(winrt::TimeSpan frameDuration) const;
    void Initialize(IMFMediaType* type);
    winrt::TimeSpan CheckForIncreasedFrameDelay(winrt::TimeSpan delta);

    static uint32_t CalculateLostFrames(winrt::TimeSpan delta, winrt::TimeSpan frameDuration);
    static winrt::TimeSpan GetSampleTime(IMFSample* sample);
    static winrt::TimeSpan GetSampleDuration(IMFSample* sample);
    static winrt::TimeSpan GetDts(IMFSample* sample);
    static winrt::TimeSpan Now();
    static int64_t AsMilliseconds(winrt::TimeSpan ts);
    static bool IsInvalidTime(winrt::TimeSpan ts);

private:
    static constexpr long long HundredNanoSecondsInSecond = 10000000LL;
    static constexpr winrt::TimeSpan DeltaLimit{ HundredNanoSecondsInSecond * 2};
    static constexpr winrt::TimeSpan InvalidTime{ -1 };

    winrt::TimeSpan m_prevDts{ InvalidTime };
    winrt::TimeSpan m_clockStart{ InvalidTime };
    winrt::TimeSpan m_avgTimePerFrame{};
    uint32_t        m_frameCounter{};
    uint32_t        m_approxLostFrames{};
    uint32_t        m_accumLostFrames{};
    winrt::TimeSpan m_accumDuration{};
    winrt::TimeSpan m_delayUntilFirstFrame{ InvalidTime };
};


struct VideoEncoder : winrt::implements<VideoEncoder, IMFCaptureEngineOnSampleCallback2>
{
    VideoEncoder(const winrt::com_ptr<IMFCaptureEngine>& engine);

    static winrt::com_ptr<VideoEncoder> Create(IMFActivate* videoDevice, ID3D11Device* device);

    // IMFCaptureEngineOnSampleCallback2
    HRESULT __stdcall OnSample(IMFSample* sample) noexcept final;
    HRESULT __stdcall OnSynchronizedEvent(IMFMediaEvent* event) noexcept final;
    void OnCaptureEngineEvent(IMFMediaEvent* event);

    void StartEncoder();
    void StopEncoder();
    void TakePhoto();

private:
    HRESULT AwaitReady() const noexcept;
    void SignalStatus(HRESULT stauts);

    winrt::com_ptr<IMFCaptureSource> GetSource() const;
    winrt::com_ptr<IMFCaptureRecordSink> GetRecordSink() const;
    winrt::com_ptr<IMFCapturePhotoSink> GetPhotoSink() const;
    winrt::com_ptr<IMFTransform> GetEncoderMFT(IMFCaptureSink* sink) noexcept;

    HRESULT OnInitialized(HRESULT status);
    HRESULT OnSinkPrepared(HRESULT status);

private:
    VideoFrameAnalyzer               m_analyzer;
    winrt::com_ptr<IMFCaptureEngine> m_engine;
    winrt::com_ptr<IMFTransform>     m_encoderMFT;
    winrt::handle                    m_ready;
    winrt::hresult                   m_status{};
    DWORD                            m_photoSink{0xffffffff};
    uint32_t                         m_gopLength{};
};

