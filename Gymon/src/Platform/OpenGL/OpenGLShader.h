#pragma once

#include "Gymon/Renderer/Shader.h"

#include <glm/glm.hpp>

// Forward declaration so we don't leak the GL typedef into the header.
typedef unsigned int GLenum;

namespace Gymon {

	class OpenGLShader : public Shader
	{
	public:
		OpenGLShader(const std::string& filepath);
		OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
		virtual ~OpenGLShader();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void SetInt(const std::string& name, int value) override;
		virtual void SetIntArray(const std::string& name, int* values, uint32_t count) override;
		virtual void SetFloat(const std::string& name, float value) override;
		virtual void SetFloat2(const std::string& name, const glm::vec2& value) override;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;
		virtual void SetMat3(const std::string& name, const glm::mat3& value) override;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) override;

		virtual const std::string& GetName() const override { return m_Name; }

		virtual bool IsReloadable() const override { return !m_FilePath.empty(); }
		virtual bool Reload() override;
		virtual bool HasSourceChanged() const override;

		void UploadUniformInt(const std::string& name, int value);
		void UploadUniformIntArray(const std::string& name, int* values, uint32_t count);
		void UploadUniformFloat(const std::string& name, float value);
		void UploadUniformFloat2(const std::string& name, const glm::vec2& value);
		void UploadUniformFloat3(const std::string& name, const glm::vec3& value);
		void UploadUniformFloat4(const std::string& name, const glm::vec4& value);
		void UploadUniformMat3(const std::string& name, const glm::mat3& matrix);
		void UploadUniformMat4(const std::string& name, const glm::mat4& matrix);
	private:
		std::string ReadFile(const std::string& filepath);
		std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);
		void Compile(const std::unordered_map<GLenum, std::string>& shaderSources);
		// Compiles into a fresh program and returns it, leaving m_RendererID
		// alone. Returns 0 on failure so the caller can keep the old program.
		uint32_t CompileProgram(const std::unordered_map<GLenum, std::string>& shaderSources, bool logErrors);
		// Last write time of m_FilePath, or 0 when it cannot be read.
		uint64_t SourceTimestamp() const;
	private:
		uint32_t m_RendererID;
		std::string m_Name;
		// Empty for shaders built from in-memory source; those cannot reload.
		std::string m_FilePath;
		uint64_t m_SourceTimestamp = 0;
	};
}
