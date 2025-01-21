#pragma once
#include "winrt/base.h"

struct IMFActivate;

struct CaptureParams
{
    bool DisableThrottling{ false };
    bool Fragmented{ false };
    uint32_t AacSamplePerSecond{ 48000u };
    uint32_t AacAvgBytesPerSecond{ 24000u };
};

struct CaptureEngine
{
    static winrt::com_ptr<IMFActivate> ShowAvailableAudioDevices();
    static void Run(const winrt::com_ptr<IMFActivate>& device, const CaptureParams& params);
};

