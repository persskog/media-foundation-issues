#include "pch.h"
#include "video_frame_analyzer.hpp"

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