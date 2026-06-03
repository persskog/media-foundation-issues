#pragma once

struct FourCC
{
    wchar_t Data[5]{};

    constexpr FourCC() noexcept = default;
    constexpr FourCC(const unsigned long data1) noexcept
        : Data{ to_char(data1,  0ul),
                to_char(data1,  8ul),
                to_char(data1, 16ul),
                to_char(data1, 24ul),
                 L'\0' }
    {}

private:
    static constexpr wchar_t to_char(unsigned long data1, unsigned long bits) noexcept
    {
        return static_cast<wchar_t>((data1 >> bits) & 0xFFul);
    }
};

struct PinOutput
{
    uint32_t Width;
    uint32_t Height;
    uint32_t RateNum;
    uint32_t RateDen;
    FourCC   Subtype;
    DWORD    TypeIndex;

    void print() const
    {
        PRINTLN("%ws: [%ux%u] @ %.2ffps, ratio %u/%u",
            Subtype.Data,
            Width,
            Height,
            static_cast<float>(RateNum) / RateDen,
            RateNum,
            RateDen);
    }
};

struct FrameCb : winrt::implements<FrameCb, IMFCaptureEngineOnSampleCallback2>
{
    int64_t         PrevDts{ -1 };
    uint64_t        FrameCounter{};
    winrt::TimeSpan ExpectedFrameDuration{};
    int64_t         FpsStartTime{ -1 };
    uint32_t        FpsFrameCounter{};
    const char*     Name{};

    explicit FrameCb(const char* name) noexcept : Name{ name }
    {}

    HRESULT __stdcall OnSynchronizedEvent(IMFMediaEvent*) noexcept final
    {
        return S_OK;
    }

    HRESULT __stdcall OnSample(IMFSample* frame) noexcept final
    {
        auto now = ::MFGetSystemTime();

        if (Name == nullptr)
        {
            return S_OK;
        }

        if (frame)
        {
            uint64_t dts{};
            frame->GetUINT64(MFSampleExtension_DeviceTimestamp, &dts);
            if (PrevDts >= 0)
            {
                auto fps = UpdateFps(now);
                const auto elapsed = dts - PrevDts;
                const auto elapsed_ms = ToMilliseconds(elapsed);
                const auto dropped = CalculateLostFrames(dts, PrevDts, ExpectedFrameDuration.count());
                if (dropped)
                {
                    PRINTLN("[%s pin]:%llu: %u dropped frames, %u ms (%lld 100ns) between two frames",
                        Name,
                        FrameCounter,
                        dropped,
                        elapsed_ms,
                        elapsed);
                }
                else if (fps)
                {
                    PRINTLN("[%s pin]:%llu: %.2f fps", Name, FrameCounter, *fps);
                }
            }
            else
            {
                FpsStartTime = now;
                FpsFrameCounter = 1;
            }

            PrevDts = dts;
            ++FrameCounter;
        }
        return S_OK;
    }

    std::optional<float> UpdateFps(MFTIME now)
    {
        constexpr const auto OneSecond = 10'000'000; // 1 second in 100ns units
        ++FpsFrameCounter;
        const auto clock_elapsed = now - FpsStartTime;
        if (clock_elapsed >= OneSecond)
        {
            FpsStartTime = now;
            auto fps = (FpsFrameCounter * OneSecond) / static_cast<float>(clock_elapsed);
            FpsFrameCounter = 0;
            return fps;
        }
        return std::nullopt;
    }

    static uint32_t ToMilliseconds(int64_t hundredNanosec) noexcept
    {
        return static_cast<uint32_t>(duration_cast<milliseconds>(winrt::TimeSpan{ hundredNanosec }).count());
    }

    static int32_t CalculateLostFrames(uint64_t dts, int64_t prevDts, int64_t duration) noexcept
    {
        const auto dt = dts - static_cast<uint64_t>(prevDts);
        const auto lost = dt / static_cast<float>(duration);
        // Do not include current frame, hence -1
        const auto count = static_cast<int32_t>(std::round(lost)) - 1;
        return count >= 0 ? count : 0;
    }
};

struct State
{
    static constexpr DWORD PreviewStream = static_cast<DWORD>(
        MF_CAPTURE_ENGINE_PREFERRED_SOURCE_STREAM_FOR_VIDEO_PREVIEW);
    static constexpr DWORD EncodeStream = static_cast<DWORD>(
        MF_CAPTURE_ENGINE_PREFERRED_SOURCE_STREAM_FOR_VIDEO_RECORD);

    winrt::com_ptr<IMFDXGIDeviceManager>   Manager;
    winrt::com_ptr<IMFActivate>            VideoDevice;
    winrt::com_ptr<IMFAttributes>          Settings;
    winrt::com_ptr<IMFCaptureEngine>       Engine;
    winrt::com_ptr<IMFCaptureSource>       EngineSource;
    winrt::com_ptr<IMFCapturePreviewSink>  EnginePreviewSink;
    winrt::com_ptr<IMFCaptureRecordSink>   EngineRecordSink;
    winrt::com_ptr<IMFCapturePhotoSink>    EnginePhotoSink;

    winrt::com_ptr<FrameCb>                PreviewCallback;
    winrt::com_ptr<FrameCb>                EncodeCallback;

    std::vector<PinOutput>                 PreviewOutputs;
    std::vector<PinOutput>                 EncodeOutputs;

    State();
    ~State();
    void CollectAvailableDeviceOutputs(std::vector<PinOutput>& outputs, DWORD stream);
    void OnEvent(IMFMediaEvent* event) noexcept;
    void PreparePreview();
    void PrepareEncode();
    void TogglePreview();
    void ToggleEncode();

    void ShowAvailablePreviewOutputs() const
    {
        PRINTLN("Available preview outputs:");
        for (const auto& output : PreviewOutputs)
        {
            output.print();
        }
    }

    void ShowAvailableEncodeOutputs() const
    {
        PRINTLN("Available encode outputs:");
        for (const auto& output : EncodeOutputs)
        {
            output.print();
        }
    }

private:
    std::atomic_bool m_previewing{ false };
    std::atomic_bool m_encoding{ false };
};

struct CaptureEngineCb : winrt::implements<CaptureEngineCb, IMFCaptureEngineOnEventCallback>
{
    // This class makes cleanup much easier and decouples the VideoEngine from the IMFCaptureEngine
    explicit CaptureEngineCb(State* state) noexcept : m_state{ state } {}

    HRESULT __stdcall OnEvent(IMFMediaEvent* event) noexcept final
    {
        if (m_state != nullptr)
        {
            m_state->OnEvent(event);
        }
        return S_OK;
    }

    State* m_state;
};


winrt::com_ptr<IMFDXGIDeviceManager> CreateMFDXGIDeviceManager();

winrt::com_ptr<IMFCaptureEngine> CreateCaptureEngine();

winrt::com_ptr<IMFActivate> CreateVideoDevice(State& state);

void Initialize(State& state);
