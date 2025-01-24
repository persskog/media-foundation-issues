#include "pch.h"
#include "video_encoder.hpp"

static auto AsString(const GUID& event)
{
    VV_VAR_TO_STRING(event, MF_CAPTURE_ENGINE_INITIALIZED);
    VV_VAR_TO_STRING(event, MF_CAPTURE_ENGINE_PREVIEW_STARTED);
    VV_VAR_TO_STRING(event, MF_CAPTURE_ENGINE_PREVIEW_STOPPED);
    VV_VAR_TO_STRING(event, MF_CAPTURE_ENGINE_RECORD_STARTED);
    VV_VAR_TO_STRING(event, MF_CAPTURE_ENGINE_RECORD_STOPPED);
    VV_VAR_TO_STRING(event, MF_CAPTURE_SINK_PREPARED);
    VV_VAR_TO_STRING(event, MF_CAPTURE_ENGINE_ERROR);
    VV_VAR_TO_STRING(event, MF_CAPTURE_ENGINE_CAMERA_STREAM_BLOCKED);
    VV_VAR_TO_STRING(event, MF_CAPTURE_ENGINE_CAMERA_STREAM_UNBLOCKED);
    VV_VAR_TO_STRING(event, MF_CAPTURE_SOURCE_CURRENT_DEVICE_MEDIA_TYPE_SET);
    VV_VAR_TO_STRING(event, MF_CAPTURE_ENGINE_OUTPUT_MEDIA_TYPE_SET);
    VV_VAR_TO_STRING(event, MF_CAPTURE_ENGINE_PHOTO_TAKEN);
    VV_VAR_TO_STRING(event, MF_CAPTURE_ENGINE_ALL_EFFECTS_REMOVED);
    VV_VAR_TO_STRING(event, MF_CAPTURE_ENGINE_EFFECT_ADDED);
    VV_VAR_TO_STRING(event, MF_CAPTURE_ENGINE_EFFECT_REMOVED);
    return "Unknown event";
}

static winrt::com_ptr<IMFAttributes> CreateCaptureEngineSettings(::IUnknown* dxgiManager)
{
    winrt::com_ptr<IMFAttributes> attr;
    THROW_IF_FAILED(::MFCreateAttributes(attr.put(), 6));
    WINRT_VERIFY_(S_OK, attr->SetUINT32(MF_CAPTURE_ENGINE_USE_VIDEO_DEVICE_ONLY, TRUE));
    WINRT_VERIFY_(S_OK, attr->SetUnknown(MF_CAPTURE_ENGINE_D3D_MANAGER, dxgiManager));
    WINRT_VERIFY_(S_OK, attr->SetUINT32(MF_CAPTURE_ENGINE_RECORD_SINK_VIDEO_MAX_UNPROCESSED_SAMPLES, 1));
    WINRT_VERIFY_(S_OK, attr->SetUINT32(MF_CAPTURE_ENGINE_RECORD_SINK_VIDEO_MAX_PROCESSED_SAMPLES,   1));
    return attr;
}

static bool CopyAttribute(const GUID& key, IMFAttributes* src, IMFAttributes* dst)
{
    PROPVARIANT var{};
    if (SUCCEEDED(src->GetItem(key, &var)))
    {
        LOG_IF_FAILED(dst->SetItem(key, var));
        ::PropVariantClear(&var);
        return true;
    }
    return false;
}

