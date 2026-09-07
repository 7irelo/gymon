#include "gypch.h"
#include "Gymon/Utils/Screenshot.h"

#include <glad/glad.h>

#include <cstdio>
#include <filesystem>

namespace Gymon {

	namespace {

		void PushBE32(std::vector<uint8_t>& out, uint32_t value)
		{
			out.push_back((uint8_t)(value >> 24));
			out.push_back((uint8_t)(value >> 16));
			out.push_back((uint8_t)(value >> 8));
			out.push_back((uint8_t)value);
		}

		uint32_t Crc32(const uint8_t* data, size_t length)
		{
			// Built on first use rather than baked in as a literal table: 256
			// entries of magic numbers in the source would be harder to trust
			// than four lines that derive them.
			static uint32_t table[256];
			static bool initialised = false;
			if (!initialised)
			{
				for (uint32_t i = 0; i < 256; i++)
				{
					uint32_t c = i;
					for (int k = 0; k < 8; k++)
						c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
					table[i] = c;
				}
				initialised = true;
			}

			uint32_t crc = 0xFFFFFFFFu;
			for (size_t i = 0; i < length; i++)
				crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
			return crc ^ 0xFFFFFFFFu;
		}

		uint32_t Adler32(const uint8_t* data, size_t length)
		{
			uint32_t a = 1, b = 0;
			for (size_t i = 0; i < length; i++)
			{
				a = (a + data[i]) % 65521;
				b = (b + a) % 65521;
			}
			return (b << 16) | a;
		}

		void WriteChunk(std::vector<uint8_t>& out, const char type[4],
			const uint8_t* data, size_t length)
		{
			PushBE32(out, (uint32_t)length);

			const size_t crcStart = out.size();
			out.insert(out.end(), type, type + 4);
			out.insert(out.end(), data, data + length);

			PushBE32(out, Crc32(out.data() + crcStart, out.size() - crcStart));
		}

		// zlib stream with no compression: a two-byte header, then stored
		// deflate blocks of at most 65535 bytes each, then the Adler-32 of the
		// uncompressed data.
		std::vector<uint8_t> StoredZlib(const std::vector<uint8_t>& raw)
		{
			std::vector<uint8_t> out;
			out.reserve(raw.size() + raw.size() / 65535 * 5 + 16);

			out.push_back(0x78); // CMF: deflate, 32K window
			out.push_back(0x01); // FLG: no dictionary, fastest -- checksum-valid

			size_t offset = 0;
			while (offset < raw.size() || raw.empty())
			{
				const size_t chunk = std::min<size_t>(65535, raw.size() - offset);
				const bool last = (offset + chunk) >= raw.size();

				out.push_back(last ? 1 : 0);
				out.push_back((uint8_t)(chunk & 0xFF));
				out.push_back((uint8_t)(chunk >> 8));
				out.push_back((uint8_t)(~chunk & 0xFF));
				out.push_back((uint8_t)((~chunk >> 8) & 0xFF));

				out.insert(out.end(), raw.begin() + offset, raw.begin() + offset + chunk);
				offset += chunk;

				if (last)
					break;
			}

			PushBE32(out, Adler32(raw.data(), raw.size()));
			return out;
		}
	}

	bool WritePNG(const std::string& path, uint32_t width, uint32_t height,
		uint32_t channels, const uint8_t* pixels)
	{
		if (width == 0 || height == 0 || (channels != 3 && channels != 4) || !pixels)
		{
			GY_CORE_ERROR("WritePNG: invalid image ({0}x{1}, {2} channels)", width, height, channels);
			return false;
		}

		// PNG scanlines are each prefixed with a filter byte. Filter 0 (none)
		// keeps this simple; with stored deflate there is nothing to gain from
		// a smarter filter anyway.
		std::vector<uint8_t> raw;
		raw.reserve((size_t)height * (1 + (size_t)width * channels));
		for (uint32_t y = 0; y < height; y++)
		{
			raw.push_back(0);
			const uint8_t* row = pixels + (size_t)y * width * channels;
			raw.insert(raw.end(), row, row + (size_t)width * channels);
		}

		std::vector<uint8_t> png = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };

		uint8_t ihdr[13];
		ihdr[0] = (uint8_t)(width >> 24);  ihdr[1] = (uint8_t)(width >> 16);
		ihdr[2] = (uint8_t)(width >> 8);   ihdr[3] = (uint8_t)width;
		ihdr[4] = (uint8_t)(height >> 24); ihdr[5] = (uint8_t)(height >> 16);
		ihdr[6] = (uint8_t)(height >> 8);  ihdr[7] = (uint8_t)height;
		ihdr[8] = 8;                            // bit depth
		ihdr[9] = channels == 4 ? 6 : 2;        // colour type: RGBA or RGB
		ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0; // deflate, adaptive filter, no interlace
		WriteChunk(png, "IHDR", ihdr, sizeof(ihdr));

		const std::vector<uint8_t> idat = StoredZlib(raw);
		WriteChunk(png, "IDAT", idat.data(), idat.size());
		WriteChunk(png, "IEND", nullptr, 0);

		std::error_code ec;
		const auto parent = std::filesystem::path(path).parent_path();
		if (!parent.empty())
			std::filesystem::create_directories(parent, ec);

		FILE* file = std::fopen(path.c_str(), "wb");
		if (!file)
		{
			GY_CORE_ERROR("WritePNG: could not open '{0}' for writing", path);
			return false;
		}

		const size_t written = std::fwrite(png.data(), 1, png.size(), file);
		std::fclose(file);

		if (written != png.size())
		{
			GY_CORE_ERROR("WritePNG: short write to '{0}'", path);
			return false;
		}

		GY_CORE_INFO("Screenshot written: {0} ({1}x{2})", path, width, height);
		return true;
	}

	bool CaptureFramebuffer(const std::string& path, uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
			return false;

		std::vector<uint8_t> pixels((size_t)width * height * 4);

		// The default alignment of 4 would pad rows of a width that is not a
		// multiple of 4; ask for tightly packed rows instead of compensating.
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, (GLsizei)width, (GLsizei)height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

		// GL's origin is bottom-left, PNG's is top-left.
		const size_t stride = (size_t)width * 4;
		std::vector<uint8_t> flipped((size_t)width * height * 4);
		for (uint32_t y = 0; y < height; y++)
			std::memcpy(flipped.data() + y * stride,
				pixels.data() + (size_t)(height - 1 - y) * stride, stride);

		return WritePNG(path, width, height, 4, flipped.data());
	}
}
