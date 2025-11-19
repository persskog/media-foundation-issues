#pragma once
#include <initguid.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mfcaptureengine.h>
#include <mfreadwrite.h>
#include <codecapi.h>
#include <ks.h>
#include <ksmedia.h>
#include <d3d11.h>
#include <string>

#include "string_builder.hpp"

#define VV_STRINGIFY(val) #val
#define VV_VAR_TO_STRING(var, val) \
    if (var == val)                \
        return VV_STRINGIFY(val)


DEFINE_GUID(CLSID_VideoInputDeviceCategory, 0x860BB310, 0x5D01, 0x11d0, 0xBD, 0x3B, 0x00, 0xA0, 0xC9, 0x11, 0xCE, 0x86);
DEFINE_GUID(FORMAT_VideoInfo, 0x05589f80, 0xc356, 0x11ce, 0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a);
DEFINE_GUID(MF_DEVICESOURCE_ATTRIBUTE_ENABLE_PLUGIN, 0x3c69b62a, 0xa424, 0x415f, 0x83, 0x03, 0x5b, 0xfb, 0xa9, 0x4b, 0xec, 0x9b);
DEFINE_GUID(MF_DEVSOURCE_ATTRIBUTE_DEVICETYPE, 0x3c69b62a, 0xa424, 0x415f, 0x83, 0x03, 0x5b, 0xfb, 0xa9, 0x4b, 0xec, 0x9b);
DEFINE_GUID(MF_DEVSOURCE_ATTRIBUTE_USE_DSHOWBRIDGE, 0x053f5e22, 0xcc2f, 0x43bc, 0xaa, 0x76, 0x6f, 0xf5, 0x97, 0x1c, 0x71, 0x31);
DEFINE_GUID(MF_DEVPROXY_ATTRIBUTE_DEVSOURCE_MODE, 0x645dc2b3, 0xf804, 0x4067, 0x9a, 0x53, 0x13, 0x73, 0xc2, 0xf0, 0x39, 0xb2);
DEFINE_GUID(MF_TELEMETRY_SESSION_OBJECT_ATTRIBUTE, 0x2acf1917, 0x3743, 0x41df, 0xa5, 0x64, 0xe7, 0x27, 0xa8, 0x0e, 0xa3, 0x3e);
DEFINE_GUID(MF_MT_D3D_DECODE_PROFILE_GUID, 0x657c3e17, 0x3341, 0x41a7, 0x9a, 0xe6, 0x37, 0xa9, 0xd6, 0x99, 0x85, 0x1f);
//DEFINE_GUID(MF_SA_D3D11_ALLOCATE_DISPLAYABLE_RESOURCES, 0xeeface6d, 0x2ea9, 0x4adf, 0xbb, 0xdf, 0x7b, 0xbc, 0x48, 0x2a, 0x1b, 0x6d);
DEFINE_GUID(MF_FRAMESERVER_CLIENTCONTEXT_CLIENTPID, 0x5f8d322e, 0x0fe4, 0x43e4, 0x9e, 0x50, 0xd8, 0x3e, 0xcd, 0x9f, 0xc2, 0xb8);
DEFINE_GUID(MEDIA_TELEMETRY_SESSION_ID, 0x2acf1917, 0x3743, 0x41df, 0xa5, 0x64, 0xe7, 0x27, 0xa8, 0x0e, 0xa3, 0x3d);
DEFINE_GUID(MF_VIRTUALCAMERA_PROVIDE_ASSOCIATED_CAMERA_SOURCES, 0xf0273718, 0x4a4d, 0x4ac5, 0xa1, 0x5d, 0x30, 0x5e, 0xb5, 0xe9, 0x06, 0x67);
DEFINE_GUID(MF_VIRTUALCAMERA_ASSOCIATED_CAMERA_SOURCES, 0x1bb79e7c, 0x5d83, 0x438c, 0x94, 0xd8, 0xe5, 0xf0, 0xdf, 0x6d, 0x32, 0x79);
DEFINE_GUID(MF_DEVICESTREAM_ATTRIBUTE_DEFAULT_DEVICE_STREAM, 0x2fe6bb9b, 0x2955, 0x46b6, 0x85, 0xd6, 0x90, 0x04, 0xec, 0xdf, 0xd3, 0xd2);
DEFINE_GUID(MF_STREAM_DEVICECONTROL_SOURCETOKEN, 0x88afeff5, 0xee0f, 0x43d3, 0x91, 0x6a, 0xbe, 0xc1, 0xff, 0x11, 0x19, 0xe9);
DEFINE_GUID(MF_BYTESTREAM_DOWNLOADRATE_SERVICE, 0xb1699c90, 0x7b31, 0x486e, 0x88, 0x0e, 0x68, 0x2d, 0xc3, 0xc3, 0x7b, 0x7b);
DEFINE_GUID(MF_BYTESTREAM_OPTICAL_MEDIA, 0x788e61cb, 0x987b, 0x4ffc, 0x9e, 0x38, 0xde, 0xfd, 0x17, 0xfe, 0x03, 0x50);
DEFINE_GUID(MF_MT_FSSourceTypeDecoded, 0xea031a62, 0x8bbb, 0x43c5, 0xb5, 0xc4, 0x57, 0x2d, 0x2d, 0x23, 0x1c, 0x18);
DEFINE_GUID(FSMediaType_CompressedPassthrough, 0x49EEB855, 0x1076, 0x4C9F, 0xB8, 0xAE, 0x6F, 0x4E, 0x04, 0x92, 0x2B, 0xC5);

DEFINE_GUID(MFSampleExtension_FSMediaTypeInfo, 0x03ffdcb2, 0xc32c, 0x4b92, 0x98, 0xff, 0x87, 0xc0, 0x33, 0x3d, 0xb2, 0x53);
DEFINE_GUID(MFSampleExtension_EOS, 0x137e6b95, 0x4cad, 0x4cb9, 0x9b,0x7f,0x65,0xce,0xf2,0x06,0x8a,0x41);
DEFINE_GUID(MFSampleExtension_FrameServer_AllocatorType, 0x9bec3fd1, 0xd3fd, 0x4816, 0x89,0x43,0xdb,0xfe,0xb5,0xb4,0x33,0x72);
DEFINE_GUID(_FrameServerClient_SampleMediaTypeIndex, 0x3680913a, 0x0e20, 0x4b48, 0x93,0x64,0xbe,0x67,0xfb,0xf1,0xa5,0xe6);