static HRESULT CreateMediaTypeAndEncoderParameters(IMFMediaType*   deviceType,
                                                   IMFMediaType**  type,
                                                   IMFAttributes** params)
{
    RETURN_IF_FAILED(::MFCreateMediaType(type));
    RETURN_IF_FAILED(::MFCreateAttributes(params, 4));

    // Encoding type
    (*type)->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    (*type)->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    (*type)->SetUINT32(MF_MT_VIDEO_PROFILE, 100);
    (*type)->SetUINT32(MF_MT_AVG_BITRATE, 1024 * 1024 * 15);
    (*type)->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    CopyAttribute(MF_MT_FRAME_SIZE, deviceType, *type);
    CopyAttribute(MF_MT_FRAME_RATE, deviceType, *type);
    if (!CopyAttribute(MF_MT_PIXEL_ASPECT_RATIO, deviceType, *type))
    {
        ::MFSetAttributeRatio(*type, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    }
    // GOP length
    uint32_t num = {}, den = {};
    LOG_IF_FAILED(::MFGetAttributeRatio(*type, MF_MT_FRAME_RATE, &num, &den));
    uint64_t avgTimePerFrame{};
    LOG_IF_FAILED(::MFFrameRateToAverageTimePerFrame(num, den, &avgTimePerFrame));
    const double fps = num / static_cast<double>(den);
    const auto wholeFrames = static_cast<uint32_t>(fps);
    //const auto scale = static_cast<double>(VideoFrameAnalyzer::HundredNanoSecondsInSecond) / HundredNanoSecondsInSecond;
    const auto gopLength = static_cast<uint32_t>(wholeFrames);
    (*type)->SetUINT32(MF_MT_MAX_KEYFRAME_SPACING, gopLength);

    // Encoder settings
    (*params)->SetUINT32(MF_LOW_LATENCY, TRUE);
    (*params)->SetUINT32(CODECAPI_AVEncMPVGOPSize, gopLength);
    return S_OK;
}

struct Callback : winrt::implements<Callback, IMFCaptureEngineOnEventCallback>
{
    explicit Callback(VideoEncoder* encoder) noexcept : m_encoder{ encoder->get_weak() } {}

    HRESULT __stdcall OnEvent(IMFMediaEvent* event) noexcept final
    {
        if (auto engine = m_encoder.get())
        {
            engine->OnCaptureEngineEvent(event);
        }
        return S_OK;
    }

    winrt::weak_ref<VideoEncoder> m_encoder;
};

constexpr auto PhotoSourceStream = static_cast<DWORD>(MF_CAPTURE_ENGINE_PREFERRED_SOURCE_STREAM_FOR_PHOTO);
constexpr auto RecordSourceStream = static_cast<DWORD>(MF_CAPTURE_ENGINE_PREFERRED_SOURCE_STREAM_FOR_VIDEO_RECORD);


int32_t VideoFrameAnalyzer::Analyze(IMFSample* frame)
{
    using namespace std::chrono;
    WINRT_ASSERT(frame);

    ++m_frameCounter;
    if (IsInvalidTime(m_prevDts))
    {
        OnFirstFrame(frame);
        return 0;
    }

    uint32_t discontinuity{};
    uint32_t lostFramesSinceLastValid{};
    frame->GetUINT32(MFSampleExtension_Discontinuity, &discontinuity);
    const auto duration = GetSampleDuration(frame);
    const auto dts = GetDts(frame);
    // Should be around average time per frame for the current frame rate
    const auto delta = dts - m_prevDts;

    if (discontinuity)
    {
        lostFramesSinceLastValid = CalculateLostFrames(delta, duration);
        m_accumLostFrames += lostFramesSinceLastValid;
        PrintLine("[Discontinuity] %lld (%lldms) ~%u lost since last valid (total lost %u)",
            delta.count(), 
            duration_cast<milliseconds>(delta).count(),
            lostFramesSinceLastValid,
            m_accumLostFrames);
    }
    else
    {
        auto delay = CheckForIncreasedFrameDelay(delta);
        m_accumDuration += delay;
    }

    m_accumDuration += duration;
    ValidateFrameCounter(duration);
    m_prevDts = dts;
    return lostFramesSinceLastValid;
}

void VideoFrameAnalyzer::Reset()
{
    m_prevDts = InvalidTime;
    m_clockStart = InvalidTime;
    m_delayUntilFirstFrame = InvalidTime;
    m_frameCounter = {};
    m_approxLostFrames = {};
}

void VideoFrameAnalyzer::OnFirstFrame(IMFSample* frame)
{
    using namespace std::chrono;
    const auto now = VideoFrameAnalyzer::Now();
    const auto dts = GetDts(frame);
    const auto duration = GetSampleDuration(frame);
    const auto delay = now - dts;

    uint32_t keyframe{};
    frame->GetUINT32(MFSampleExtension_CleanPoint, &keyframe);
    WINRT_ASSERT(keyframe);

    m_clockStart = now - delay;
    m_prevDts = dts;
    m_delayUntilFirstFrame = delay;
    m_accumLostFrames = 0;
    m_accumDuration += duration;
    PrintLine("[First frame] %lld, %lldms delay since captured", dts.count(), AsMilliseconds(delay));
}

winrt::TimeSpan VideoFrameAnalyzer::ElapsedTimeSinceFirstFrame() const
{
    return Now() - m_clockStart;
}

bool VideoFrameAnalyzer::IsDtsDeltaOk(winrt::TimeSpan delta, winrt::TimeSpan frameDuration)
{
    const auto abs = std::chrono::abs(delta - frameDuration);
    return abs <= DeltaLimit;
}

void VideoFrameAnalyzer::ValidateFrameCounter(winrt::TimeSpan frameDuration) const
{
    using namespace std::chrono;
    const auto elapsed_clock = ElapsedTimeSinceFirstFrame();
    const auto diff_duration = elapsed_clock - m_accumDuration;
 /*   PrintLine("[%lld] : %lld (%lld ms)",
        elapsed_clock.count(),
        m_accumDuration.count(),
        duration_cast<milliseconds>(diff_duration).count());*/

    const auto num_frames_clock_based = elapsed_clock.count() / static_cast<double>(frameDuration.count());
    const auto num_frames_duration_based = m_accumDuration.count() / static_cast<double>(frameDuration.count());
    const uint32_t counter = m_frameCounter + m_accumLostFrames;
    const uint32_t approx = static_cast<uint32_t>(num_frames_clock_based);
    if (approx > counter)
    {
        PrintLine("%u <-> %.2lf | clock vs. accum: %lldms (%lld)",
            counter, 
            num_frames_clock_based,
            AsMilliseconds(diff_duration),
            frameDuration);
    }
}

void VideoFrameAnalyzer::Initialize(IMFMediaType* type)
{
    uint32_t num = {}, den = {};
    LOG_IF_FAILED(::MFGetAttributeRatio(type, MF_MT_FRAME_RATE, &num, &den));
    uint64_t avgTimePerFrame{};
    LOG_IF_FAILED(::MFFrameRateToAverageTimePerFrame(num, den, &avgTimePerFrame));
    m_avgTimePerFrame = winrt::TimeSpan{ avgTimePerFrame };
}

winrt::TimeSpan VideoFrameAnalyzer::CheckForIncreasedFrameDelay(winrt::TimeSpan delta)
{
    using namespace std::chrono_literals;

    // The frame took longer than the excepted time to arrive.
    // Webcameras can have increased it's shutter time and therefore
    // the capture time increases
    // Not really lost
    const auto delay = delta - m_avgTimePerFrame;
    const auto ms = AsMilliseconds(delay);
    if (delta > m_avgTimePerFrame)
    {
        if (delay > 5ms)
        {
            PrintLine("[%u] High delay between frames %lldms", m_frameCounter, ms);
        }
        return delay;
    }
    else
    {
        if (ms != 0)
        {
            PrintLine("[%u] %lldms", m_frameCounter, ms);
        }
    }
    return {};
}

uint32_t VideoFrameAnalyzer::CalculateLostFrames(winrt::TimeSpan delta, winrt::TimeSpan frameDuration)
{
    auto lost = delta.count() / static_cast<double>(frameDuration.count());
    auto count = std::round(lost); 
    // Do not include current frame, hence -1
    return static_cast<uint32_t>(count) - 1;
}

winrt::TimeSpan VideoFrameAnalyzer::GetSampleTime(IMFSample* sample)
{
    int64_t ts{};
    sample->GetSampleTime(&ts);
    return winrt::TimeSpan(ts);
}

winrt::TimeSpan VideoFrameAnalyzer::GetSampleDuration(IMFSample* sample)
{
    int64_t ts{};
    sample->GetSampleDuration(&ts);
    return winrt::TimeSpan(ts);
}

winrt::TimeSpan VideoFrameAnalyzer::GetDts(IMFSample* sample)
{
    uint64_t dts{};
    sample->GetUINT64(MFSampleExtension_DeviceTimestamp, &dts);
    return winrt::TimeSpan(dts);
}

winrt::TimeSpan VideoFrameAnalyzer::Now()
{
    return winrt::TimeSpan{ ::MFGetSystemTime() };
}

int64_t VideoFrameAnalyzer::AsMilliseconds(winrt::TimeSpan ts)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(ts).count();
}

