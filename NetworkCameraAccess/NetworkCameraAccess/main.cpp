#include "framework.hpp"

int main()
{
    try
    {
        auto ctx{ InitializeApp() };
        if (auto device = SelectVideoDeviceActivate())
        {
            auto reader = CreateSourceReader(device);

            HRESULT hr = S_OK;
            uint32_t frames = 10;
            while (SUCCEEDED(hr) && frames > 0)
            {
                winrt::com_ptr<IMFSample> sample;
                DWORD flags{};
                hr = reader->ReadSample(
                    MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                    0,
                    nullptr,
                    &flags,
                    nullptr,
                    sample.put());

                if (sample)
                {
                    int64_t timestamp{};
                    int64_t duration{};
                    sample->GetSampleDuration(&duration);
                    sample->GetSampleTime(&timestamp);
                    Print("Sample: %lld (%lld)", timestamp, duration);
                    --frames;
                }
            }
        }
    }
    CATCH_LOG();
    return 0;
}