DEFINE_GUID(MF_ALLOCATOR_TOTAL_BUFFER_COUNT, 0xc4139297, 0x2cec, 0x47c6, 0x9c,0xdf,0x6d,0xb6,0x2e,0xe6,0xdf,0x72);


static const char* to_string(const GUID & key) noexcept
{
    // MediaType attributes
    VV_VAR_TO_STRING(key, MF_MT_MAJOR_TYPE);
    VV_VAR_TO_STRING(key, MF_MT_MAJOR_TYPE);
    VV_VAR_TO_STRING(key, MF_MT_SUBTYPE);
    VV_VAR_TO_STRING(key, MF_MT_ALL_SAMPLES_INDEPENDENT);
    VV_VAR_TO_STRING(key, MF_MT_FIXED_SIZE_SAMPLES);
    VV_VAR_TO_STRING(key, MF_MT_COMPRESSED);
    VV_VAR_TO_STRING(key, MF_MT_SAMPLE_SIZE);
    VV_VAR_TO_STRING(key, MF_MT_WRAPPED_TYPE);
    VV_VAR_TO_STRING(key, MF_MT_AUDIO_NUM_CHANNELS);
    VV_VAR_TO_STRING(key, MF_MT_AUDIO_SAMPLES_PER_SECOND);
    VV_VAR_TO_STRING(key, MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND);
    VV_VAR_TO_STRING(key, MF_MT_AUDIO_AVG_BYTES_PER_SECOND);
    VV_VAR_TO_STRING(key, MF_MT_AUDIO_BLOCK_ALIGNMENT);
    VV_VAR_TO_STRING(key, MF_MT_AUDIO_BITS_PER_SAMPLE);
    VV_VAR_TO_STRING(key, MF_MT_AUDIO_VALID_BITS_PER_SAMPLE);
    VV_VAR_TO_STRING(key, MF_MT_AUDIO_SAMPLES_PER_BLOCK);
    VV_VAR_TO_STRING(key, MF_MT_AUDIO_CHANNEL_MASK);
    VV_VAR_TO_STRING(key, MF_MT_AUDIO_FOLDDOWN_MATRIX);
    VV_VAR_TO_STRING(key, MF_MT_AUDIO_WMADRC_PEAKREF);
    VV_VAR_TO_STRING(key, MF_MT_AUDIO_WMADRC_PEAKTARGET);
    VV_VAR_TO_STRING(key, MF_MT_AUDIO_WMADRC_AVGREF);
    VV_VAR_TO_STRING(key, MF_MT_AUDIO_WMADRC_AVGTARGET);
    VV_VAR_TO_STRING(key, MF_MT_AUDIO_PREFER_WAVEFORMATEX);
    VV_VAR_TO_STRING(key, MF_MT_AAC_PAYLOAD_TYPE);
    VV_VAR_TO_STRING(key, MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION);
    VV_VAR_TO_STRING(key, MF_MT_FRAME_SIZE);
    VV_VAR_TO_STRING(key, MF_MT_FRAME_RATE);
    VV_VAR_TO_STRING(key, MF_MT_FRAME_RATE_RANGE_MAX);
    VV_VAR_TO_STRING(key, MF_MT_FRAME_RATE_RANGE_MIN);
    VV_VAR_TO_STRING(key, MF_MT_PIXEL_ASPECT_RATIO);
    VV_VAR_TO_STRING(key, MF_MT_PAD_CONTROL_FLAGS);
    VV_VAR_TO_STRING(key, MF_MT_SOURCE_CONTENT_HINT);
    VV_VAR_TO_STRING(key, MF_MT_VIDEO_CHROMA_SITING);
    VV_VAR_TO_STRING(key, MF_MT_INTERLACE_MODE);
    VV_VAR_TO_STRING(key, MF_MT_TRANSFER_FUNCTION);
    VV_VAR_TO_STRING(key, MF_MT_VIDEO_PRIMARIES);
    VV_VAR_TO_STRING(key, MF_MT_CUSTOM_VIDEO_PRIMARIES);
    VV_VAR_TO_STRING(key, MF_MT_YUV_MATRIX);
    VV_VAR_TO_STRING(key, MF_MT_VIDEO_LIGHTING);
    VV_VAR_TO_STRING(key, MF_MT_VIDEO_NOMINAL_RANGE);
    VV_VAR_TO_STRING(key, MF_MT_GEOMETRIC_APERTURE);
    VV_VAR_TO_STRING(key, MF_MT_MINIMUM_DISPLAY_APERTURE);
    VV_VAR_TO_STRING(key, MF_MT_PAN_SCAN_APERTURE);
    VV_VAR_TO_STRING(key, MF_MT_PAN_SCAN_ENABLED);
    VV_VAR_TO_STRING(key, MF_MT_AVG_BITRATE);
    VV_VAR_TO_STRING(key, MF_MT_AVG_BIT_ERROR_RATE);
    VV_VAR_TO_STRING(key, MF_MT_MAX_KEYFRAME_SPACING);
    VV_VAR_TO_STRING(key, MF_MT_DEFAULT_STRIDE);
    VV_VAR_TO_STRING(key, MF_MT_PALETTE);
    VV_VAR_TO_STRING(key, MF_MT_USER_DATA);
    VV_VAR_TO_STRING(key, MF_MT_AM_FORMAT_TYPE);
    VV_VAR_TO_STRING(key, MF_MT_VIDEO_PROFILE);
    VV_VAR_TO_STRING(key, MF_MT_VIDEO_LEVEL);
    VV_VAR_TO_STRING(key, MF_MT_MPEG_START_TIME_CODE);
    VV_VAR_TO_STRING(key, MF_MT_MPEG_SEQUENCE_HEADER);
    VV_VAR_TO_STRING(key, MF_MT_MPEG2_FLAGS);
    VV_VAR_TO_STRING(key, MF_MT_DV_AAUX_SRC_PACK_0);
    VV_VAR_TO_STRING(key, MF_MT_DV_AAUX_CTRL_PACK_0);
    VV_VAR_TO_STRING(key, MF_MT_DV_AAUX_SRC_PACK_1);
    VV_VAR_TO_STRING(key, MF_MT_DV_AAUX_CTRL_PACK_1);
    VV_VAR_TO_STRING(key, MF_MT_DV_VAUX_SRC_PACK);
    VV_VAR_TO_STRING(key, MF_MT_DV_VAUX_CTRL_PACK);
    VV_VAR_TO_STRING(key, MF_MT_ARBITRARY_HEADER);
    VV_VAR_TO_STRING(key, MF_MT_ARBITRARY_FORMAT);
    VV_VAR_TO_STRING(key, MF_MT_IMAGE_LOSS_TOLERANT);
    VV_VAR_TO_STRING(key, MF_MT_ORIGINAL_4CC);
    VV_VAR_TO_STRING(key, MF_MT_ORIGINAL_WAVE_FORMAT_TAG);
    VV_VAR_TO_STRING(key, MF_MT_MPEG4_SAMPLE_DESCRIPTION);
    VV_VAR_TO_STRING(key, MF_MT_MPEG4_CURRENT_SAMPLE_ENTRY);
    VV_VAR_TO_STRING(key, MF_MT_VIDEO_ROTATION);
    VV_VAR_TO_STRING(key, MF_MT_VIDEO_3D);
    VV_VAR_TO_STRING(key, MF_MT_VIDEO_3D_FORMAT);
    VV_VAR_TO_STRING(key, MF_MT_VIDEO_3D_NUM_VIEWS);
    VV_VAR_TO_STRING(key, MF_MT_VIDEO_3D_LEFT_IS_BASE);
    VV_VAR_TO_STRING(key, MF_MT_VIDEO_3D_FIRST_IS_LEFT);
    VV_VAR_TO_STRING(key, MF_MT_TIMESTAMP_CAN_BE_DTS);
    VV_VAR_TO_STRING(key, MF_MT_PAD_CONTROL_FLAGS);
    VV_VAR_TO_STRING(key, MF_MT_SOURCE_CONTENT_HINT);
    VV_VAR_TO_STRING(key, MF_MT_CUSTOM_VIDEO_PRIMARIES);
    VV_VAR_TO_STRING(key, MF_MT_REALTIME_CONTENT);
    VV_VAR_TO_STRING(key, MF_MT_FSSourceTypeDecoded);
    // H264 / HEVC
    VV_VAR_TO_STRING(key, MF_NALU_LENGTH_SET);
    VV_VAR_TO_STRING(key, MF_NALU_LENGTH_INFORMATION);
    // MPEG4-sink
    VV_VAR_TO_STRING(key, MF_MPEG4SINK_MOOV_BEFORE_MDAT);
    VV_VAR_TO_STRING(key, MF_MPEG4SINK_SPSPPS_PASSTHROUGH);
    VV_VAR_TO_STRING(key, MF_MPEG4SINK_MINIMUM_PROPERTIES_SIZE);
    VV_VAR_TO_STRING(key, MF_MPEG4SINK_MIN_FRAGMENT_DURATION);
    VV_VAR_TO_STRING(key, MF_MPEG4SINK_MAX_CODED_SEQUENCES_PER_FRAGMENT);
    // Misc
    VV_VAR_TO_STRING(key, MF_USER_DATA_PAYLOAD);
    VV_VAR_TO_STRING(key, MF_PROGRESSIVE_CODING_CONTENT);
    VV_VAR_TO_STRING(key, MF_WVC1_PROG_SINGLE_SLICE_CONTENT);
    // mfreadwrite.h
    VV_VAR_TO_STRING(key, MF_READWRITE_DISABLE_CONVERTERS);
    VV_VAR_TO_STRING(key, MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS);
    VV_VAR_TO_STRING(key, MF_READWRITE_MMCSS_CLASS);
    VV_VAR_TO_STRING(key, MF_READWRITE_MMCSS_PRIORITY);
    VV_VAR_TO_STRING(key, MF_READWRITE_MMCSS_CLASS_AUDIO);
    VV_VAR_TO_STRING(key, MF_READWRITE_MMCSS_PRIORITY_AUDIO);
    VV_VAR_TO_STRING(key, MF_READWRITE_D3D_OPTIONAL);
    VV_VAR_TO_STRING(key, MF_READWRITE_ENABLE_AUTOFINALIZE);
    // Misc
    VV_VAR_TO_STRING(key, MF_MEDIASINK_AUTOFINALIZE_SUPPORTED);
    VV_VAR_TO_STRING(key, MF_MEDIASINK_ENABLE_AUTOFINALIZE);
    // IMFSourceReader
    VV_VAR_TO_STRING(key, MF_SOURCE_READER_ASYNC_CALLBACK);
    VV_VAR_TO_STRING(key, MF_SOURCE_READER_D3D_MANAGER);
    VV_VAR_TO_STRING(key, MF_SOURCE_READER_DISABLE_DXVA);
    VV_VAR_TO_STRING(key, MF_SOURCE_READER_MEDIASOURCE_CONFIG);
    VV_VAR_TO_STRING(key, MF_SOURCE_READER_MEDIASOURCE_CHARACTERISTICS);
    VV_VAR_TO_STRING(key, MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING);
    VV_VAR_TO_STRING(key, MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING);
    VV_VAR_TO_STRING(key, MF_SOURCE_READER_DISABLE_CAMERA_PLUGINS);
    VV_VAR_TO_STRING(key, MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN);
    VV_VAR_TO_STRING(key, MF_SOURCE_READER_ENABLE_TRANSCODE_ONLY_TRANSFORMS);
    VV_VAR_TO_STRING(key, MF_SOURCE_READER_D3D11_BIND_FLAGS);
    // IMFSinkWriter
    VV_VAR_TO_STRING(key, MF_SINK_WRITER_ASYNC_CALLBACK);
    VV_VAR_TO_STRING(key, MF_SINK_WRITER_DISABLE_THROTTLING);
    VV_VAR_TO_STRING(key, MF_SINK_WRITER_D3D_MANAGER);
    VV_VAR_TO_STRING(key, MF_SINK_WRITER_ENCODER_CONFIG);
    // Event
    VV_VAR_TO_STRING(key, MF_EVENT_DO_THINNING);
    VV_VAR_TO_STRING(key, MF_EVENT_SOURCE_CHARACTERISTICS);
    VV_VAR_TO_STRING(key, MF_EVENT_OUTPUT_NODE);
    VV_VAR_TO_STRING(key, MF_EVENT_PRESENTATION_TIME_OFFSET);
    VV_VAR_TO_STRING(key, MF_EVENT_SCRUBSAMPLE_TIME);
    VV_VAR_TO_STRING(key, MF_EVENT_SESSIONCAPS);
    VV_VAR_TO_STRING(key, MF_EVENT_SESSIONCAPS_DELTA);
    VV_VAR_TO_STRING(key, MF_EVENT_SOURCE_ACTUAL_START);
    VV_VAR_TO_STRING(key, MF_EVENT_SOURCE_CHARACTERISTICS);
    VV_VAR_TO_STRING(key, MF_EVENT_SOURCE_CHARACTERISTICS_OLD);
    VV_VAR_TO_STRING(key, MF_EVENT_SOURCE_FAKE_START);
    VV_VAR_TO_STRING(key, MF_EVENT_SOURCE_PROJECTSTART);
    VV_VAR_TO_STRING(key, MF_EVENT_SOURCE_TOPOLOGY_CANCELED);
    VV_VAR_TO_STRING(key, MF_EVENT_START_PRESENTATION_TIME);
    VV_VAR_TO_STRING(key, MF_EVENT_START_PRESENTATION_TIME_AT_OUTPUT);
    // Byte stream
    VV_VAR_TO_STRING(key, MF_BYTESTREAM_ORIGIN_NAME);
    VV_VAR_TO_STRING(key, MF_BYTESTREAM_CONTENT_TYPE);
    VV_VAR_TO_STRING(key, MF_BYTESTREAM_DURATION);
    VV_VAR_TO_STRING(key, MF_BYTESTREAM_LAST_MODIFIED_TIME);
    VV_VAR_TO_STRING(key, MF_BYTESTREAM_IFO_FILE_URI);
    VV_VAR_TO_STRING(key, MF_BYTESTREAM_DLNA_PROFILE_ID);
    VV_VAR_TO_STRING(key, MF_BYTESTREAM_EFFECTIVE_URL);
    VV_VAR_TO_STRING(key, MF_BYTESTREAM_TRANSCODED);
    // Major types
    VV_VAR_TO_STRING(key, MFMediaType_Audio);
    VV_VAR_TO_STRING(key, MFMediaType_Video);
    VV_VAR_TO_STRING(key, MFMediaType_Protected);
    VV_VAR_TO_STRING(key, MFMediaType_SAMI);
    VV_VAR_TO_STRING(key, MFMediaType_Script);
    VV_VAR_TO_STRING(key, MFMediaType_Image);
    VV_VAR_TO_STRING(key, MFMediaType_HTML);
    VV_VAR_TO_STRING(key, MFMediaType_Binary);
    VV_VAR_TO_STRING(key, MFMediaType_FileTransfer);
    // Video format
    VV_VAR_TO_STRING(key, MFVideoFormat_AI44);
    VV_VAR_TO_STRING(key, MFVideoFormat_ARGB32);
    VV_VAR_TO_STRING(key, MFVideoFormat_AYUV);
    VV_VAR_TO_STRING(key, MFVideoFormat_DV25);
    VV_VAR_TO_STRING(key, MFVideoFormat_DV50);
    VV_VAR_TO_STRING(key, MFVideoFormat_DVH1);
    VV_VAR_TO_STRING(key, MFVideoFormat_DVSD);
    VV_VAR_TO_STRING(key, MFVideoFormat_DVSL);
    VV_VAR_TO_STRING(key, MFVideoFormat_H264);
    VV_VAR_TO_STRING(key, MFVideoFormat_H264_ES);
    VV_VAR_TO_STRING(key, MFVideoFormat_HEVC);
    VV_VAR_TO_STRING(key, MFVideoFormat_HEVC_ES);
    VV_VAR_TO_STRING(key, MFVideoFormat_I420);
    VV_VAR_TO_STRING(key, MFVideoFormat_IYUV);
    VV_VAR_TO_STRING(key, MFVideoFormat_M4S2);
    VV_VAR_TO_STRING(key, MFVideoFormat_MJPG);
    VV_VAR_TO_STRING(key, MFVideoFormat_MP43);
    VV_VAR_TO_STRING(key, MFVideoFormat_MP4S);
    VV_VAR_TO_STRING(key, MFVideoFormat_MP4V);
    VV_VAR_TO_STRING(key, MFVideoFormat_MPG1);
    VV_VAR_TO_STRING(key, MFVideoFormat_MSS1);
    VV_VAR_TO_STRING(key, MFVideoFormat_MSS2);
    VV_VAR_TO_STRING(key, MFVideoFormat_NV11);
    VV_VAR_TO_STRING(key, MFVideoFormat_NV12);
    VV_VAR_TO_STRING(key, MFVideoFormat_P010);
    VV_VAR_TO_STRING(key, MFVideoFormat_P016);
    VV_VAR_TO_STRING(key, MFVideoFormat_P210);
    VV_VAR_TO_STRING(key, MFVideoFormat_P216);
    VV_VAR_TO_STRING(key, MFVideoFormat_RGB24);
    VV_VAR_TO_STRING(key, MFVideoFormat_RGB32);
    VV_VAR_TO_STRING(key, MFVideoFormat_RGB555);
    VV_VAR_TO_STRING(key, MFVideoFormat_RGB565);
    VV_VAR_TO_STRING(key, MFVideoFormat_RGB8);
    VV_VAR_TO_STRING(key, MFVideoFormat_UYVY);
    VV_VAR_TO_STRING(key, MFVideoFormat_v210);
    VV_VAR_TO_STRING(key, MFVideoFormat_v410);
    VV_VAR_TO_STRING(key, MFVideoFormat_WMV1);
    VV_VAR_TO_STRING(key, MFVideoFormat_WMV2);
    VV_VAR_TO_STRING(key, MFVideoFormat_WMV3);
    VV_VAR_TO_STRING(key, MFVideoFormat_WVC1);
    VV_VAR_TO_STRING(key, MFVideoFormat_Y210);
    VV_VAR_TO_STRING(key, MFVideoFormat_Y216);
    VV_VAR_TO_STRING(key, MFVideoFormat_Y410);
    VV_VAR_TO_STRING(key, MFVideoFormat_Y416);
    VV_VAR_TO_STRING(key, MFVideoFormat_Y41P);
    VV_VAR_TO_STRING(key, MFVideoFormat_Y41T);
    VV_VAR_TO_STRING(key, MFVideoFormat_YUY2);
    VV_VAR_TO_STRING(key, MFVideoFormat_YV12);
    VV_VAR_TO_STRING(key, MFVideoFormat_YVYU);
    VV_VAR_TO_STRING(key, MFVideoFormat_MPEG2);
    VV_VAR_TO_STRING(key, MFVideoFormat_L8);
    VV_VAR_TO_STRING(key, MFMPEG4Format_Base);
    // Audio format
    VV_VAR_TO_STRING(key, MFAudioFormat_PCM);
    VV_VAR_TO_STRING(key, MFAudioFormat_Float);
    VV_VAR_TO_STRING(key, MFAudioFormat_DTS);
    VV_VAR_TO_STRING(key, MFAudioFormat_Dolby_AC3);
    VV_VAR_TO_STRING(key, MFAudioFormat_Dolby_AC3_SPDIF);
    VV_VAR_TO_STRING(key, MFAudioFormat_DRM);
    VV_VAR_TO_STRING(key, MFAudioFormat_WMAudioV8);
    VV_VAR_TO_STRING(key, MFAudioFormat_WMAudioV9);
    VV_VAR_TO_STRING(key, MFAudioFormat_WMAudio_Lossless);
    VV_VAR_TO_STRING(key, MFAudioFormat_WMASPDIF);
    VV_VAR_TO_STRING(key, MFAudioFormat_MSP1);
    VV_VAR_TO_STRING(key, MFAudioFormat_MP3);
    VV_VAR_TO_STRING(key, MFAudioFormat_MPEG);
    VV_VAR_TO_STRING(key, MFAudioFormat_AAC);
    VV_VAR_TO_STRING(key, MFAudioFormat_ADTS);
    // Sample attributes
    VV_VAR_TO_STRING(key, MFSampleExtension_CleanPoint);
    VV_VAR_TO_STRING(key, MFSampleExtension_Interlaced);
    VV_VAR_TO_STRING(key, MFSampleExtension_Token);
    VV_VAR_TO_STRING(key, MFSampleExtension_Discontinuity);
    VV_VAR_TO_STRING(key, MFSampleExtension_SampleKeyID);
    VV_VAR_TO_STRING(key, MFSampleExtension_Timestamp);
    VV_VAR_TO_STRING(key, MFSampleExtension_FrameCorruption);
    VV_VAR_TO_STRING(key, MFSampleExtension_VideoEncodeQP);
    VV_VAR_TO_STRING(key, MFSampleExtension_VideoEncodePictureType);
    VV_VAR_TO_STRING(key, MFSampleExtension_3DVideo);
    VV_VAR_TO_STRING(key, MFSampleExtension_3DVideo_SampleFormat);
    VV_VAR_TO_STRING(key, MFSampleExtension_DecodeTimestamp);
    VV_VAR_TO_STRING(key, MFSampleExtension_DeviceTimestamp);
    VV_VAR_TO_STRING(key, MFSampleExtension_LongTermReferenceFrameInfo);
    VV_VAR_TO_STRING(key, MFSampleExtension_BottomFieldFirst);
    VV_VAR_TO_STRING(key, MFSampleExtension_DeviceReferenceSystemTime);
    VV_VAR_TO_STRING(key, MFSampleExtension_RepeatFirstField);
    VV_VAR_TO_STRING(key, MFSampleExtension_CaptureMetadata);
    VV_VAR_TO_STRING(key, MFSampleExtension_Depth_MinReliableDepth);
    VV_VAR_TO_STRING(key, MFSampleExtension_Depth_MaxReliableDepth);

    // Undocumented / custom GUIDs
    VV_VAR_TO_STRING(key, MFSampleExtension_FSMediaTypeInfo);
    VV_VAR_TO_STRING(key, MFSampleExtension_EOS);
    VV_VAR_TO_STRING(key, MFSampleExtension_FrameServer_AllocatorType);
    VV_VAR_TO_STRING(key, MF_ALLOCATOR_TOTAL_BUFFER_COUNT);

    VV_VAR_TO_STRING(key, MF_TRANSFORM_ASYNC);
    VV_VAR_TO_STRING(key, MF_TRANSFORM_ASYNC_UNLOCK);
    VV_VAR_TO_STRING(key, MF_TRANSFORM_CATEGORY_Attribute);
    VV_VAR_TO_STRING(key, MF_TRANSFORM_FLAGS_Attribute);

    VV_VAR_TO_STRING(key, MF_VIDEO_MAX_MB_PER_SEC);
    VV_VAR_TO_STRING(key, MF_ST_MEDIASOURCE_COLLECTION);
    VV_VAR_TO_STRING(key, MF_XVP_CALLER_ALLOCATES_OUTPUT);
    VV_VAR_TO_STRING(key, MF_XVP_DISABLE_FRC);
    VV_VAR_TO_STRING(key, MF_VIDEO_PROCESSOR_ALGORITHM);

    VV_VAR_TO_STRING(key, MF_SA_D3D_AWARE);
    VV_VAR_TO_STRING(key, MF_SA_D3D11_AWARE);
    VV_VAR_TO_STRING(key, MF_SA_D3D11_HW_PROTECTED);
    VV_VAR_TO_STRING(key, MF_SA_D3D11_ALLOW_DYNAMIC_YUV_TEXTURE);
    VV_VAR_TO_STRING(key, MF_SA_BUFFERS_PER_SAMPLE);
    VV_VAR_TO_STRING(key, MF_SA_D3D11_SHARED_WITHOUT_MUTEX);
    VV_VAR_TO_STRING(key, MF_SA_D3D11_SHARED);
    VV_VAR_TO_STRING(key, MF_SA_D3D11_USAGE);
    VV_VAR_TO_STRING(key, MF_SA_D3D11_BINDFLAGS);
    VV_VAR_TO_STRING(key, MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT);
    VV_VAR_TO_STRING(key, MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT_PROGRESSIVE);
    VV_VAR_TO_STRING(key, MF_SA_REQUIRED_SAMPLE_COUNT_PROGRESSIVE);

    VV_VAR_TO_STRING(key, MF_TOPONODE_TRANSFORM_OBJECTID);
    VV_VAR_TO_STRING(key, MF_TOPONODE_DECODER);
    VV_VAR_TO_STRING(key, MF_TOPONODE_MARKIN_HERE);
    VV_VAR_TO_STRING(key, MF_TOPONODE_MARKOUT_HERE);
    // IMFTransform
    VV_VAR_TO_STRING(key, MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE);
    VV_VAR_TO_STRING(key, MFT_TRANSFORM_CLSID_Attribute);
    VV_VAR_TO_STRING(key, MFT_INPUT_TYPES_Attributes);
    VV_VAR_TO_STRING(key, MFT_OUTPUT_TYPES_Attributes);
    VV_VAR_TO_STRING(key, MFT_ENUM_HARDWARE_URL_Attribute);
    VV_VAR_TO_STRING(key, MFT_FRIENDLY_NAME_Attribute);
    VV_VAR_TO_STRING(key, MFT_CONNECTED_STREAM_ATTRIBUTE);
    VV_VAR_TO_STRING(key, MFT_CONNECTED_TO_HW_STREAM);
    VV_VAR_TO_STRING(key, MFT_PREFERRED_OUTPUTTYPE_Attribute);
    VV_VAR_TO_STRING(key, MFT_PROCESS_LOCAL_Attribute);
    VV_VAR_TO_STRING(key, MFT_PREFERRED_ENCODER_PROFILE);
    VV_VAR_TO_STRING(key, MFT_HW_TIMESTAMP_WITH_QPC_Attribute);
    VV_VAR_TO_STRING(key, MFT_FIELDOFUSE_UNLOCK_Attribute);
    VV_VAR_TO_STRING(key, MFT_CODEC_MERIT_Attribute);
    VV_VAR_TO_STRING(key, MFT_ENUM_TRANSCODE_ONLY_ATTRIBUTE);
    VV_VAR_TO_STRING(key, MFT_ENUM_HARDWARE_VENDOR_ID_Attribute);
    VV_VAR_TO_STRING(key, MFT_ENCODER_SUPPORTS_CONFIG_EVENT);
    VV_VAR_TO_STRING(key, MFT_REMUX_MARK_I_PICTURE_AS_CLEAN_POINT);
    VV_VAR_TO_STRING(key, MFT_DECODER_FINAL_VIDEO_RESOLUTION_HINT);
    VV_VAR_TO_STRING(key, MFT_DECODER_EXPOSE_OUTPUT_TYPES_IN_NATIVE_ORDER);
    VV_VAR_TO_STRING(key, MFT_SUPPORT_3DVIDEO);
    VV_VAR_TO_STRING(key, MFT_GFX_DRIVER_VERSION_ID_Attribute);
    VV_VAR_TO_STRING(key, MFT_DECODER_QUALITY_MANAGEMENT_CUSTOM_CONTROL);
    VV_VAR_TO_STRING(key, MFT_DECODER_QUALITY_MANAGEMENT_RECOVERY_WITHOUT_ARTIFACTS);
    // Capture
    VV_VAR_TO_STRING(key, MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME);
    VV_VAR_TO_STRING(key, MF_DEVSOURCE_ATTRIBUTE_MEDIA_TYPE);
    VV_VAR_TO_STRING(key, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE);
    VV_VAR_TO_STRING(key, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_CATEGORY);
    VV_VAR_TO_STRING(key, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK);
    VV_VAR_TO_STRING(key, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_MAX_BUFFERS);
    VV_VAR_TO_STRING(key, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_HW_SOURCE);
    VV_VAR_TO_STRING(key, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    VV_VAR_TO_STRING(key, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_SYMBOLIC_LINK);
    VV_VAR_TO_STRING(key, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_ENDPOINT_ID);
    VV_VAR_TO_STRING(key, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_ROLE);
    VV_VAR_TO_STRING(key, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID);

    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_FRAMESERVER_SHARED);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_FRAMESERVER_HIDDEN);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_IMAGE_STREAM);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_INDEPENDENT_IMAGE_STREAM);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_STREAM_ID);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_STREAM_CATEGORY);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_TRANSFORM_STREAM_ID);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_SENSORSTREAM_ID);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_REQUIRED_CAPABILITIES);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_REQUIRED_SDDL);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_FILTER_KSCONTROL);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_PIN_KSCONTROL);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_SOURCE_ATTRIBUTES);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_MAX_FRAME_BUFFERS);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_EXTENSION_PLUGIN_CLSID);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_EXTENSION_PLUGIN_CONNECTION_POINT);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_TAKEPHOTO_TRIGGER);

    VV_VAR_TO_STRING(key, MF_DEVICEMFT_CONNECTED_FILTER_KSCONTROL);
    VV_VAR_TO_STRING(key, MF_DEVICEMFT_CONNECTED_PIN_KSCONTROL);
    VV_VAR_TO_STRING(key, MF_DEVICEMFT_SENSORPROFILE_COLLECTION);
    VV_VAR_TO_STRING(key, MF_DEVICEMFT_EXTENSION_PLUGIN_CLSID);
    VV_VAR_TO_STRING(key, MF_DEVICE_THERMAL_STATE_CHANGED);

    VV_VAR_TO_STRING(key, MF_SOURCE_STREAM_SUPPORTS_HW_CONNECTION);

    VV_VAR_TO_STRING(key, MF_TRANSCODE_CONTAINERTYPE);

    VV_VAR_TO_STRING(key, PINNAME_VIDEO_CAPTURE);
    VV_VAR_TO_STRING(key, PINNAME_VIDEO_PREVIEW);
    VV_VAR_TO_STRING(key, PINNAME_VIDEO_STILL);
    VV_VAR_TO_STRING(key, KSCATEGORY_VIDEO_CAMERA);
    VV_VAR_TO_STRING(key, KSCATEGORY_SENSOR_CAMERA);
    VV_VAR_TO_STRING(key, KSCATEGORY_NETWORK_CAMERA);

    // CODEC API
    VV_VAR_TO_STRING(key, MF_LOW_LATENCY);
    VV_VAR_TO_STRING(key, CODECAPI_AVDecVideoThumbnailGenerationMode);
    VV_VAR_TO_STRING(key, CODECAPI_AVDecSoftwareDynamicFormatChange);
    VV_VAR_TO_STRING(key, CODECAPI_AVDecNumWorkerThreads);
    VV_VAR_TO_STRING(key, CODECAPI_AVDecVideoMaxCodedWidth);
    VV_VAR_TO_STRING(key, CODECAPI_AVDecVideoMaxCodedHeight);
    VV_VAR_TO_STRING(key, CODECAPI_AVDecVideoAcceleration_H264);
    VV_VAR_TO_STRING(key, CODECAPI_AVDecDDDynamicRangeScaleLow);
    VV_VAR_TO_STRING(key, CODECAPI_AVDecAudioDualMono);
    VV_VAR_TO_STRING(key, CODECAPI_AVDecDDDynamicRangeScaleHigh);
    VV_VAR_TO_STRING(key, CODECAPI_AVDecCommonMeanBitRate);
    VV_VAR_TO_STRING(key, CODECAPI_AVDecAudioDualMonoReproMode);
    VV_VAR_TO_STRING(key, CODECAPI_AVDecDDOperationalMode);
    VV_VAR_TO_STRING(key, CODECAPI_AVDecVideoSWPowerLevel);

    VV_VAR_TO_STRING(key, KSMFT_CATEGORY_VIDEO_DECODER);
    VV_VAR_TO_STRING(key, KSMFT_CATEGORY_VIDEO_ENCODER);
    VV_VAR_TO_STRING(key, KSMFT_CATEGORY_VIDEO_PROCESSOR);
    VV_VAR_TO_STRING(key, KSMFT_CATEGORY_VIDEO_EFFECT);

    // Capture Metadata
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_FRAME_RAWSTREAM);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_FOCUSSTATE);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_REQUESTED_FRAME_SETTING_ID);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_EXPOSURE_TIME);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_EXPOSURE_COMPENSATION);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_ISO_SPEED);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_LENS_POSITION);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_SCENE_MODE);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_FLASH);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_FLASH_POWER);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_WHITEBALANCE);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_ZOOMFACTOR);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_FACEROIS);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_FACEROITIMESTAMPS);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_FACEROICHARACTERIZATIONS);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_ISO_GAINS);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_SENSORFRAMERATE);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_WHITEBALANCE_GAINS);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_HISTOGRAM);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_EXIF);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_FRAME_ILLUMINATION);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_UVC_PAYLOADHEADER);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_FIRST_SCANLINE_START_TIME_QPC);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_LAST_SCANLINE_END_TIME_QPC);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_SCANLINE_TIME_QPC_ACCURACY);
    VV_VAR_TO_STRING(key, MF_CAPTURE_METADATA_SCANLINE_DIRECTION);

    VV_VAR_TO_STRING(key, MF_CAPTURE_ENGINE_SELECTEDCAMERAPROFILE);
    VV_VAR_TO_STRING(key, MF_CAPTURE_ENGINE_SELECTEDCAMERAPROFILE_INDEX);
    VV_VAR_TO_STRING(key, MF_CAPTURE_ENGINE_D3D_MANAGER);

    // Not really undocumented, do not want do link with old DShow-libs...
    VV_VAR_TO_STRING(key, CLSID_VideoInputDeviceCategory);
    VV_VAR_TO_STRING(key, FORMAT_VideoInfo);

    VV_VAR_TO_STRING(key, MF_DEVPROXY_ATTRIBUTE_DEVSOURCE_MODE);

    VV_VAR_TO_STRING(key, MF_DEVSOURCE_ATTRIBUTE_USE_DSHOWBRIDGE);
    VV_VAR_TO_STRING(key, MF_DEVSOURCE_ATTRIBUTE_DEVICETYPE);
    VV_VAR_TO_STRING(key, MF_DEVSOURCE_ATTRIBUTE_FRAMESERVER_SHARE_MODE);
    VV_VAR_TO_STRING(key, MF_VIRTUALCAMERA_PROVIDE_ASSOCIATED_CAMERA_SOURCES);
    VV_VAR_TO_STRING(key, MF_VIRTUALCAMERA_ASSOCIATED_CAMERA_SOURCES);
    VV_VAR_TO_STRING(key, MF_DEVICESTREAM_ATTRIBUTE_DEFAULT_DEVICE_STREAM);
    VV_VAR_TO_STRING(key, MF_SA_D3D11_ALLOCATE_DISPLAYABLE_RESOURCES);
    VV_VAR_TO_STRING(key, MF_STREAM_DEVICECONTROL_SOURCETOKEN);

    VV_VAR_TO_STRING(key, MF_MT_D3D_DECODE_PROFILE_GUID);
    VV_VAR_TO_STRING(key, MF_TELEMETRY_SESSION_OBJECT_ATTRIBUTE);
    VV_VAR_TO_STRING(key, MF_FRAMESERVER_CLIENTCONTEXT_CLIENTPID);
    VV_VAR_TO_STRING(key, MEDIA_TELEMETRY_SESSION_ID);

    VV_VAR_TO_STRING(key, MF_BYTESTREAM_DOWNLOADRATE_SERVICE);
    VV_VAR_TO_STRING(key, MF_BYTESTREAM_OPTICAL_MEDIA);

    // Decoder profiles
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_MPEG2_MOCOMP);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_MPEG2_IDCT);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_MPEG2_VLD);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_MPEG1_VLD);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_MPEG2and1_VLD);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_H264_MOCOMP_NOFGT);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_H264_MOCOMP_FGT);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_H264_IDCT_NOFGT);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_H264_IDCT_FGT);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_H264_VLD_NOFGT);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_H264_VLD_FGT);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_H264_VLD_WITHFMOASO_NOFGT);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_H264_VLD_STEREO_PROGRESSIVE_NOFGT);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_H264_VLD_STEREO_NOFGT);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_H264_VLD_MULTIVIEW_NOFGT);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_WMV8_POSTPROC);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_WMV8_MOCOMP);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_WMV9_POSTPROC);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_WMV9_MOCOMP);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_WMV9_IDCT);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_VC1_POSTPROC);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_VC1_MOCOMP);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_VC1_IDCT);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_VC1_VLD);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_VC1_D2010);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_MPEG4PT2_VLD_SIMPLE);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_MPEG4PT2_VLD_ADVSIMPLE_NOGMC);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_MPEG4PT2_VLD_ADVSIMPLE_GMC);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_HEVC_VLD_MAIN);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_HEVC_VLD_MAIN10);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_VP9_VLD_PROFILE0);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_VP9_VLD_10BIT_PROFILE2);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_VP8_VLD);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_AV1_VLD_PROFILE0);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_AV1_VLD_PROFILE1);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_AV1_VLD_PROFILE2);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_AV1_VLD_12BIT_PROFILE2);
    VV_VAR_TO_STRING(key, D3D11_DECODER_PROFILE_AV1_VLD_12BIT_PROFILE2_420);

    VV_VAR_TO_STRING(key, MF_MT_FSSourceTypeDecoded);
    VV_VAR_TO_STRING(key, FSMediaType_CompressedPassthrough);

    return nullptr;
}

