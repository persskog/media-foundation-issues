#include "pch.h"
#include "setup.h"

static winrt::IAsyncAction RunAsync(winrt::DispatcherQueue queue)
{
    State state;
    state.Manager = CreateMFDXGIDeviceManager();
    state.Engine = CreateCaptureEngine();
    state.VideoDevice = CreateVideoDevice(state);
    if (state.VideoDevice)
    {
        Initialize(state);
    }
    else
    {
        PRINTLN("Failed to initialize video device, exiting...");
        co_return;
    }

    // --- Helper Text for the User ---
    PRINTLN("=======================================");
    PRINTLN(" Media Capture Application Initialized ");
    PRINTLN("=======================================");
    PRINTLN(" Controls:");
    PRINTLN("   Press [A] to Toggle Preview");
    PRINTLN("   Press [S] to Toggle Encode");
    PRINTLN("   Press [D] to Show Available Preview Outputs (only shows NV12/YUY2 for clearity)");
    PRINTLN("   Press [F] to Show Available Encode Outputs (only shows NV12/YUY2 for clearity)");
    PRINTLN("   Press [Q] to Quit");
    PRINTLN("=======================================\n");

    while (true)
    {
        // 1. Check if a key has been pressed without blocking the loop
        if (_kbhit())
        {
            // 2. Read the key character (convert to lowercase for consistency)
            char ch = tolower(_getch());

            // 3. Handle the specific keypresses
            if (ch == 'a')
            {
                state.TogglePreview();
            }
            else if (ch == 's')
            {
                state.ToggleEncode();
            }
            else if (ch == 'q')
            {
                break; // Exit the loop
            }
            else if (ch == 'd')
            {
                state.ShowAvailablePreviewOutputs();
            }
            else if (ch == 'f')
            {
                state.ShowAvailableEncodeOutputs();
            }
        }

        // 4. Prevent the loop from maxing out your CPU core
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    co_return;
}

int main()
{
    try
    {
        winrt::init_apartment();
        auto dqc = winrt::CreateDispatcherQueueController();
        winrt::check_hresult(::MFStartup(MF_VERSION, MFSTARTUP_LITE));
        RunAsync(dqc.DispatcherQueue()).get();
    }
    catch (...)
    {
        LOG_ERROR();
    }

    WINRT_VERIFY_(S_OK, ::MFShutdown());
    return 0;
}

