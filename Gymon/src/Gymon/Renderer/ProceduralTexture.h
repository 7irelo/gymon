#pragma once

#include "Gymon/Core.h"
#include "Gymon/Renderer/Texture.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <functional>
#include <vector>

namespace Gymon {

	// Textures generated in code rather than loaded from disk.
	//
	// A renderer with good lighting and untextured surfaces still looks like a
	// tech demo: real materials vary across a surface, and it is that variation
	// the eye reads as "a thing" rather than "a shape". Shipping a few hundred
	// megabytes of scanned PBR sets is not an option for a repository like
	// this, so the textures are synthesised instead.
	//
	// Everything here is tileable: the noise lattice wraps at the texture
	// period, so a road surface repeated a hundred times along a track has no
	// visible seam and no repeating landmark.
	class Noise
	{
	public:
		// Value noise on a wrapping integer lattice. `period` is in lattice
		// cells, and must match across every octave for the result to tile.
		static float Value(float x, float y, uint32_t period, uint32_t seed);

		// Fractal sum of value noise. Each octave doubles the frequency and
		// halves the amplitude, which is what turns a smooth blobby field into
		// something with detail at every scale.
		static float FBM(float x, float y, uint32_t period, uint32_t seed,
			int octaves = 5, float lacunarity = 2.0f, float gain = 0.5f);
	};

	// A CPU-side RGBA8 image, before it becomes a GPU texture.
	//
	// Kept separate from Texture2D so a generator can build a height field,
	// derive a normal map from it, and upload both -- deriving one texture
	// from another is impossible once the data is on the GPU.
	struct ImageBuffer
	{
		uint32_t Width = 0;
		uint32_t Height = 0;
		std::vector<uint8_t> Pixels; // RGBA8

		ImageBuffer(uint32_t width, uint32_t height);

		void Set(uint32_t x, uint32_t y, const glm::vec4& color);
		glm::vec4 Get(uint32_t x, uint32_t y) const;

		// Fills every texel from a function of normalised (u, v) coordinates.
		void Fill(const std::function<glm::vec4(float, float)>& shader);

		// Uploads as an sRGB colour texture with mips: the right choice for
		// anything that represents a colour a person would see.
		Ref<Texture2D> ToColorTexture() const;
		// Uploads as linear data with mips: normal maps, roughness, masks.
		Ref<Texture2D> ToDataTexture() const;
	};

	// The material set the racing demo is built from. Each returns albedo,
	// normal and metallic-roughness maps that belong together.
	struct MaterialTextures
	{
		Ref<Texture2D> Albedo;
		Ref<Texture2D> Normal;
		Ref<Texture2D> MetallicRoughness;
	};

	class ProceduralTextures
	{
	public:
		// Worn asphalt: dark aggregate, lighter stone speckle, faint tyre
		// polish down the wheel lines.
		static MaterialTextures Asphalt(uint32_t size = 512, uint32_t seed = 1337);

		// Red and white kerbing, the diagonal stripe used on every circuit.
		static MaterialTextures Kerb(uint32_t size = 256);

		// Coarse grass for the run-off areas.
		static MaterialTextures Grass(uint32_t size = 512, uint32_t seed = 991);

		// Poured concrete for barriers and pit structures.
		static MaterialTextures Concrete(uint32_t size = 512, uint32_t seed = 77);

		// Automotive paint: near-uniform colour, very low roughness, with the
		// faint orange peel a real clear coat has.
		static MaterialTextures CarPaint(const glm::vec3& color, uint32_t size = 256);

		// Rubber tyre with a tread pattern and a sidewall band.
		static MaterialTextures Tyre(uint32_t size = 256);

		// Derives a tangent-space normal map from a height field using a Sobel
		// filter. `strength` scales the slope; 1 is subtle, 4 is pronounced.
		static Ref<Texture2D> NormalFromHeight(const std::vector<float>& height,
			uint32_t width, uint32_t height_, float strength = 2.0f);
	};
}
