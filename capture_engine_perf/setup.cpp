#include "pch.h"
#include "setup.h"

State::State()
{
    winrt::check_hresult(::MFCreateAttributes(Settings.put(), 5));
}

State::~State()
{
    if (Manager)
    {
        WINRT_VERIFY_(S_OK, ::MFUnlockDXGIDeviceManager());
    }

    if (VideoDevice)
    {
        WINRT_VERIFY_(S_OK, VideoDevice->ShutdownObject());
    }
}

void State::CollectAvailableDeviceOutputs(std::vector<PinOutput>& outputs, DWORD stream)
{
    outputs.reserve(200);
    auto src = EngineSource.get();
    winrt::com_ptr<IMFMediaType> type;
    DWORD type_index{};

    const DWORD nv12_fourcc = MFVideoFormat_NV12.Data1;
    const DWORD yuy2_fourcc = MFVideoFormat_YUY2.Data1;

    while (S_OK == src->GetAvailableDeviceMediaType(stream, type_index, type.put()))
    {
        GUID subtype{};
        winrt::check_hresult(type->GetGUID(MF_MT_SUBTYPE, &subtype));

        // Just collect NV12 and YUY2 for clearity...
        if (subtype.Data1 != nv12_fourcc && subtype.Data1 != yuy2_fourcc)
        {
            ++type_index;
            continue;
        }

        auto& output = outputs.emplace_back();
        winrt::check_hresult(::MFGetAttributeSize(type.get(), MF_MT_FRAME_SIZE, &output.Width, &output.Height));
        winrt::check_hresult(::MFGetAttributeRatio(type.get(), MF_MT_FRAME_RATE, &output.RateNum, &output.RateDen));
        output.Subtype = FourCC(subtype.Data1);
        output.TypeIndex = type_index;
        ++type_index;
    }
}

void State::OnEvent(IMFMediaEvent* event) noexcept
{
    HRESULT status{};
    GUID type{};
    WINRT_VERIFY_(S_OK, event->GetStatus(&status));
    WINRT_VERIFY_(S_OK, event->GetExtendedType(&type));
    auto e = Engine.get();
    try
    {
        if (MF_CAPTURE_ENGINE_INITIALIZED == type)
        {
            winrt::check_hresult(e->GetSource(EngineSource.put()));
            winrt::com_ptr<IMFCaptureSink> sink;
            winrt::check_hresult(e->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_PREVIEW, sink.put()));
            winrt::check_hresult(sink->QueryInterface(EnginePreviewSink.put()));
            winrt::check_hresult(e->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_RECORD, sink.put()));
            winrt::check_hresult(sink->QueryInterface(EngineRecordSink.put()));
            winrt::check_hresult(e->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_PHOTO, sink.put()));
            winrt::check_hresult(sink->QueryInterface(EnginePhotoSink.put()));

            PrepareEncode();
            PreparePreview();
        }
        else if (MF_CAPTURE_ENGINE_PREVIEW_STARTED == type)
        {
            m_previewing.store(SUCCEEDED(status));
            PRINTLN("\n<< Preview started >>\n");
        }
        else if (MF_CAPTURE_ENGINE_PREVIEW_STOPPED == type)
        {
            m_previewing.store(SUCCEEDED(status) ? false : true);
            PRINTLN("\n<< Preview stopped >>\n");
        }
        else if (MF_CAPTURE_ENGINE_RECORD_STARTED == type)
        {
            m_encoding.store(SUCCEEDED(status));
            PRINTLN("\n<< Encoding started >>\n");
        }
        else if (MF_CAPTURE_ENGINE_RECORD_STOPPED == type)
        {
            m_encoding.store(SUCCEEDED(status) ? false : true);
            PRINTLN("\n<< Encoding stopped >>\n");
        }

        if (FAILED(status))
        {
            throw winrt::hresult_error(status);
        }

    }
    catch (...)
    {
        LOG_ERROR();
    }
}

