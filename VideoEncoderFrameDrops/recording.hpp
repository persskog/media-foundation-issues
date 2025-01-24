#pragma once
#include "winrt/base.h"
#include <mfidl.h>
#include "video_frame_analyzer.hpp"

struct Recording : winrt::implements<Recording, IMFAsyncCallback>
{
    HRESULT __stdcall GetParameters(DWORD* flags, DWORD* queue) noexcept final;
    HRESULT __stdcall Invoke(IMFAsyncResult* result) noexcept final;

    HRESULT Initialize(IMFMediaType* type);
    HRESULT Done();

private:
    void Write(IMFSample* frame);

private:
    VideoFrameAnalyzer            m_analyzer;
    winrt::TimeSpan               m_videoTime{};
    winrt::com_ptr<IMFSinkWriter> m_writer;
};
