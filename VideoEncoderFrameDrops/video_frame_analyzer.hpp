/********************************************************************
*
* Description:
*
* Author: Carl Persskog
*
********************************************************************/
#pragma once
#define VV_ENABLE_VIDEO_FRAME_ANALYZER_LOG
#include "winrt/Windows.Foundation.h"

// Forward declare
struct IMFSample;

struct VideoFrameAnalyzer
{
    winrt::TimeSpan Analyze(IMFSample* frame);
    void Initialize(IMFMediaType* type);
    void Reset();

#ifdef VV_ENABLE_VIDEO_FRAME_ANALYZER_LOG
    template <typename ... Args>
    static void PrintLine(char const* const format, Args ... args) noexcept
    {
#ifdef _DEBUG
        char buffer[1024] = {};
        ::sprintf_s(buffer, std::size(buffer), format, args ...);
        ::OutputDebugStringA(buffer);
        ::OutputDebugStringA("\n");
#else
        ::printf_s(format, args ...);
        ::printf_s("\n");
#endif
    }
#endif

    static winrt::TimeSpan GetSampleTime(IMFSample* sample);
    static winrt::TimeSpan GetSampleDuration(IMFSample* sample);
    static winrt::TimeSpan GetDts(IMFSample* sample);
    winrt::TimeSpan GetPrevDts() const noexcept { return m_prevDts; }
    static int64_t AsMilliseconds(winrt::TimeSpan ts);
    static winrt::TimeSpan Now();
    static constexpr winrt::TimeSpan InvalidTime{ -1 };

private:
    void OnFirstFrame(IMFSample* frame);
    winrt::TimeSpan ElapsedTimeSinceFirstFrame() const;
    void ValidateFrameCounter(winrt::TimeSpan elapsedSinceFirstFrame, winrt::TimeSpan frameDuration) const;
    winrt::TimeSpan CheckForIncreasedFrameDelay(winrt::TimeSpan delta) const;
    static uint32_t CalculateDroppenFrames(winrt::TimeSpan delta, winrt::TimeSpan frameDuration);
    static bool IsInvalidTime(winrt::TimeSpan ts);

private:
    winrt::TimeSpan m_prevDts{ InvalidTime };
    winrt::TimeSpan m_clockStart{ InvalidTime };
    winrt::TimeSpan m_avgTimePerFrame{};
    winrt::TimeSpan m_accumDuration{};
    winrt::TimeSpan m_delayUntilFirstFrame{ InvalidTime };
    uint32_t        m_frameCounter{};
#ifdef VV_ENABLE_VIDEO_FRAME_ANALYZER_LOG
    uint32_t        m_accumDroppenFrames{};
#endif
};
