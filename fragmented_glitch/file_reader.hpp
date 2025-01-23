#pragma once
#include <string_view>

struct FileReader
{
    static void ReadFile(std::wstring_view path);
    static void Reindex(std::wstring_view src, std::wstring_view dst);
};

