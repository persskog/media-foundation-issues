#include "pch.h"
#include "video_frame_analyzer.hpp"

#ifdef VV_ENABLE_VIDEO_FRAME_ANALYZER_LOG

#define VFA_LOG(fmt, ...) \
    VideoFrameAnalyzer::PrintLine(fmt, __VA_ARGS__)

#else

#define VFA_LOG(fmt, ...)

#endif

winrt::TimeSpan VideoFrameAnalyzer::Analyze(IMFSample* frame)
{
    using namespace std::chrono;
    WINRT_ASSERT(frame);

    ++m_frameCounter;
    const auto elapsedSinceFirstFrame = ElapsedTimeSinceFirstFrame();

    if (IsInvalidTime(m_prevDts))
    {
        OnFirstFrame(frame);
        return {};
    }

    uint32_t discontinuity{};
    uint32_t lostFramesSinceLastValid{};
    frame->GetUINT32(MFSampleExtension_Discontinuity, &discontinuity);
    const auto duration = GetSampleDuration(frame);
    const auto dts = GetDts(frame);
    // Should be around average time per frame for the current frame rate
    const auto deltaDts = dts - m_prevDts;
    m_accumDuration += duration;
    m_prevDts = dts;

    //if (discontinuity)
    //{
    //    // We can have some dropped frames by the encoder
    //    lostFramesSinceLastValid = CalculateDroppenFrames(deltaDts, duration);
    //    m_accumDroppenFrames += lostFramesSinceLastValid;

    //    VFA_LOG("[Discontinuity] %lld (%lldms) ~%u lost since last valid (total lost %u)",
    //        deltaDts.count(),
    //        duration_cast<milliseconds>(deltaDts).count(),
    //        lostFramesSinceLastValid,
    //        m_accumDroppenFrames);
    //}
    //else
    //{
    //    auto delay = CheckForIncreasedFrameDelay(deltaDts);
    //    // TODO: What should be done here?
    //}

    ValidateFrameCounter(elapsedSinceFirstFrame,duration);

    // The actual time missing in the stream
    return lostFramesSinceLastValid * duration;
}

void VideoFrameAnalyzer::Initialize(IMFMediaType* type)
{
    uint32_t num = {}, den = {};
    LOG_IF_FAILED(::MFGetAttributeRatio(type, MF_MT_FRAME_RATE, &num, &den));
    uint64_t avgTimePerFrame{};
    LOG_IF_FAILED(::MFFrameRateToAverageTimePerFrame(num, den, &avgTimePerFrame));
    m_avgTimePerFrame = winrt::TimeSpan{ avgTimePerFrame };
    const auto fps = num / static_cast<double>(den);
    VFA_LOG("Video frame rate: %.2ffps (%lld)", fps, m_avgTimePerFrame);
}

void VideoFrameAnalyzer::Reset()
{
    m_prevDts = InvalidTime;
    m_clockStart = InvalidTime;
    m_delayUntilFirstFrame = InvalidTime;
    m_frameCounter = {};
    m_approxDroppedFrames = {};
}

void VideoFrameAnalyzer::OnFirstFrame(IMFSample* frame)
{
    using namespace std::chrono;
    const auto now = VideoFrameAnalyzer::Now();
    const auto dts = GetDts(frame);
    const auto duration = GetSampleDuration(frame);
    const auto delay = now - dts;

#ifdef _DEBUG
    uint32_t keyframe{};
    frame->GetUINT32(MFSampleExtension_CleanPoint, &keyframe);
    WINRT_ASSERT(keyframe);
#endif // _DEBUG

    m_clockStart = now - delay;
    m_prevDts = dts;
    m_delayUntilFirstFrame = delay;
    m_accumDroppenFrames = 0;
    m_accumDuration += duration;
    VFA_LOG("[First frame] %lld, %lldms delay since captured",
        dts.count(), 
        AsMilliseconds(delay));
}

winrt::TimeSpan VideoFrameAnalyzer::ElapsedTimeSinceFirstFrame() const
{
    return Now() - m_clockStart;
}

void VideoFrameAnalyzer::ValidateFrameCounter(winrt::TimeSpan elapsedSinceFirstFrame, winrt::TimeSpan frameDuration) const
{
    using namespace std::chrono;

    const auto diff_duration = elapsedSinceFirstFrame - m_accumDuration;

    // Based upon the clock this should represent an approximation of
    // number of frames received duration current period
    const auto num_frames_clock_based = elapsedSinceFirstFrame.count() / static_cast<double>(frameDuration.count());
    // This is based on accumelated duration, it should match the frame counter
    const auto num_frames_duration_based = m_accumDuration.count() / static_cast<double>(frameDuration.count());

    const auto frames_diff = num_frames_duration_based - num_frames_clock_based;

    const uint32_t counter = m_frameCounter + m_accumDroppenFrames;
    const auto actual_frames = abs(num_frames_clock_based - num_frames_duration_based);

    if (abs(frames_diff) > 1.0)
    {
        VFA_LOG("frame %u: [%.2lf, %.2lf] %.2lf",
            counter,
            num_frames_clock_based,
            num_frames_duration_based,
            frames_diff);
    }

    
  /*  if (actual_frames > 1.0)
    {
        VFA_LOG(
            "%u <-> %.2lf, %.2lf | clock vs. accum: %lldms (%lld) | fc=%u vs. fl=%u",
            counter,
            num_frames_clock_based,
            num_frames_duration_based,
            AsMilliseconds(diff_duration),
            frameDuration,
            m_frameCounter,
            m_accumDroppenFrames);
    }*/
}

winrt::TimeSpan VideoFrameAnalyzer::CheckForIncreasedFrameDelay(winrt::TimeSpan delta) const
{
    using namespace std::chrono_literals;

    // The frame took longer than the excepted time to arrive.
    // Webcameras can have increased it's shutter time and therefore
    // the capture time increases
    // Not really lost
    if (delta > m_avgTimePerFrame)
    {
        const auto amount = delta - m_avgTimePerFrame;
        if (amount > 10ms)
        {
            VFA_LOG(
                "[%u] High delay between frames %lldms", 
                m_frameCounter,
                AsMilliseconds(amount));
        }
        return amount;
    }
    return {};
}

uint32_t VideoFrameAnalyzer::CalculateDroppenFrames(winrt::TimeSpan delta, winrt::TimeSpan frameDuration)
{
    const auto lost = delta.count() / static_cast<double>(frameDuration.count());
    const auto count = std::round(lost);
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