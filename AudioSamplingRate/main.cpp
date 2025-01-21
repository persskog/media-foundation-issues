#include "pch.h"
#include "capture_engine.h"

static auto InitializeApp(DWORD flags = MFSTARTUP_LITE) noexcept
{
    wil::SetResultLoggingCallback([](wil::FailureInfo const& failure) noexcept
        {
            std::array<wchar_t, 2048> msg{};
            if (SUCCEEDED(wil::GetFailureLogString(msg.data(), msg.max_size(), failure)))
            {
                std::fputws(msg.data(), stderr);
            }
        });

    winrt::init_apartment();
    WINRT_VERIFY_(S_OK, ::MFStartup(MF_VERSION, flags));
    return wil::scope_exit([]
        {
            WINRT_VERIFY_(S_OK, ::MFShutdown());
        });
}

static void WaitForQKeyPress()
{
    std::cout << "Press 'Q' to exit..." << std::endl;
    while (true)
    {
        char ch;
        std::cin >> ch;
        if (ch == 'Q' || ch == 'q')
        {
            break;
        }
    }
}

int main()
{
    try
    {
        auto app{ InitializeApp() };
        auto device = CaptureEngine::ShowAvailableAudioDevices();

        CaptureParams params;
        params.DisableThrottling = true;
        params.Fragmented = true;
        CaptureEngine::Run(device, params);
    }
    catch (...)
    {
        LOG_CAUGHT_EXCEPTION();
    }
    WaitForQKeyPress();
    return 0;
}
