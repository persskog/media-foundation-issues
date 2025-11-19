#include "pch.h"
#include "capture_manager.hpp"
#include <cstdio>
#include <cctype>

static auto Run(const winrt::com_ptr<CaptureManager>& manager)
{
    char input_char{};
    do
    {
        ::printf_s("Enter command (A=start, S=stop, Q=quit): ");
        if (::scanf_s(" %c", &input_char, 1u) != 1)
        {
            continue;
        }

        // Convert the input character to uppercase to ensure case-insensitive matching
        switch (::toupper(input_char))
        {
        case 'A':
            PrintLine("Starting encoding...");
            LOG_IF_FAILED(manager->Engine->StartRecord());
            break;

        case 'S':
            PrintLine("Stopping encoding...");
            LOG_IF_FAILED(manager->Engine->StopRecord(TRUE, TRUE));
            break;

        case 'Q':
            PrintLine("Exiting...");
            return;

        default:
            PrintLine("Invalid command. Please enter A, S, or Q.");
            break;
        }

    } while (true);
}

int main()
{
    try
    {
        auto on_exit = Startup();
        auto controller = CreateDispatcherQueueControllerOnCurrentThread();
        winrt::com_ptr<IMFAttributes> attr;
        winrt::check_hresult(::MFCreateAttributes(attr.put(), 5));
        auto vid_device = SelectVideoDeviceActivate(attr.get());
        auto d3d_manager = CreateMFDXGIDeviceManager();

        auto manager = winrt::make_self<CaptureManager>();
        manager->Initialize(attr.get(), d3d_manager.get(), vid_device.get());
        Run(manager);
    }
    catch (...)
    {
        LOG_CAUGHT_EXCEPTION();
    }
    return 0;
}
