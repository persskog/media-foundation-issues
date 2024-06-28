#include "pch.h"
#include "example.hpp"

int main()
{
    try
    {
        auto ctx = InitializeApp();
        auto device = SelectVideoDevice();
        WriteOutput(device, true);
    }
    CATCH_LOG();
    return 0;
}

