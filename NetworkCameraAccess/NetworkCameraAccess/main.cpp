#include "pch.h"

int main()
{
    try
    {
        auto ctx{ InitializeApp() };
    }
    CATCH_LOG();
    return 0;
}