bool VideoFrameAnalyzer::IsInvalidTime(winrt::TimeSpan ts)
{
    return ts < winrt::TimeSpan::zero();
}



/*
    VideoEncoder
*/

VideoEncoder::VideoEncoder(const winrt::com_ptr<IMFCaptureEngine>& engine)
    : m_ready {::CreateEventA(nullptr, 0, 0, nullptr) }
    , m_engine{ engine }
{
}

winrt::com_ptr<VideoEncoder> VideoEncoder::Create(IMFActivate* videoDevice, ID3D11Device* device)
{
    // Enable GPU acceleration
    uint32_t token{};
    winrt::com_ptr<IMFDXGIDeviceManager> dxgiManager;
    THROW_IF_FAILED(::MFLockDXGIDeviceManager(&token, dxgiManager.put()));
    THROW_IF_FAILED(dxgiManager->ResetDevice(device, token));
    THROW_IF_FAILED(::MFUnlockDXGIDeviceManager());

    // Create capture engine
    auto factory = winrt::create_instance<IMFCaptureEngineClassFactory>(CLSID_MFCaptureEngineClassFactory);
    winrt::com_ptr<IMFCaptureEngine> engine;
    THROW_IF_FAILED(factory->CreateInstance(CLSID_MFCaptureEngine, winrt::guid_of<IMFCaptureEngine>(), engine.put_void()));
    auto encoder = winrt::make_self<VideoEncoder>(engine);
    auto cb = winrt::make<Callback>(encoder.get());
    auto settings = CreateCaptureEngineSettings(dxgiManager.get());
    THROW_IF_FAILED(engine->Initialize(cb.get(), settings.get(), nullptr, videoDevice));
    THROW_IF_FAILED(encoder->AwaitReady());
    return encoder;
}