void State::PreparePreview()
{
    CollectAvailableDeviceOutputs(PreviewOutputs, PreviewStream);
    winrt::com_ptr<IMFMediaType> type;
    winrt::check_hresult(EngineSource->GetCurrentDeviceMediaType(PreviewStream, type.put()));

    GUID subtype{};
    winrt::check_hresult(type->GetGUID(MF_MT_SUBTYPE, &subtype));
    FourCC fourcc(subtype.Data1);

    DWORD stream{};
    winrt::check_hresult(EnginePreviewSink->AddStream(PreviewStream, type.get(), nullptr, &stream));

    uint64_t frame_duration{};
    uint32_t frame_rate_num{};
    uint32_t frame_rate_den{};
    winrt::check_hresult(::MFGetAttributeRatio(type.get(), MF_MT_FRAME_RATE, &frame_rate_num, &frame_rate_den));
    winrt::check_hresult(::MFFrameRateToAverageTimePerFrame(frame_rate_num, frame_rate_den, &frame_duration));

    uint32_t frame_width{};
    uint32_t frame_height{};
    winrt::check_hresult(::MFGetAttributeSize(type.get(), MF_MT_FRAME_SIZE, &frame_width, &frame_height));

    PRINTLN("Preview stream: (%ws), frame size %ux%u at frame rate %u/%u (%.2f)",
        fourcc.Data,
        frame_width,
        frame_height,
        frame_rate_num,
        frame_rate_den,
        static_cast<float>(frame_rate_num) / frame_rate_den);

    PreviewCallback = winrt::make_self<FrameCb>(nullptr);
    PreviewCallback->ExpectedFrameDuration = winrt::TimeSpan{ static_cast<int64_t>(frame_duration) };
    winrt::check_hresult(EnginePreviewSink->SetSampleCallback(stream, PreviewCallback.get()));
    winrt::check_hresult(EnginePreviewSink->Prepare());
}

void State::PrepareEncode()
{
    CollectAvailableDeviceOutputs(EncodeOutputs, EncodeStream);
    winrt::com_ptr<IMFMediaType> type;
    winrt::check_hresult(EngineSource->GetCurrentDeviceMediaType(EncodeStream, type.put()));

    winrt::com_ptr<IMFMediaType> previewType;
    winrt::check_hresult(EngineSource->GetCurrentDeviceMediaType(PreviewStream, previewType.put()));

    uint32_t frame_width{};
    uint32_t frame_height{};
    winrt::check_hresult(::MFGetAttributeSize(type.get(), MF_MT_FRAME_SIZE, &frame_width, &frame_height));

    for (auto&& o : EncodeOutputs)
    {
        auto fps = static_cast<float>(o.RateNum) / o.RateDen;

        if (o.Width == frame_width &&
            o.Height == frame_height &&
            fps < 60.0f)
        {
            winrt::check_hresult(EngineSource->GetAvailableDeviceMediaType(EncodeStream, o.TypeIndex, type.put()));
            winrt::check_hresult(EngineSource->SetCurrentDeviceMediaType(EncodeStream, type.get()));
            break;
        }
    }


    GUID subtype{};
    winrt::check_hresult(type->GetGUID(MF_MT_SUBTYPE, &subtype));
    FourCC fourcc(subtype.Data1);

    DWORD stream{};
    winrt::check_hresult(EngineRecordSink->AddStream(EncodeStream, type.get(), nullptr, &stream));

    uint64_t frame_duration{};
    uint32_t frame_rate_num{};
    uint32_t frame_rate_den{};
    winrt::check_hresult(::MFGetAttributeRatio(type.get(), MF_MT_FRAME_RATE, &frame_rate_num, &frame_rate_den));
    winrt::check_hresult(::MFFrameRateToAverageTimePerFrame(frame_rate_num, frame_rate_den, &frame_duration));

    PRINTLN("Encoding stream: (%ws), frame size %ux%u at frame rate %u/%u (%.2f)",
        fourcc.Data,
        frame_width,
        frame_height,
        frame_rate_num, 
        frame_rate_den,
        static_cast<float>(frame_rate_num) / frame_rate_den);

    EncodeCallback = winrt::make_self<FrameCb>("Capture");
    EncodeCallback->ExpectedFrameDuration = winrt::TimeSpan{ static_cast<int64_t>(frame_duration) };
    winrt::check_hresult(EngineRecordSink->SetSampleCallback(stream, EncodeCallback.get()));
    winrt::check_hresult(EngineRecordSink->Prepare());
}

void State::TogglePreview()
{
    if (m_previewing.load())
    {
        Engine->StopPreview();
    }
    else
    {
        Engine->StartPreview();
    }
}

void State::ToggleEncode()
{
    if (m_encoding.load())
    {
        Engine->StopRecord(FALSE, FALSE);
    }
    else
    {
        Engine->StartRecord();
    }
}



/*
    Free functions
*/

