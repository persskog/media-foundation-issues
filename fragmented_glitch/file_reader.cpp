#include "pch.h"
#include "file_reader.hpp"

namespace winrt {
    using namespace Windows::Foundation;
}

using namespace std::chrono;
using namespace std::chrono_literals;
constexpr DWORD AllStreams = static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS);
constexpr DWORD AnyStream = static_cast<DWORD>(MF_SOURCE_READER_ANY_STREAM);

// Checks if the H264 bitstream contains an IDR picture (key frame)
static bool IDRPicture(uint8_t* bitstream, const DWORD length) noexcept
{
    DWORD totalCount = 0;
    while (totalCount <= (length - 4))
    {
        if (0x0 == bitstream[0] && 0x0 == bitstream[1])
        {
            if ((0x1 == bitstream[2]) ||
                (0x0 == bitstream[2] && 0x1 == bitstream[3]))
            {
                const DWORD startLength = (1 == bitstream[2]) ? 0x3 : 0x4;
                const DWORD temp = bitstream[startLength] & 0x1f;

                if ((0x5 == temp) || (0x9 == temp && bitstream[startLength + 1] == 0x10))
                    return true;

                bitstream += startLength;
                totalCount += startLength;
            }
            else
            {
                ++bitstream;
                ++totalCount;
            }
        }
        else
        {
            ++bitstream;
            ++totalCount;
        }
    }

    return false;
}

static void VerifyCleanPoint(IMFSample* sample, uint32_t cleanpoint) noexcept
{
    // This seems to be a issue on:
    // 2016 LTSB
    // 2019 LTC
    // This is a bug and some reports says that it's fixed, but our
    // experience is that it's not fixed.
    // https://alax.info/blog/1933
    // https://alax.info/blog/1733
    // https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/7758567c-7fb7-44b2-90a5-e1ebcfeacbb3/major-bug-in-latest-windows-10-media-players-for-fragmented-mp4-files?forum=mediafoundationdevelopment
    winrt::com_ptr<IMFMediaBuffer> buffer;
    uint8_t* bitstream{ nullptr };
    DWORD length{};
    THROW_IF_FAILED(sample->GetBufferByIndex(0, buffer.put()));
    THROW_IF_FAILED(buffer->Lock(&bitstream, nullptr, &length));
    if (IDRPicture(bitstream, length))
    {
        sample->SetUINT32(MFSampleExtension_CleanPoint, 1);
    }
    else
    {
        if (cleanpoint)
        {
            PrintLine("Source marked sample as MFSampleExtension_CleanPoint faulty");
        }
    }
    buffer->Unlock();
}

static std::chrono::milliseconds CalculateAudioVideoDiff(int64_t audio, int64_t video)
{
    return duration_cast<milliseconds>(winrt::TimeSpan(audio) - winrt::TimeSpan(video));
}


static auto OpenFile(std::wstring_view path)
{
    winrt::com_ptr<IMFSourceReader> reader;
    THROW_IF_FAILED(::MFCreateSourceReaderFromURL(path.data(), nullptr, reader.put()));
    THROW_IF_FAILED(reader->SetStreamSelection(AllStreams, 1));
    return reader;
}

static auto CreateWriter(std::wstring_view path)
{
    winrt::com_ptr<IMFSinkWriter> writer;
    THROW_IF_FAILED(::MFCreateSinkWriterFromURL(path.data(), nullptr, nullptr, writer.put()));
    return writer;
}

static auto ConfigureWriter(IMFSourceReader* reader, IMFSinkWriter* writer)
{
    DWORD writerAudio{};
    DWORD writerVideo{};
    winrt::com_ptr<IMFMediaType> audio;
    winrt::com_ptr<IMFMediaType> video;
    THROW_IF_FAILED(reader->GetCurrentMediaType(0u, audio.put()));
    THROW_IF_FAILED(reader->GetCurrentMediaType(1u, video.put()));
    THROW_IF_FAILED(writer->AddStream(audio.get(), &writerAudio));
    THROW_IF_FAILED(writer->AddStream(video.get(), &writerVideo));
    THROW_IF_FAILED(writer->BeginWriting());
}

static void SetPosition(IMFSourceReader* reader, winrt::TimeSpan position)
{
    PROPVARIANT var{};
    var.vt = VT_I8;
    var.hVal.QuadPart = position.count();
    HRESULT hr = reader->SetCurrentPosition(GUID_NULL, var);
    LOG_HR_IF(hr, FAILED(hr));
}

