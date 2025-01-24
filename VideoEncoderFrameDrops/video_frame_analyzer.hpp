#pragma once
#include "winrt/Windows.Foundation.h"

struct IMFSample;

#define VV_VIDEO_FRAME_ANALYZER_PRINT

struct VideoFrameAnalyzer
{
    winrt::TimeSpan Analyze(IMFSample* frame);
    void Initialize(IMFMediaType* type);
    void Reset();

#ifdef VV_VIDEO_FRAME_ANALYZER_PRINT
    template <typename ... Args>
    static void PrintLine(char const* const format, Args ... args) noexcept
    {
        char buffer[1024] = {};
        ::sprintf_s(buffer, std::size(buffer), format, args ...);
        ::OutputDebugStringA(buffer);
        ::OutputDebugStringA("\n");
    }
#endif

private:
    void OnFirstFrame(IMFSample* frame);
    winrt::TimeSpan ElapsedTimeSinceFirstFrame() const;
    void ValidateFrameCounter(winrt::TimeSpan frameDuration) const;
    
    winrt::TimeSpan CheckForIncreasedFrameDelay(winrt::TimeSpan delta) const;

    static uint32_t CalculateDroppenFrames(winrt::TimeSpan delta, winrt::TimeSpan frameDuration);
    static winrt::TimeSpan GetSampleTime(IMFSample* sample);
    static winrt::TimeSpan GetSampleDuration(IMFSample* sample);
    static winrt::TimeSpan GetDts(IMFSample* sample);
    static winrt::TimeSpan Now();
    static int64_t AsMilliseconds(winrt::TimeSpan ts);
    static bool IsInvalidTime(winrt::TimeSpan ts);



private:
    static constexpr long long HundredNanoSecondsInSecond = 10000000LL;
    static constexpr winrt::TimeSpan DeltaLimit{ HundredNanoSecondsInSecond * 2 };
    static constexpr winrt::TimeSpan InvalidTime{ -1 };

    winrt::TimeSpan m_prevDts{ InvalidTime };
    winrt::TimeSpan m_clockStart{ InvalidTime };
    winrt::TimeSpan m_avgTimePerFrame{};
    uint32_t        m_frameCounter{};
    uint32_t        m_approxDroppedFrames{};
    uint32_t        m_accumDroppenFrames{};
    winrt::TimeSpan m_accumDuration{};
    winrt::TimeSpan m_delayUntilFirstFrame{ InvalidTime };
};
