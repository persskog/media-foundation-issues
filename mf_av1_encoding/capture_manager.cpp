#include "pch.h"
#include "capture_manager.hpp"
#include "mf_attributes.hpp"

#include "av1_utils.hpp"

using namespace winrt;

#include <mfapi.h>
#include <mfobjects.h>
#include <mferror.h>
#include <vector>
#include <cstdint>

// Helper to write big-endian 32-bit
void WriteBE32(std::vector<uint8_t>& buf, uint32_t value)
{
    buf.push_back((value >> 24) & 0xFF);
    buf.push_back((value >> 16) & 0xFF);
    buf.push_back((value >> 8) & 0xFF);
    buf.push_back(value & 0xFF);
}

// Helper to write 4CC
void WriteFourCC(std::vector<uint8_t>& buf, const char* fourcc) 
{
    buf.insert(buf.end(), fourcc, fourcc + 4);
}

HRESULT BuildAV1SampleDescription(IMFMediaType* pMediaType,
    const uint8_t* obuData,
    size_t obuSize,
    uint16_t width,
    uint16_t height) {
    std::vector<uint8_t> stsd;

    // ---- av1C box ----
    std::vector<uint8_t> av1C;
    WriteBE32(av1C, 12 + (uint32_t)obuSize); // size
    WriteFourCC(av1C, "av1C");
    av1C.push_back(0x81); // marker(1) + version(1)
    av1C.push_back(0x00); // seq_profile
    av1C.push_back(0x00); // seq_level_idx_0
    av1C.push_back(0x00); // bit_depth + flags
    av1C.insert(av1C.end(), obuData, obuData + obuSize); // configOBUs

    // ---- av01 sample entry ----
    std::vector<uint8_t> av01;
    uint32_t av01Size = 86 + (uint32_t)av1C.size(); // minimal size
    WriteBE32(av01, av01Size);
    WriteFourCC(av01, "av01");
    av01.insert(av01.end(), 6, 0); // reserved
    av01.push_back(0); av01.push_back(1); // data_reference_index
    av01.insert(av01.end(), 16, 0); // pre-defined + reserved
    av01.push_back((width >> 8) & 0xFF);
    av01.push_back(width & 0xFF);
    av01.push_back((height >> 8) & 0xFF);
    av01.push_back(height & 0xFF);
    av01.insert(av01.end(), 50, 0); // rest of visual sample entry
    av01.insert(av01.end(), av1C.begin(), av1C.end());

    // ---- stsd box ----
    uint32_t stsdSize = 16 + (uint32_t)av01.size();
    WriteBE32(stsd, stsdSize);
    WriteFourCC(stsd, "stsd");
    stsd.insert(stsd.end(), { 0,0,0,0 }); // version/flags
    WriteBE32(stsd, 1); // entry_count
    stsd.insert(stsd.end(), av01.begin(), av01.end());

    // Set on media type
    return pMediaType->SetBlob(MF_MT_MPEG4_SAMPLE_DESCRIPTION, stsd.data(), (UINT32)stsd.size());
}


CaptureManager::CaptureManager() : Engine{ CreateCaptureEngine() }
{
}

HRESULT __stdcall CaptureManager::OnSample(IMFSample* sample) noexcept
{
    // Only try to create the descriptor if we haven't yet, and this is a Key Frame (CleanPoint)
    if (!m_sampleDescriptorBoxWritten &&
        sample->GetItem(MFSampleExtension_CleanPoint, nullptr) == S_OK)
    {
        com_ptr<IMFMediaBuffer> media_buf;
        if (SUCCEEDED(sample->GetBufferByIndex(0, media_buf.put())))
        {
            uint8_t* payload{ nullptr };
            DWORD len{};
            if (SUCCEEDED(media_buf->Lock(&payload, nullptr, &len)))
            {
                // Pass span to helper
                m_sampleDescriptorBoxWritten = CreateSampleDescriptorBoxIfNeeded({ payload, len });
                media_buf->Unlock();
            }
        }
    }

    if (m_writer)
    {
        sample->SetSampleTime(m_duration);
        int64_t duration{};
        sample->GetSampleDuration(&duration);

        //... Pass sample to writer ...
        m_writer->WriteSample(0, sample);
        m_duration += duration;
    }

    return S_OK;
}

