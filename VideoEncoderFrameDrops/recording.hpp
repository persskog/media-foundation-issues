#pragma once
#include "winrt/base.h"
#include <mfidl.h>
#include "video_frame_analyzer.hpp"

struct Recording : winrt::implements<Recording, IMFAsyncCallback>
{
    HRESULT __stdcall GetParameters(DWORD* flags, DWORD* queue) noexcept final;
    HRESULT __stdcall Invoke(IMFAsyncResult* result) noexcept final;

    HRESULT Initialize(IMFMediaType* videoType, IMFMediaType* audioType);
    HRESULT Done();

private:
    void Write(IMFSample* frame);
    void WriteAudio(IMFSample* audio);
    void WriteVideo(IMFSample* video);

    void WriteInternal(IMFSample* sample, DWORD stream);

private:
    VideoFrameAnalyzer            m_analyzer;
    DWORD                         m_videoStream{};
    DWORD                         m_audioStream{ 0xffffffff };
    std::atomic_bool              m_done;
    winrt::TimeSpan               m_videoTime{ -1 };
    winrt::TimeSpan               m_audioTime{ -1 };
    winrt::TimeSpan               m_audioVideoSyncPoint{ -1 };
    winrt::com_ptr<IMFSinkWriter> m_writer;
};