HRESULT __stdcall VideoEncoder::OnSample(IMFSample* sample) noexcept
{
    if (!sample)
    {
        return S_OK;
    }
    auto count = m_analyzer.Analyze(sample);

    return S_OK;
}

HRESULT __stdcall VideoEncoder::OnSynchronizedEvent(IMFMediaEvent*) noexcept
{
    return S_OK;
}

void VideoEncoder::OnCaptureEngineEvent(IMFMediaEvent* event)
{
    HRESULT status{};
    GUID type{};
    WINRT_VERIFY_(S_OK, event->GetStatus(&status));
    WINRT_VERIFY_(S_OK, event->GetExtendedType(&type));
    if (FAILED(status))
    {
        PrintLine("-> %s (0x%08lx)", AsString(type), status);
        WINRT_ASSERT(false);
    }
    if (MF_CAPTURE_ENGINE_INITIALIZED == type)
    {
        status = OnInitialized(status);
    }
    else if (MF_CAPTURE_ENGINE_RECORD_STARTED == type)
    {
        WINRT_ASSERT(SUCCEEDED(status));
    }
    else if (MF_CAPTURE_ENGINE_RECORD_STOPPED == type)
    {
        WINRT_ASSERT(SUCCEEDED(status));
    }
    else if (MF_CAPTURE_SINK_PREPARED == type)
    {
        status = OnSinkPrepared(status);
    }

    if (FAILED(status))
    {
        SignalStatus(status);
    }
}

void VideoEncoder::StartEncoder()
{
    LOG_IF_FAILED(m_engine->StartRecord());
}

void VideoEncoder::StopEncoder()
{
    LOG_IF_FAILED(m_engine->StopRecord(FALSE, TRUE));
}

void VideoEncoder::TakePhoto()
{
    auto sink = GetPhotoSink();
    if (m_photoSink == 0xffffffff)
    {
        auto source = GetSource();
        winrt::com_ptr<IMFMediaType> deviceType;
        THROW_IF_FAILED(source->GetCurrentDeviceMediaType(PhotoSourceStream, deviceType.put()));
        winrt::com_ptr<IMFMediaType> photoType;
        THROW_IF_FAILED(::MFCreateMediaType(photoType.put()));
        CopyAttribute(MF_MT_FRAME_SIZE, deviceType.get(), photoType.get());
        photoType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Image);
        photoType->SetGUID(MF_MT_SUBTYPE, GUID_ContainerFormatJpeg);
        THROW_IF_FAILED(sink->AddStream(PhotoSourceStream, photoType.get(), nullptr, &m_photoSink));
    }
    THROW_IF_FAILED(sink->SetOutputFileName(L"photo.jpeg"));
    THROW_IF_FAILED(m_engine->TakePhoto());
}

HRESULT VideoEncoder::AwaitReady() const noexcept
{
    if (WAIT_OBJECT_0 != ::WaitForSingleObject(m_ready.get(), INFINITE))
    {
        return MF_E_UNEXPECTED;
    }
    return m_status;
}

