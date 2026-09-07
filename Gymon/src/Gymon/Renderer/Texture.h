#pragma once

#include "Gymon/Core.h"

#include <string>

namespace Gymon {

	enum class TextureFilter
	{
		Nearest = 0,
		Linear
	};

	enum class TextureWrap
	{
		Repeat = 0,
		MirroredRepeat,
		ClampToEdge,
		ClampToBorder
	};

	// How a texture is sampled, and whether it carries a mip chain.
	//
	// The defaults reproduce what the engine did before this struct existed
	// (linear minification, nearest magnification, repeat on both axes, no
	// mips), so existing call sites keep their exact behaviour.
	struct TextureSpecification
	{
		TextureFilter MinFilter = TextureFilter::Linear;
		TextureFilter MagFilter = TextureFilter::Nearest;
		TextureWrap WrapS = TextureWrap::Repeat;
		TextureWrap WrapT = TextureWrap::Repeat;

		// Allocates a full mip chain and generates it after upload. Worth it
		// for anything minified in 3D; pointless for screen-space 2D sprites.
		bool GenerateMips = false;

		// Stores the texture as sRGB so the GPU linearises it on every fetch.
		//
		// Colour textures (albedo, emissive) are authored in sRGB and must be
		// converted to linear before any lighting maths, or everything comes
		// out washed out. Data textures -- normal maps, metallic-roughness,
		// masks -- hold raw numbers, not colours, and must stay untouched.
		bool SRGB = false;

		// Maximum anisotropy, clamped to what the driver supports. 1 is plain
		// trilinear filtering.
		//
		// This matters more than it sounds for a ground plane: a road seen at
		// a grazing angle has a huge texture gradient along the view direction
		// and almost none across it, so trilinear filtering has to pick a mip
		// for the larger of the two and blurs the surface into flat grey.
		// Anisotropic filtering is the difference between a road you can see
		// the aggregate in and a road that is a grey ribbon.
		//
		// Only meaningful together with GenerateMips.
		uint32_t Anisotropy = 1;
	};

	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetRendererID() const = 0;

		// Number of mip levels actually allocated. 1 when mips are disabled.
		virtual uint32_t GetMipLevels() const = 0;

		// Source file, or empty for textures created from raw dimensions.
		virtual const std::string& GetPath() const = 0;

		virtual const TextureSpecification& GetSpecification() const = 0;

		// Sampler state can be changed after creation; storage cannot, so
		// GenerateMips is fixed once the texture exists.
		virtual void SetFilter(TextureFilter minFilter, TextureFilter magFilter) = 0;
		virtual void SetWrap(TextureWrap wrapS, TextureWrap wrapT) = 0;

		virtual void SetData(void* data, uint32_t size) = 0;

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual bool IsLoaded() const = 0;

		virtual bool operator==(const Texture& other) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create(uint32_t width, uint32_t height);
		static Ref<Texture2D> Create(const std::string& path);

		static Ref<Texture2D> Create(uint32_t width, uint32_t height, const TextureSpecification& spec);
		static Ref<Texture2D> Create(const std::string& path, const TextureSpecification& spec);
	};
}
