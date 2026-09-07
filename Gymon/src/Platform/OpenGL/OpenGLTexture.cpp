#include "gypch.h"
#include "Platform/OpenGL/OpenGLTexture.h"

#include <stb_image.h>

namespace Gymon {

	namespace {

		GLenum ToGLWrap(TextureWrap wrap)
		{
			switch (wrap)
			{
				case TextureWrap::Repeat:         return GL_REPEAT;
				case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
				case TextureWrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
				case TextureWrap::ClampToBorder:  return GL_CLAMP_TO_BORDER;
			}
			GY_CORE_ASSERT(false, "Unknown TextureWrap!");
			return GL_REPEAT;
		}

		// Magnification has no mip levels to choose between, so it is always
		// the plain filter.
		GLenum ToGLMagFilter(TextureFilter filter)
		{
			return filter == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR;
		}

		// Minification picks a mipmapped variant only when a chain exists.
		// Asking for one without mips samples an incomplete texture, which
		// renders black.
		GLenum ToGLMinFilter(TextureFilter filter, bool hasMips)
		{
			if (!hasMips)
				return filter == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR;

			return filter == TextureFilter::Nearest
				? GL_NEAREST_MIPMAP_NEAREST
				: GL_LINEAR_MIPMAP_LINEAR;
		}

		uint32_t MipLevelsFor(uint32_t width, uint32_t height)
		{
			uint32_t levels = 1;
			uint32_t size = width > height ? width : height;
			while (size > 1)
			{
				size >>= 1;
				levels++;
			}
			return levels;
		}
	}

	void OpenGLTexture2D::ApplySampling()
	{
		const bool hasMips = m_MipLevels > 1;

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, ToGLMinFilter(m_Specification.MinFilter, hasMips));
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, ToGLMagFilter(m_Specification.MagFilter));

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, ToGLWrap(m_Specification.WrapS));
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, ToGLWrap(m_Specification.WrapT));

		if (hasMips && m_Specification.Anisotropy > 1)
		{
			// Core in 4.6, an ubiquitous extension before that, and the
			// enum is the same either way. Query the driver's ceiling
			// rather than assuming 16: asking for more is an error.
			float maxSupported = 1.0f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxSupported);

			const float requested = (float)m_Specification.Anisotropy;
			glTextureParameterf(m_RendererID, GL_TEXTURE_MAX_ANISOTROPY,
				requested < maxSupported ? requested : maxSupported);
		}
	}

	OpenGLTexture2D::OpenGLTexture2D(uint32_t width, uint32_t height, const TextureSpecification& spec)
		: m_Specification(spec), m_Width(width), m_Height(height)
	{
		m_InternalFormat = spec.SRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
		m_DataFormat = GL_RGBA;

		m_MipLevels = spec.GenerateMips ? MipLevelsFor(m_Width, m_Height) : 1;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, m_MipLevels, m_InternalFormat, m_Width, m_Height);

		ApplySampling();
	}

	OpenGLTexture2D::OpenGLTexture2D(const std::string& path, const TextureSpecification& spec)
		: m_Specification(spec), m_Path(path)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

		if (data)
		{
			m_IsLoaded = true;

			m_Width = width;
			m_Height = height;

			GLenum internalFormat = 0, dataFormat = 0;
			if (channels == 4)
			{
				internalFormat = spec.SRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
				dataFormat = GL_RGBA;
			}
			else if (channels == 3)
			{
				internalFormat = spec.SRGB ? GL_SRGB8 : GL_RGB8;
				dataFormat = GL_RGB;
			}

			m_InternalFormat = internalFormat;
			m_DataFormat = dataFormat;

			GY_CORE_ASSERT(internalFormat & dataFormat, "Format not supported!");

			m_MipLevels = spec.GenerateMips ? MipLevelsFor(m_Width, m_Height) : 1;

			glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
			glTextureStorage2D(m_RendererID, m_MipLevels, internalFormat, m_Width, m_Height);

			ApplySampling();

			glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);

			// Mips are generated from level 0, so this has to follow the upload.
			if (m_MipLevels > 1)
				glGenerateTextureMipmap(m_RendererID);

			stbi_image_free(data);
		}
		else
		{
			GY_CORE_ERROR("Failed to load image '{0}'", path);
		}
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTexture2D::SetFilter(TextureFilter minFilter, TextureFilter magFilter)
	{
		m_Specification.MinFilter = minFilter;
		m_Specification.MagFilter = magFilter;
		ApplySampling();
	}

	void OpenGLTexture2D::SetWrap(TextureWrap wrapS, TextureWrap wrapT)
	{
		m_Specification.WrapS = wrapS;
		m_Specification.WrapT = wrapT;
		ApplySampling();
	}

	void OpenGLTexture2D::SetData(void* data, uint32_t size)
	{
		const uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
		GY_CORE_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);

		// Level 0 just changed, so any mip chain below it is now stale.
		if (m_MipLevels > 1)
			glGenerateTextureMipmap(m_RendererID);
	}

	void OpenGLTexture2D::Bind(uint32_t slot) const
	{
		glBindTextureUnit(slot, m_RendererID);
	}

	Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
	{
		return CreateRef<OpenGLTexture2D>(width, height, TextureSpecification{});
	}

	Ref<Texture2D> Texture2D::Create(const std::string& path)
	{
		return CreateRef<OpenGLTexture2D>(path, TextureSpecification{});
	}

	Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height, const TextureSpecification& spec)
	{
		return CreateRef<OpenGLTexture2D>(width, height, spec);
	}

	Ref<Texture2D> Texture2D::Create(const std::string& path, const TextureSpecification& spec)
	{
		return CreateRef<OpenGLTexture2D>(path, spec);
	}
}