void FileReader::ReadFile(std::wstring_view path)
{
    auto reader = OpenFile(path);

    constexpr winrt::TimeSpan BEGIN_POSITION = 0s;//567s;
    constexpr winrt::TimeSpan END_POSITION = 575s;//winrt::TimeSpan::max();

    SetPosition(reader.get(), BEGIN_POSITION);
    uint32_t vfc = 0;
    int64_t accumVideo{};
    int64_t accumAudio{};
    HRESULT hr{};
    winrt::com_ptr<IMFSample> doubleFrame;
    while (SUCCEEDED(hr))
    {
        winrt::com_ptr<IMFSample> sample;

        DWORD actual{};
        DWORD flags{};
        int64_t timestamp{};
        hr = reader->ReadSample(AnyStream, 0, &actual, &flags, &timestamp, sample.put());
        if (sample)
        {
            int64_t duration{};
            sample->GetSampleDuration(&duration);
            uint32_t cp{};
            sample->GetUINT32(MFSampleExtension_CleanPoint, &cp);
            uint32_t discontinuity{};
            sample->GetUINT32(MFSampleExtension_Discontinuity, &discontinuity);
            if (0 == actual)
            {
                accumAudio += duration;
                if (vfc > 1)
                {
                    PrintLine("------");
                }
                PrintLine("# %lld (%lld) a=%lld <-> v=%lld (%lldms) %s",
                    timestamp,
                    duration,
                    accumAudio, 
                    accumVideo,
                    CalculateAudioVideoDiff(accumAudio, accumVideo).count(),
                    discontinuity ? "DISCON" : "");
                vfc = 0;
            }
            else
            {
                accumVideo += duration;
                ++vfc;

                if (vfc == 2)
                {
                    auto f = sample.get();
                    auto s = doubleFrame.get();
                    int g = 0;


                }

                if (cp)
                {
                    PrintLine("= %lld (%lld) a=%lld <-> v=%lld (%lldms) <-- KEYFRAME (%s)",
                        timestamp,
                        duration,
                        accumAudio,
                        accumVideo,
                        CalculateAudioVideoDiff(accumAudio, accumVideo).count(),
                        discontinuity ? "DISCON" : "");
                }
                else
                {
                    PrintLine("= %lld (%lld) a=%lld <-> v=%lld (%lldms) %s",
                        timestamp,
                        duration,
                        accumAudio,
                        accumVideo,
                        CalculateAudioVideoDiff(accumAudio, accumVideo).count(),
                        discontinuity ? "DISCON" : "");
                }

                doubleFrame = sample;

                if (timestamp >= END_POSITION.count())
                {
                    break;
                }
            }
        }
        else
        {
            if (flags & MF_SOURCE_READERF_STREAMTICK)
            {
                PrintLine("MF_SOURCE_READERF_STREAMTICK: Stream (%u) %lld", actual, timestamp);
            }
            else if (flags & MF_SOURCE_READERF_ERROR)
            {
                PrintLine("MF_SOURCE_READERF_ERROR: Stream (%u)", actual);
            }
        }
    }
}

void FileReader::Reindex(std::wstring_view src, std::wstring_view dst)
{
    auto reader = OpenFile(src);
    auto writer = CreateWriter(dst);
    ConfigureWriter(reader.get(), writer.get());

    HRESULT hr{};

    bool eosAudio{};
    bool eosVideo{};

    constexpr winrt::TimeSpan BEGIN_POSITION = 0s;
    constexpr winrt::TimeSpan END_POSITION = 575s;
    SetPosition(reader.get(), BEGIN_POSITION);

    int64_t audioWritten{};
    int64_t videoWritten{};

    while (SUCCEEDED(hr) && !(eosAudio && eosVideo))
    {
        winrt::com_ptr<IMFSample> sample;
        DWORD actual{};
        DWORD flags{};
        int64_t timestamp{};
        hr = reader->ReadSample(AnyStream, 0, &actual, &flags, &timestamp, sample.put());
        if (sample)
        {
            int64_t duration{};
            sample->GetSampleDuration(&duration);
            uint32_t cp{};
            sample->GetUINT32(MFSampleExtension_CleanPoint, &cp);
            uint32_t discontinuity{};
            sample->GetUINT32(MFSampleExtension_Discontinuity, &discontinuity);

            

            

            if (0 == actual) // audio
            {
                if (discontinuity && audioWritten > 0)
                {
                    sample->DeleteItem(MFSampleExtension_Discontinuity);
                }

                //PrintLine("Audio Cleanpoint (%u) Discont (%u): %lld", cp, discontinuity, videoWritten);
                sample->SetSampleTime(audioWritten);
                THROW_IF_FAILED(writer->WriteSample(0, sample.get()));
                audioWritten += duration;
            }
            else
            {
                if (discontinuity && videoWritten > 0)
                {
                    sample->DeleteItem(MFSampleExtension_Discontinuity);
                }

                
                sample->SetSampleTime(videoWritten);
                THROW_IF_FAILED(writer->WriteSample(1, sample.get()));
                videoWritten += duration;

                PrintLine("a=%lld <-> v=%lld (%lldms)",
                    audioWritten,
                    videoWritten,
                    CalculateAudioVideoDiff(audioWritten, videoWritten).count());
            }

            if (timestamp >= END_POSITION.count())
            {
                break;
            }
        }
        else
        {
            if (flags & MF_SOURCE_READERF_STREAMTICK)
            {
                PrintLine("MF_SOURCE_READERF_STREAMTICK: Stream (%u) %lld", actual, timestamp);
                THROW_IF_FAILED(writer->SendStreamTick(actual, timestamp));
            }
            else if (flags & MF_SOURCE_READERF_ERROR)
            {
                PrintLine("MF_SOURCE_READERF_ERROR: Stream (%u)", actual);
            }
            else if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
            {
                if (0 == actual)
                {
                    eosAudio = true;
                }
                else
                {
                    eosVideo = true;
                }
            }
        }
    }
    THROW_IF_FAILED(writer->Finalize());
    PrintLine("Done");
}
