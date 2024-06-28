#pragma once

/// <summary>
/// Let's the user select the video device
/// </summary>
/// <returns>The selected video device as an IMFSourceReader.</returns>
winrt::com_ptr<IMFSourceReader> SelectVideoDevice();

/// <summary>
/// Writes the output using the specified Media Foundation Source Reader.
/// </summary>
/// <param name="reader">The Media Foundation Source Reader.</param>
void WriteOutput(const winrt::com_ptr<IMFSourceReader>& reader, bool moovBeforeMdat);
