#pragma once
#include <string>
#include <array>
#include <type_traits>
#include <concepts>
#include <span>
#include <algorithm>
#include <cstring>

template <typename CharT, typename U>
constexpr auto determine_fmt()
{
    static_assert(std::is_arithmetic_v<U>);
    constexpr bool is_wide = std::is_same_v<CharT, wchar_t>;
    if constexpr (std::is_same_v<U, uint64_t>)
    {
        if constexpr (is_wide)
        {
            return L"%llu";
        }
        else
        {
            return "%llu";
        }
    }
    else if constexpr (std::is_same_v<U, int64_t>)
    {
        if constexpr (is_wide)
        {
            return L"%lld";
        }
        else
        {
            return "%lld";
        }
    }
    else if constexpr (std::is_same_v<U, uint32_t>)
    {
        if constexpr (is_wide)
        {
            return L"%u";
        }
        else
        {
            return "%u";
        }
    }
    else if constexpr (std::is_same_v<U, int32_t>)
    {
        if constexpr (is_wide)
        {
            return L"%d";
        }
        else
        {
            return "%d";
        }
    }
    else if constexpr (std::is_same_v<U, uint16_t>)
    {
        if constexpr (is_wide)
        {
            return L"%hu";
        }
        else
        {
            return "%hu";
        }
    }
    else if constexpr (std::is_same_v<U, int16_t>)
    {
        if constexpr (is_wide)
        {
            return L"%hi";
        }
        else
        {
            return "%hi";
        }
    }
    else if constexpr (std::is_same_v<U, uint8_t>)
    {
        if constexpr (is_wide)
        {
            return L"%hhu";
        }
        else
        {
            return "%hhu";
        }
    }
    else if constexpr (std::is_same_v<U, int8_t>)
    {
        if constexpr (is_wide)
        {
            return L"%hhi";
        }
        else
        {
            return "%hhi";
        }
    }
    else if constexpr (std::is_same_v<U, char>)
    {
        if constexpr (is_wide)
        {
            return L"%c";
        }
        else
        {
            return "%c";
        }
    }
    else if constexpr (std::is_same_v<U, wchar_t>)
    {
        if constexpr (is_wide)
        {
            return L"%lc";
        }
        else
        {
            return "%lc";
        }
    }
    else if constexpr (std::is_same_v<U, double>)
    {
        if constexpr (is_wide)
        {
            return L"%lf";
        }
        else
        {
            return "%lf";
        }
    }
    else
    {
        static_assert(std::is_same_v<U, float>);
        if constexpr (is_wide)
        {
            return L"%f";
        }
        else
        {
            return "%f";
        }
    }
}


template <typename T, size_t N> requires std::is_same_v<T, wchar_t> || std::is_same_v<T, char>
class basic_string_builder
{
    using type_t = T;

    size_t m_written{};
    std::array<type_t, N> m_buffer{};

    static constexpr bool is_wide = std::is_same_v<type_t, wchar_t>;

public:
    basic_string_builder() noexcept = default;

    // Appends formatted data to the string builder.
    template <typename ... Args>
    auto append_format(const type_t* const format, Args ... args) noexcept
    {
        return printf(format, std::forward<Args>(args)...);
    }

    // Appends a same-character-type string to the string builder without formatting.
    auto append_string(std::basic_string_view<type_t> str) noexcept
    {
        // Reserve one char for the terminating null
        const auto available = (m_buffer.size() > m_written) ? (m_buffer.size() - m_written - 1) : 0;
        const auto to_copy = std::min(available, str.size());

        if (to_copy > 0)
        {
            std::memcpy(m_buffer.data() + m_written, str.data(), to_copy * sizeof(type_t));
            m_written += to_copy;
        }

        // Ensure NUL-termination if there is room
        if (m_written < m_buffer.size())
        {
            m_buffer[m_written] = type_t{};
        }
        return m_written;
    }

    // Appends a C-style string of the same character type.
    auto append_cstr(const type_t* s) noexcept
    {
        if (!s) return m_written;
        const auto len = std::char_traits<type_t>::length(s);
        return append_string(std::basic_string_view<type_t>(s, len));
    }

    // Appends an arithmetic value to the string builder.
    template <typename U>
    auto append(const U value) noexcept
    {
        return printf(determine_fmt<type_t, U>(), value);
    }

    constexpr auto data() noexcept { return m_buffer.data(); }
    constexpr auto data() const noexcept { return m_buffer.data(); }
    constexpr size_t length() const noexcept { return m_written; }
    constexpr auto empty() const noexcept { return 0 == m_written; }
    constexpr auto max_size() const noexcept { return m_buffer.max_size(); }

    void clear() noexcept
    {
        m_written = 0;
        m_buffer[0] = type_t{};
    }

    // Converts the contents of the string builder to a std::string or std::wstring.
    constexpr auto to_string()
    {
        if constexpr (is_wide)
        {
            return std::wstring(data());
        }
        else
        {
            return std::string(data());
        }
    }

private:
    auto write_position() noexcept
    {
        const auto written = length();
        const auto remain = (m_buffer.size() > written) ? (m_buffer.size() - written) : 0;
        return std::span<type_t>{ m_buffer.begin() + written, remain };
    }

    template <typename ... Args>
    auto printf(const type_t* const format, Args ... args) noexcept
    {
        const auto buffer = write_position();
        if (buffer.size() < 2)
        {
            // Not enough room to write anything (need at least room for '\0').
            return m_written;
        }

        // swprintf_s / sprintf_s returns number of chars written (excluding null) or a negative value on error.
        if constexpr (is_wide)
        {
            return update_written(::swprintf_s(buffer.data(), buffer.size(), format, args ...));
        }
        else
        {
            return update_written(::sprintf_s(buffer.data(), buffer.size(), format, args ...));
        }
    }

    auto update_written(const int32_t c) noexcept
    {
        if (c > 0)
        {
            // Ensure we don't overflow (c should never exceed remaining capacity - 1).
            const auto remaining = (m_buffer.size() > m_written) ? (m_buffer.size() - m_written) : 0;
            const auto add = static_cast<size_t>(std::min<int32_t>(c, static_cast<int32_t>(remaining)));
            m_written += add;
            // Ensure terminal NUL
            if (m_written < m_buffer.size())
            {
                m_buffer[m_written] = type_t{};
            }
        }
        return m_written;
    }
};

// Type aliases for char and wchar_t specializations.
template <size_t N>
using string_builder = basic_string_builder<char, N>;

template <size_t N>
using wstring_builder = basic_string_builder<wchar_t, N>;

