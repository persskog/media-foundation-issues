#pragma once
#pragma comment(lib, "mf")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WINRT_LEAN_AND_MEAN
#define WINRT_LEAN_AND_MEAN
#endif
#include <mfidl.h>
#include <mfapi.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <mfcaptureengine.h>

#include <iostream>

#include "wil/resource.h"

#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Foundation.Collections.h"

template <typename ... Args>
static void PrintLine(char const* const format, Args ... args) noexcept
{
    char buffer[1024] = {};
    ::sprintf_s(buffer, std::size(buffer), format, args ...);
    ::printf_s("%s\n", buffer);
}