struct attribute_item
{
    GUID              key{};
    MF_ATTRIBUTE_TYPE type{};
    PROPVARIANT       value{};

    ~attribute_item()
    {
        ::PropVariantClear(&value);
    }
};

static std::string to_guid_string(const GUID& value) noexcept
{
    char buffer[40];
    ::sprintf_s(
        buffer,
        "{%08x-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx}",
        value.Data1, value.Data2, value.Data3, value.Data4[0], value.Data4[1],
        value.Data4[2], value.Data4[3], value.Data4[4], value.Data4[5], value.Data4[6], value.Data4[7]);
    return std::string{ buffer };
}

static std::string to_string(std::wstring_view str)
{
    const int32_t size = ::WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int32_t>(str.size()), nullptr, 0, nullptr, nullptr);
    if (size == 0)
    {
        return {};
    }
    std::string result(size, '?');
    WINRT_VERIFY_(size, ::WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int32_t>(str.size()), result.data(), size, nullptr, nullptr));
    return result;
}

using attribute_writer = string_builder<2000>;
static bool is_frame_rate(const GUID& key) noexcept
{
    return MF_MT_FRAME_RATE_RANGE_MAX == key ||
        MF_MT_FRAME_RATE_RANGE_MIN == key ||
        MF_MT_FRAME_RATE == key;
}

