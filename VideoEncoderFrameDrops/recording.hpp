#pragma once
#include "winrt/base.h"
#include <mfidl.h>
#include "video_frame_analyzer.hpp"

struct RecordingFile : winrt::implements<RecordingFile, IMFAsyncCallback>
{
    static constexpr DWORD InvalidStream = 0xffffffff;

    static HRESULT Create(
        std::wstring    filePath,
        IMFMediaType*   videoType,
        IMFMediaType*   audioType,
        RecordingFile** file);

    RecordingFile(IMFSinkWriter* writer, DWORD workQueue, DWORD audioStream, DWORD videoStream);
    ~RecordingFile();
    HRESULT __stdcall GetParameters(DWORD* flags, DWORD* queue) noexcept final;
    HRESULT __stdcall Invoke(IMFAsyncResult* result) noexcept final;
    HRESULT Prepare(IMFMediaType* videoType);
    HRESULT Finalize() const;
    winrt::DateTime AcquisitionTime() const noexcept { return m_acquisitionTime; }
    void GetStatistics(MF_SINK_WRITER_STATISTICS* videoStats, MF_SINK_WRITER_STATISTICS* audioStats) const;

private:
    void WriteVideo(IMFSample* video);
    void WriteAudio(IMFSample* audio);
    winrt::TimeSpan WriteInternal(IMFSample* sample, DWORD stream) const;
    void SendStreamTick(DWORD stream, winrt::TimeSpan timestamp) const;
    bool WaitForFirstKeyFrame(IMFSample* video);

    bool HasAudioVideoSyncPoint() const noexcept { return m_audioVideoSyncPoint >= winrt::TimeSpan::zero(); }
    bool ReceivedFirstKeyFrame() const noexcept { return m_videoTime >= winrt::TimeSpan::zero(); }
    bool FirstAudioSample() const noexcept { return m_audioTime < winrt::TimeSpan::zero(); }

    void GetStatistics(DWORD stream, MF_SINK_WRITER_STATISTICS* stats) const;

private:
    DWORD                         m_videoStream{ InvalidStream };
    DWORD                         m_audioStream{ InvalidStream };
    DWORD                         m_workQueue{ 0x1 };
    winrt::TimeSpan               m_audioTime{ VideoFrameAnalyzer::InvalidTime };
    winrt::TimeSpan               m_audioVideoSyncPoint{ VideoFrameAnalyzer::InvalidTime };
    winrt::com_ptr<IMFSinkWriter> m_writer;
    winrt::TimeSpan               m_videoTime{ VideoFrameAnalyzer::InvalidTime };
    VideoFrameAnalyzer            m_videoAnalyzer;
    winrt::DateTime               m_acquisitionTime{};
    MF_SINK_WRITER_STATISTICS     m_videoStats{};
    MF_SINK_WRITER_STATISTICS     m_audioStats{};
};
