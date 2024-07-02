#include "framework.hpp"

static void ReadFramesWithSourceReader(winrt::com_ptr<IMFActivate>& activate)
{
    auto reader = CreateSourceReader(activate);
    HRESULT hr = S_OK;
    uint32_t frames = 10;
    while (SUCCEEDED(hr) && frames > 0)
    {
        winrt::com_ptr<IMFSample> sample;
        DWORD flags{};
        hr = reader->ReadSample(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0,
            nullptr,
            &flags,
            nullptr,
            sample.put());

        if (sample)
        {
            int64_t timestamp{};
            int64_t duration{};
            sample->GetSampleDuration(&duration);
            sample->GetSampleTime(&timestamp);
            Print("Sample: %lld (%lld)", timestamp, duration);
            --frames;
        }
    }
}

struct CaptureEngineCB : winrt::implements<CaptureEngineCB, ::IMFCaptureEngineOnEventCallback, ::IMFCaptureEngineOnSampleCallback>
{
    winrt::hresult m_status;
    winrt::handle  m_event;
    GUID           m_type{};
    uint32_t       m_frameCount{};

    CaptureEngineCB() : m_event{ ::CreateEventW(nullptr, FALSE, FALSE, nullptr) }
    {
    }

    winrt::hresult WaitFor(const GUID& type, DWORD timeout = INFINITE)
    {
        m_type = type;
        if (::WaitForSingleObject(m_event.get(), timeout) == WAIT_OBJECT_0)
        {
            return m_status;
        }
        return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
    }

    void WaitForAllFrames(DWORD timeout = INFINITE) const
    {
        ::WaitForSingleObject(m_event.get(), timeout);
    }

    HRESULT GetResult(IMFMediaEvent* event, GUID* type) const
    {
        WINRT_VERIFY_(S_OK, event->GetExtendedType(type));
        HRESULT status{};
        event->GetStatus(&status);
        return status;
    }

    HRESULT __stdcall OnEvent(IMFMediaEvent* event) noexcept final
    {
        GUID type{};
        HRESULT status = GetResult(event, &type);

        if (MF_CAPTURE_ENGINE_INITIALIZED == type)
        {
            Print("MF_CAPTURE_ENGINE_INITIALIZED (0x%08lx)", status);
        }
        else if (MF_CAPTURE_ENGINE_PREVIEW_STARTED == type)
        {
            Print("MF_CAPTURE_ENGINE_PREVIEW_STARTED (0x%08lx)", status);
        }
        else if (MF_CAPTURE_ENGINE_PREVIEW_STOPPED == type)
        {
            Print("MF_CAPTURE_ENGINE_PREVIEW_STOPPED (0x%08lx)", status);
        }
        else if (MF_CAPTURE_SINK_PREPARED == type)
        {
            Print("MF_CAPTURE_SINK_PREPARED (0x%08lx)", status);
        }

        if (type == m_type)
        {
            m_type = {};
            m_status = status;
            ::SetEvent(m_event.get());
        }
        return S_OK;
    }

    HRESULT __stdcall OnSample(IMFSample* sample) noexcept final
    {
        if (sample)
        {
            int64_t timestamp{};
            int64_t duration{};
            sample->GetSampleDuration(&duration);
            sample->GetSampleTime(&timestamp);
            Print("Sample: %lld (%lld)", timestamp, duration);
            auto count = ++m_frameCount;
            if (count == 10)
            {
                ::SetEvent(m_event.get());
            }
        }
        return S_OK;
    }
};

static void ReadFramesWithCaptureEngine(winrt::com_ptr<IMFActivate>& activate)
{
    activate->ShutdownObject();

    auto factory{ winrt::create_instance<IMFCaptureEngineClassFactory>(CLSID_MFCaptureEngineClassFactory) };
    winrt::com_ptr<IMFCaptureEngine> engine;
    winrt::check_hresult(factory->CreateInstance(CLSID_MFCaptureEngine, winrt::guid_of<IMFCaptureEngine>(), engine.put_void()));

    winrt::com_ptr<IMFAttributes> settings;
    winrt::check_hresult(::MFCreateAttributes(settings.put(), 6));
    WINRT_VERIFY_(S_OK, settings->SetUINT32(MF_CAPTURE_ENGINE_USE_VIDEO_DEVICE_ONLY, TRUE));
    WINRT_VERIFY_(S_OK, settings->SetUINT32(MF_CAPTURE_ENGINE_ENABLE_CAMERA_STREAMSTATE_NOTIFICATION, 1));

    auto cb = winrt::make_self<CaptureEngineCB>();
    winrt::check_hresult(engine->Initialize(cb.get(), settings.get(), nullptr, activate.get()));
    cb->WaitFor(MF_CAPTURE_ENGINE_INITIALIZED);

    winrt::com_ptr<IMFMediaType> mediaType;
    winrt::com_ptr<IMFCaptureSource> source;
    WINRT_VERIFY_(S_OK, engine->GetSource(source.put()));
    WINRT_VERIFY_(S_OK, source->GetCurrentDeviceMediaType(0, mediaType.put()));

    winrt::com_ptr<IMFCaptureSink> sink;
    WINRT_VERIFY_(S_OK, engine->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_PREVIEW, sink.put()));

    DWORD sinkStream{};
    winrt::check_hresult(sink->AddStream(0, mediaType.get(), nullptr, &sinkStream));
    winrt::check_hresult(sink.as<IMFCapturePreviewSink>()->SetSampleCallback(sinkStream, cb.get()));
    winrt::check_hresult(sink->Prepare());
    cb->WaitFor(MF_CAPTURE_SINK_PREPARED);

    winrt::check_hresult(engine->StartPreview());
    cb->WaitForAllFrames();
}


int main()
{
    auto ctx{ InitializeApp() };
    try
    {
        if (auto device = SelectVideoDeviceActivate())
        {
            ReadFramesWithSourceReader(device);
            ReadFramesWithCaptureEngine(device);
        }
    }
    CATCH_LOG();
    return 0;
}