static bool is_packed_uint64(const GUID& key) noexcept
{
    return MF_MT_FRAME_SIZE == key ||
        MF_MT_FRAME_RATE_RANGE_MAX == key ||
        MF_MT_FRAME_RATE_RANGE_MIN == key ||
        MF_MT_FRAME_RATE == key ||
        MF_MT_PIXEL_ASPECT_RATIO == key;
}

static void write_uint32(attribute_writer& writer, const attribute_item& item, std::string_view keyname) noexcept
{
    writer.append_format("[uint32_t]\t%s = %u\n", keyname.data(), item.value.ulVal);
}

static void write_uint64(attribute_writer& writer, const attribute_item& item, std::string_view keyname) noexcept
{
    const uint64_t raw_value = item.value.uhVal.QuadPart;
    if (is_packed_uint64(item.key))
    {
        uint32_t hi{};
        uint32_t lo{};
        ::Unpack2UINT32AsUINT64(raw_value, &hi, &lo);
        if (is_frame_rate(item.key))
        {
            writer.append_format("[uint64_t]\t%s = [%ux%u]\n", keyname.data(), hi, lo);
        }
        else
        {
            writer.append_format("[uint64_t]\t%s = [%ux%u]\n", keyname.data(), hi, lo);
        }
    }
    else
    {
        writer.append_format("[uint64_t]\t%s = %llu\n", keyname.data(), raw_value);
    }
}

