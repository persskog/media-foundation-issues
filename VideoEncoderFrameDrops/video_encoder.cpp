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
    WINRT_VERIFY_(S_OK, attr->SetUINT32(MF_CAPTURE_ENGINE_ENABLE_CAMERA_STREAMSTATE_NOTIFICATION, 1));
    WINRT_VERIFY_(S_OK, attr->SetUnknown(MF_CAPTURE_ENGINE_D3D_MANAGER, dxgiManager));
    WINRT_VERIFY_(S_OK, attr->SetUINT32(MF_CAPTURE_ENGINE_RECORD_SINK_VIDEO_MAX_UNPROCESSED_SAMPLES, 8));
    WINRT_VERIFY_(S_OK, attr->SetUINT32(MF_CAPTURE_ENGINE_RECORD_SINK_VIDEO_MAX_PROCESSED_SAMPLES,   8));
    return attr;
}

struct CaptureEngineCb : winrt::implements<CaptureEngineCb, IMFCaptureEngineOnEventCallback>
{
    explicit CaptureEngineCb(VideoEncoder* encoder) noexcept : m_encoder{ encoder->get_weak() } {}

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



/*
    VideoEncoder
*/

winrt::com_ptr<VideoEncoder> VideoEncoder::Create()
{
    return winrt::com_ptr<VideoEncoder>();
}

HRESULT __stdcall VideoEncoder::OnSample(IMFSample* sample) noexcept
{
    return E_NOTIMPL;
}

HRESULT __stdcall VideoEncoder::OnSynchronizedEvent(IMFMediaEvent* event) noexcept
{
    return E_NOTIMPL;
}

void VideoEncoder::OnCaptureEngineEvent(IMFMediaEvent* event)
{
    HRESULT status{};
    GUID type{};
    WINRT_VERIFY_(S_OK, event->GetStatus(&status));
    WINRT_VERIFY_(S_OK, event->GetExtendedType(&type));
    PrintLine("-> %s (0x%08lx)", AsString(type), status);

    if (MF_CAPTURE_ENGINE_PHOTO_TAKEN == type)
    {
        //m_components.PhotoSystem->OnPhotoTaken(status);
    }
    if (MF_CAPTURE_ENGINE_INITIALIZED == type)
    {
        //status = OnCaptureEngineInitialized(status, m_components);
    }
    else if (MF_CAPTURE_ENGINE_PREVIEW_STARTED == type)
    {
        //m_previewState.CheckStartStatus(status);
        //m_signal.signal_status(status);
    }
    else if (MF_CAPTURE_ENGINE_PREVIEW_STOPPED == type)
    {
        //status = OnCaptureEnginePreviewStopped(m_components);
        //VV_ASSERT(SUCCEEDED(status));
    }
    else if (MF_CAPTURE_ENGINE_RECORD_STARTED == type)
    {
        //m_encoderState.CheckStartStatus(status);
    }
    else if (MF_CAPTURE_ENGINE_RECORD_STOPPED == type)
    {
        //VV_ASSERT(SUCCEEDED(status));
    }
    else if (MF_CAPTURE_SINK_PREPARED == type)
    {
        //status = OnCaptureSinkPrepared(status, event);
    }
    else if (MF_CAPTURE_SOURCE_CURRENT_DEVICE_MEDIA_TYPE_SET == type)
    {
        //status = OnCaptureSourceDeviceMediaTypeSet(event, m_components);
    }
    else if (MF_CAPTURE_ENGINE_OUTPUT_MEDIA_TYPE_SET == type)
    {
        //status = OnCaptureSinkOutputMediaTypeSet(status, event, m_components);
    }
    else
    {
        //CheckForCameraStreamStateChanges(type);
    }
}

