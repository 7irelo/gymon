#pragma once

#include "Gymon/Renderer/Texture.h"

#include <glad/glad.h>

namespace Gymon {

	class OpenGLTexture2D : public Texture2D
	{
	public:
		OpenGLTexture2D(uint32_t width, uint32_t height, const TextureSpecification& spec = {});
		OpenGLTexture2D(const std::string& path, const TextureSpecification& spec = {});
		virtual ~OpenGLTexture2D();

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetRendererID() const override { return m_RendererID; }
		virtual uint32_t GetMipLevels() const override { return m_MipLevels; }
		virtual const std::string& GetPath() const override { return m_Path; }
		virtual const TextureSpecification& GetSpecification() const override { return m_Specification; }

		virtual void SetFilter(TextureFilter minFilter, TextureFilter magFilter) override;
		virtual void SetWrap(TextureWrap wrapS, TextureWrap wrapT) override;

		virtual void SetData(void* data, uint32_t size) override;

		virtual void Bind(uint32_t slot = 0) const override;

		virtual bool IsLoaded() const override { return m_IsLoaded; }

		virtual bool operator==(const Texture& other) const override
		{
			return m_RendererID == other.GetRendererID();
		}
	private:
		// Pushes m_Specification's sampler state onto the live texture object.
		void ApplySampling();
	private:
		TextureSpecification m_Specification;
		std::string m_Path;
		bool m_IsLoaded = false;
		uint32_t m_Width = 0, m_Height = 0;
		uint32_t m_RendererID = 0;
		uint32_t m_MipLevels = 1;
		GLenum m_InternalFormat = 0, m_DataFormat = 0;
	};
}