bool CaptureManager::CreateSampleDescriptorBoxIfNeeded(std::span<const uint8_t> payload) noexcept
{
    if (!m_mediaType)
        return false;

    // CreateAV1C likely returns an av1C box (size + "av1C" + 4 header bytes + configOBUs).
    // BuildAV1SampleDescription expects raw config OBUs (it will build the av1C box itself).
    std::vector<uint8_t> av1cBlob = AV1Helper::CreateAV1C(payload);

    if (av1cBlob.empty())
        return false;

    const uint8_t* obuData = nullptr;
    size_t obuSize = 0;

    // Detect av1C box header and skip it if present (size(4) + "av1C"(4) + 4 bytes header = 12)
    if (av1cBlob.size() > 12 && std::memcmp(av1cBlob.data() + 4, "av1C", 4) == 0)
    {
        obuData = av1cBlob.data() + 12;
        obuSize = av1cBlob.size() - 12;
    }
    else
    {
        // If helper already returned just the configOBUs, use them directly.
        obuData = av1cBlob.data();
        obuSize = av1cBlob.size();
    }

    // Get frame size for sample entry (width/height)
    UINT32 width = 0, height = 0;
    HRESULT hrSize = MFGetAttributeSize(m_mediaType.get(), MF_MT_FRAME_SIZE, &width, &height);
    if (FAILED(hrSize))
    {
        // Can't build a correct av01 sample entry without frame size
        return false;
    }

    // Build and set the full 'stsd' sample description on the media type
    HRESULT hr = BuildAV1SampleDescription(m_mediaType.get(), obuData, obuSize, static_cast<uint16_t>(width), static_cast<uint16_t>(height));
    if (FAILED(hr))
        return false;

    // Set current sample entry index
    m_mediaType->SetUINT32(MF_MT_MPEG4_CURRENT_SAMPLE_ENTRY, 0);

    // Update/ recreate writer as needed
    if (m_writer)
    {
        // Update input media type on existing writer (common requirement)
        HRESULT hrSet = m_writer->SetInputMediaType(0, m_mediaType.get(), nullptr);
        if (FAILED(hrSet))
        {
            // If update fails, try recreating the writer
            CreateWriter();
        }
    }
    else
    {
        CreateWriter();
    }

    ForceKeyFrame();
    return true;
}

HRESULT __stdcall CaptureManager::OnSynchronizedEvent(IMFMediaEvent* event) noexcept
{
    return S_OK;
}

void CaptureManager::Initialize(IMFAttributes* attr, ::IUnknown* d3dManager, ::IUnknown* videoSource)
{
    auto cb = winrt::make<CaptureEngineCb>(this);
    attr->SetUnknown(MF_CAPTURE_ENGINE_D3D_MANAGER, d3dManager);
    attr->SetUINT32(MF_CAPTURE_ENGINE_USE_VIDEO_DEVICE_ONLY, TRUE);
    winrt::check_hresult(Engine->Initialize(cb.get(), attr, nullptr, videoSource));
}



void CaptureManager::PrepareAv1Encoding() noexcept
{
    constexpr auto stream = static_cast<DWORD>(MF_CAPTURE_ENGINE_PREFERRED_SOURCE_STREAM_FOR_VIDEO_RECORD);

    winrt::com_ptr<IMFCaptureSource> source;
    winrt::com_ptr<IMFCaptureRecordSink> sink;

    winrt::check_hresult(Engine->GetSource(source.put()));
    winrt::check_hresult(Engine->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_RECORD, reinterpret_cast<IMFCaptureSink**>(sink.put())));

    com_ptr<IMFMediaType> srcType;
    winrt::check_hresult(source->GetCurrentDeviceMediaType(stream, srcType.put()));

    com_ptr<IMFMediaType> av1Type;
    winrt::check_hresult(MFCreateMediaType(av1Type.put()));
    winrt::check_hresult(av1Type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
    winrt::check_hresult(av1Type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_AV1));
    CopyAttribute(MF_MT_FRAME_SIZE, srcType.get(), av1Type.get());
    CopyAttribute(MF_MT_FRAME_RATE, srcType.get(), av1Type.get());
    av1Type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    av1Type->SetUINT32(MF_MT_AVG_BITRATE, 1u * 1024u * 1024u);
    ::MFSetAttributeRatio(av1Type.get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

    DWORD sinkStreamIndex{};
    winrt::check_hresult(sink->AddStream(stream, av1Type.get(), nullptr, &sinkStreamIndex));
    winrt::check_hresult(sink->SetSampleCallback(sinkStreamIndex, this));
    winrt::check_hresult(sink->Prepare());
}