void VideoEncoder::SignalStatus(HRESULT status)
{
    m_status = status;
    ::SetEvent(m_ready.get());
}

winrt::com_ptr<IMFCaptureSource> VideoEncoder::GetSource() const
{
    winrt::com_ptr<IMFCaptureSource> source;
    LOG_IF_FAILED(m_engine->GetSource(source.put()));
    return source;
}

winrt::com_ptr<IMFCaptureRecordSink> VideoEncoder::GetRecordSink() const
{
    winrt::com_ptr<IMFCaptureSink> sink;
    LOG_IF_FAILED(m_engine->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_RECORD, sink.put()));
    return sink.try_as<IMFCaptureRecordSink>();
}

winrt::com_ptr<IMFCapturePhotoSink> VideoEncoder::GetPhotoSink() const
{
    winrt::com_ptr<IMFCaptureSink> sink;
    LOG_IF_FAILED(m_engine->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_PHOTO, sink.put()));
    return sink.try_as<IMFCapturePhotoSink>();
}

winrt::com_ptr<IMFTransform> VideoEncoder::GetEncoderMFT(IMFCaptureSink* sink) noexcept
{
    winrt::com_ptr<::IUnknown> unk;
    if (FAILED(sink->GetService(0, {}, winrt::guid_of<IMFSinkWriter>(), unk.put())))
    {
        return {};
    }

    if (auto writer = unk.try_as<IMFSinkWriterEx>())
    {
        winrt::com_ptr<IMFTransform> mft;
        DWORD i{};
        GUID category{};
        while (SUCCEEDED(writer->GetTransformForStream(0, i++, &category, mft.put())))
        {
            if (MFT_CATEGORY_VIDEO_ENCODER == category)
            {
                return mft;
            }
        }
    }
    return {};
}

HRESULT VideoEncoder::OnInitialized(HRESULT status)
{
    RETURN_IF_FAILED(status);
    DWORD sinkStream{};
    auto source = GetSource();
    winrt::com_ptr<IMFMediaType> deviceType;
    RETURN_IF_FAILED(source->GetCurrentDeviceMediaType(RecordSourceStream, deviceType.put()));
    winrt::com_ptr<IMFMediaType> encodingType;
    winrt::com_ptr<IMFAttributes> encoderSettings;
    RETURN_IF_FAILED(CreateMediaTypeAndEncoderParameters(deviceType.get(), encodingType.put(), encoderSettings.put()));

    encoderSettings->GetUINT32(CODECAPI_AVEncMPVGOPSize, &m_gopLength);


    auto sink = GetRecordSink();
    RETURN_IF_FAILED(sink->AddStream(RecordSourceStream, encodingType.get(), nullptr, &sinkStream));
    RETURN_IF_FAILED(sink->SetSampleCallback(sinkStream, this));
    RETURN_IF_FAILED(sink->Prepare());
    return S_OK;
}

HRESULT VideoEncoder::OnSinkPrepared(HRESULT status)
{
    RETURN_IF_FAILED(status);

    auto sink = GetRecordSink();
    winrt::com_ptr<IMFMediaType> type;
    RETURN_IF_FAILED(sink->GetOutputMediaType(0, type.put()));

    // Low latency
    m_encoderMFT = GetEncoderMFT(sink.get());
    auto codec = m_encoderMFT.try_as<ICodecAPI>();
    VARIANT var{};
    var.vt = VT_BOOL;
    var.boolVal = VARIANT_TRUE;
    RETURN_IF_FAILED(codec->SetValue(&CODECAPI_AVLowLatencyMode, &var));

    var.vt = VT_UI4;
    var.ulVal = 0;
    RETURN_IF_FAILED(codec->SetValue(&CODECAPI_AVEncMPVDefaultBPictureCount, &var));


    LOG_IF_FAILED(codec->GetValue(&CODECAPI_AVEncMPVGOPSize, &var));
    if (var.ulVal != m_gopLength)
    {
        var.ulVal = m_gopLength;
        LOG_IF_FAILED(codec->SetValue(&CODECAPI_AVEncMPVGOPSize, &var));
    }

    //CODECAPI_AVEncMPVGOPSize
    //AVEncMPVDefaultBPictureCount
    m_analyzer.Initialize(type.get());
    SignalStatus(S_OK);
    return S_OK;
}

