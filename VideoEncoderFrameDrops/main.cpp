#include "pch.h"
//#include "file_reader.hpp"

int main()
{
    try
    {
        auto app{ InitializeApp() };
    }
    catch (...)
    {
        LOG_CAUGHT_EXCEPTION();
    }
    WaitForQKeyPress();
    return 0;
}