void CaptureManager::OnRecordSinkPrepared() noexcept
{
    winrt::com_ptr<IMFCaptureRecordSink> sink;
    winrt::check_hresult(Engine->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_RECORD, reinterpret_cast<IMFCaptureSink**>(sink.put())));
    winrt::check_hresult(sink->GetOutputMediaType(0, m_mediaType.put()));

    winrt::com_ptr<IUnknown> unk;
    winrt::check_hresult(sink->GetService(0, {}, winrt::guid_of<IMFSinkWriter>(), unk.put()));
    auto writer = unk.as<IMFSinkWriterEx>();
    winrt::com_ptr<IMFTransform> mft;
    DWORD i{};
    GUID category{};
    while (SUCCEEDED(writer->GetTransformForStream(0, i++, &category, mft.put())))
    {
        if (MFT_CATEGORY_VIDEO_ENCODER == category)
        {
            m_encoder = mft.as<ICodecAPI>();
            break;
        }
    }
}

void CaptureManager::ForceKeyFrame() const noexcept
{
    if (m_encoder)
    {
        VARIANT var{};
        var.vt = VT_UI4;
        var.ulVal = 1;
        m_encoder->SetValue(&CODECAPI_AVEncVideoForceKeyFrame, &var);
    }
}

void CaptureManager::CreateWriter()
{
    com_ptr<IMFSinkWriter> writer;
    HRESULT hr = ::MFCreateSinkWriterFromURL(
        L"D:\\source\\mf_av1_encoding\\av1.mp4",
        nullptr,
        nullptr,
        writer.put());
    DWORD avs{};
    WINRT_VERIFY_(S_OK, writer->AddStream(m_mediaType.get(), &avs));
    WINRT_VERIFY_(S_OK, writer->SetInputMediaType(avs, m_mediaType.get(), nullptr));
    WINRT_VERIFY_(S_OK, writer->BeginWriting());
    m_writer = writer;
}

void CaptureManager::OnEvent(IMFMediaEvent* event) noexcept
{
    GUID type{};
    HRESULT hr{};
    event->GetExtendedType(&type);
    event->GetStatus(&hr);
    if (MF_CAPTURE_ENGINE_INITIALIZED == type)
    {
        PrintLine("Capture Engine Initialized (0x%08lx)", hr);
        PrepareAv1Encoding();
    }
    else if (MF_CAPTURE_ENGINE_RECORD_STARTED == type)
    {
        PrintLine("Capture Engine Record Started (0x%08lx)", hr);
    }
    else if (MF_CAPTURE_ENGINE_RECORD_STOPPED == type)
    {
        PrintLine("Capture Engine Record Stopped (0x%08lx)", hr);
        hr = m_writer->Finalize();

        int h00 = 0;
    }
    else if (MF_CAPTURE_SINK_PREPARED == type)
    {
        PrintLine("Capture Sink Prepared (0x%08lx)", hr);
        OnRecordSinkPrepared();
    }
    else if (MF_CAPTURE_ENGINE_ERROR == type)
    {
        PrintLine("Capture Engine Error (0x%08lx)", hr);
    }
    else if (MF_CAPTURE_SOURCE_CURRENT_DEVICE_MEDIA_TYPE_SET == type)
    {
        PrintLine("Capture Source Current Device Media Type Set (0x%08lx)", hr);
    }
}