static void write_double(attribute_writer& writer, const attribute_item& item, std::string_view keyname) noexcept
{
    writer.append_format("[double]\t%s = %f\n", keyname.data(), item.value.dblVal);
}

static void write_guid(attribute_writer& writer, const attribute_item& item, std::string_view keyname) noexcept
{
    const auto g = *item.value.puuid;
    auto value = to_guid_string(g);
    const char* valuename = to_string(g);
    if (!valuename)
    {
        valuename = value.data();
    }
    writer.append_format("[GUID]\t\t%s = %s\n", keyname.data(), valuename);
}

static void write_string(attribute_writer& writer, const attribute_item& item, std::string_view keyname) noexcept
{
    writer.append_format("[string]\t%s = %s\n", keyname.data(), to_string(item.value.pwszVal).data());
}

static constexpr float to_float(const MFOffset offset)
{
    return static_cast<float>(offset.value) + (static_cast<float>(offset.fract) / 65536.0f);
}

static void write_blob(attribute_writer& writer, const attribute_item& item, std::string_view keyname) noexcept
{
    if (MF_MT_MINIMUM_DISPLAY_APERTURE == item.key ||
        MF_MT_GEOMETRIC_APERTURE == item.key ||
        MF_MT_PAN_SCAN_APERTURE == item.key)
    {
        MFVideoArea area{};
        ::memcpy_s(&area, sizeof(MFVideoArea), item.value.caub.pElems, item.value.caub.cElems);
        const auto ox = to_float(area.OffsetX);
        const auto oy = to_float(area.OffsetY);
        writer.append_format("[blob]\t\t%s = [%.1f,%.1f] [%dx%d] (MFVideoArea)\n",
            keyname.data(),
            ox,
            oy,
            area.Area.cx,
            area.Area.cy);
    }
    else
    {
        writer.append_format("[blob]\t\t%s = [%u bytes]\n", keyname.data(), item.value.caub.cElems);
    }
}