winrt::com_ptr<IMFDXGIDeviceManager> CreateMFDXGIDeviceManager()
{
    constexpr D3D_FEATURE_LEVEL FeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1
    };

    constexpr auto Dx11DeviceCreationFlags =
        D3D11_CREATE_DEVICE_BGRA_SUPPORT |
        D3D11_CREATE_DEVICE_VIDEO_SUPPORT |
        D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS;

    winrt::com_ptr<ID3D11Device> device;
    winrt::com_ptr<ID3D11DeviceContext> ctx;
    winrt::check_hresult(::D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        Dx11DeviceCreationFlags,
        FeatureLevels,
        static_cast<uint32_t>(std::size(FeatureLevels)),
        D3D11_SDK_VERSION,
        device.put(),
        nullptr,
        ctx.put()));

    // Required for Media Foundation to work with the D3D11 device across multiple threads.
    ctx.as<ID3D11Multithread>()->SetMultithreadProtected(TRUE);

    uint32_t token{};
    winrt::com_ptr<IMFDXGIDeviceManager> device_manager;
    winrt::check_hresult(::MFLockDXGIDeviceManager(&token, device_manager.put()));
    winrt::check_hresult(device_manager->ResetDevice(device.get(), token));
    PRINTLN("IMFDXGIDeviceManager created successfully");
    //return nullptr;
    return device_manager;
}

winrt::com_ptr<IMFCaptureEngine> CreateCaptureEngine()
{
    winrt::com_ptr<IMFCaptureEngine> engine;
    auto factory = winrt::create_instance<IMFCaptureEngineClassFactory>(CLSID_MFCaptureEngineClassFactory);
    winrt::check_hresult(factory->CreateInstance(CLSID_MFCaptureEngine, __uuidof(IMFCaptureEngine), engine.put_void()));
    PRINTLN("IMFCaptureEngine created successfully");
    return engine;
}

winrt::com_ptr<IMFActivate> CreateVideoDevice(State& state)
{
    uint32_t num_devices{};
    IMFActivate** devices{ nullptr };
    winrt::com_ptr<IMFActivate> device;
    state.Settings->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    winrt::check_hresult(::MFEnumDeviceSources(state.Settings.get(), &devices, &num_devices));
    auto cleanup = [&]()
        {
            for (uint32_t i = 0; i < num_devices; ++i)
            {
                devices[i]->Release();
            }
            ::CoTaskMemFree(devices);
        };

    if (num_devices == 0)
    {
        PRINTLN("No video capture devices found");
        cleanup();
        return device;
    }

    PRINTLN("=======================================");
    PRINTLN(" Please choose video capture devices ");
    PRINTLN("=======================================");

    for (uint32_t i = 0; i < num_devices; ++i)
    {
        wchar_t* name{};
        uint32_t name_length{};
        devices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &name, &name_length);
        if (name)
        {
            PRINTLN("* [%u] %ws", i, name);
        }
        ::CoTaskMemFree(name);
    }

    char ch = tolower(_getch());
    int32_t choice = static_cast<int32_t>(ch - '0');

   // auto ret = ::scanf_s("%d", &choice);
    if (choice >= 0 && std::cmp_less(choice, num_devices))
    {
        device.copy_from(devices[choice]);
    }
    else
    {
        PRINTLN("Invalid choice");
    }

    cleanup();
    return device;
}

void Initialize(State& state)
{
    WINRT_VERIFY_(S_OK, state.Settings->SetUINT32(MF_LOW_LATENCY, TRUE));
    WINRT_VERIFY_(S_OK, state.Settings->SetUINT32(MF_CAPTURE_ENGINE_USE_VIDEO_DEVICE_ONLY, TRUE));
    WINRT_VERIFY_(S_OK, state.Settings->SetUINT32(MF_CAPTURE_ENGINE_ENABLE_CAMERA_STREAMSTATE_NOTIFICATION, FALSE));
    if (state.Manager)
    {
        WINRT_VERIFY_(S_OK, state.Settings->SetUnknown(MF_CAPTURE_ENGINE_D3D_MANAGER, state.Manager.get()));
    }
    auto cb = winrt::make<CaptureEngineCb>(&state);
    winrt::check_hresult(state.Engine->Initialize(cb.get(), state.Settings.get(), nullptr, state.VideoDevice.get()));
    PRINTLN("IMFCaptureEngine beginning initialization...");
}
