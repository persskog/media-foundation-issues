#include "pch.h"
#include "file_reader.hpp"

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
    constexpr auto FRAGMENTED_MP4 = L"D:\\media-foundation-issues\\fmpeg_audio_video_sync.mp4";
    constexpr auto REGULAR_MP4 = L"C:\\Users\\carl\\Videos\\glitch_0928_audio_video.mp4";
    constexpr auto REINDEX_MP4 = L"D:\\media-foundation-issues\\reindex.mp4";

    try
    {
        auto app{ InitializeApp() };
        FileReader::Reindex(FRAGMENTED_MP4, REINDEX_MP4);
        //FileReader::ReadFile(FRAGMENTED_MP4);
    }
    catch (...)
    {
        LOG_CAUGHT_EXCEPTION();
    }
    WaitForQKeyPress();
    return 0;
}