static void write_iunknown(attribute_writer& writer, const attribute_item& item, std::string_view keyname) noexcept
{
    writer.append_format("[IUnknown]\t%s = 0x%X\n", keyname.data(), item.value.punkVal);
}

static void write_item(attribute_writer& writer, const attribute_item& item) noexcept
{
    std::string raw;
    const char* keyname = to_string(item.key);
    if (!keyname)
    {
        raw = to_guid_string(item.key);
        keyname = raw.data();
    }
    switch (item.type)
    {
    case MF_ATTRIBUTE_UINT32:     write_uint32(writer, item, keyname); return;
    case MF_ATTRIBUTE_UINT64:     write_uint64(writer, item, keyname); return;
    case MF_ATTRIBUTE_DOUBLE:     write_double(writer, item, keyname); return;
    case MF_ATTRIBUTE_GUID:         write_guid(writer, item, keyname); return;
    case MF_ATTRIBUTE_STRING:     write_string(writer, item, keyname); return;
    case MF_ATTRIBUTE_BLOB:         write_blob(writer, item, keyname); return;
    case MF_ATTRIBUTE_IUNKNOWN: write_iunknown(writer, item, keyname); return;
    default:                                                           return;
    }
}


static std::string to_string(IMFAttributes* attr)
{
    if (!attr || FAILED(attr->LockStore()))
    {
        return {};
    }
    uint32_t count{};
    attr->GetCount(&count);
    attribute_writer writer;
    writer.append_format("[uint32_t]\tCount = %u\n", count);
    for (uint32_t i = 0; i < count; ++i)
    {
        attribute_item item;
        WINRT_VERIFY_(S_OK, attr->GetItemByIndex(i, &item.key, &item.value));
        WINRT_VERIFY_(S_OK, attr->GetItemType(item.key, &item.type));
        write_item(writer, item);
    }
    WINRT_VERIFY_(S_OK, attr->UnlockStore());
    return std::string(writer.data());
}

static void PrintLine(IMFAttributes* attr)
{
    PrintLine(to_string(attr).data());
}

