#pragma once

#include "Gymon/Core.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Gymon {

	// Writes an 8-bit RGB or RGBA image to a PNG file.
	//
	// The encoder emits stored (uncompressed) deflate blocks rather than
	// pulling in zlib. That costs file size, which does not matter for a
	// screenshot written once, and buys a dependency-free implementation --
	// the engine already vendors enough.
	//
	// Rows are read top-to-bottom, so a caller with bottom-up data (anything
	// coming out of glReadPixels) must flip first. CaptureFramebuffer does.
	bool WritePNG(const std::string& path, uint32_t width, uint32_t height,
		uint32_t channels, const uint8_t* pixels);

	// Reads the currently bound framebuffer back to the CPU and writes it as a
	// PNG. Call after the frame has been drawn but before the buffer swap:
	// once swapped, the back buffer's contents are undefined.
	//
	// Returns false if the read or the write failed; the reason is logged.
	bool CaptureFramebuffer(const std::string& path, uint32_t width, uint32_t height);
}
