#pragma once
#include "winrt/Windows.Foundation.h"

struct IMFSample;

struct VideoFrameAnalyzer
{
    int32_t Analyze(IMFSample* frame);
    void Reset();
    void OnFirstFrame(IMFSample* frame);
    winrt::TimeSpan ElapsedTimeSinceFirstFrame() const;
    bool IsDtsDeltaOk(winrt::TimeSpan delta, winrt::TimeSpan frameDuration);
    void ValidateFrameCounter(winrt::TimeSpan frameDuration) const;
    void Initialize(IMFMediaType* type);
    winrt::TimeSpan CheckForIncreasedFrameDelay(winrt::TimeSpan delta);

    static uint32_t CalculateLostFrames(winrt::TimeSpan delta, winrt::TimeSpan frameDuration);
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
    uint32_t        m_approxLostFrames{};
    uint32_t        m_accumLostFrames{};
    winrt::TimeSpan m_accumDuration{};
    winrt::TimeSpan m_delayUntilFirstFrame{ InvalidTime };
};
