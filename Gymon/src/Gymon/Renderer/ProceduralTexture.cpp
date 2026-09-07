#include "gypch.h"
#include "Gymon/Renderer/ProceduralTexture.h"

#include <algorithm>
#include <cmath>

namespace Gymon {

	namespace {

		// Integer hash. Not cryptographic and not trying to be: it needs to be
		// cheap, deterministic across runs, and free of visible structure at
		// the scales a texture is viewed at.
		uint32_t Hash(uint32_t x, uint32_t y, uint32_t seed)
		{
			uint32_t h = seed + x * 374761393u + y * 668265263u;
			h = (h ^ (h >> 13)) * 1274126177u;
			return h ^ (h >> 16);
		}

		float HashFloat(uint32_t x, uint32_t y, uint32_t seed)
		{
			return (float)(Hash(x, y, seed) & 0xFFFFFF) / (float)0xFFFFFF;
		}

		// Smoothstep-style interpolant. Linear interpolation between lattice
		// points leaves visible creases along the cell boundaries; this has a
		// zero first derivative at both ends, so the field is C1 continuous.
		float Fade(float t)
		{
			return t * t * (3.0f - 2.0f * t);
		}

		float Clamp01(float v)
		{
			return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
		}

		uint8_t ToByte(float v)
		{
			return (uint8_t)std::lround(Clamp01(v) * 255.0f);
		}

		glm::vec3 Mix(const glm::vec3& a, const glm::vec3& b, float t)
		{
			return a + (b - a) * Clamp01(t);
		}

		// Wraps a lattice coordinate into [0, period), including negatives.
		uint32_t Wrap(int v, uint32_t period)
		{
			const int p = (int)period;
			return (uint32_t)(((v % p) + p) % p);
		}
	}

	float Noise::Value(float x, float y, uint32_t period, uint32_t seed)
	{
		const int x0 = (int)std::floor(x);
		const int y0 = (int)std::floor(y);

		const float fx = Fade(x - (float)x0);
		const float fy = Fade(y - (float)y0);

		const uint32_t wx0 = Wrap(x0, period), wx1 = Wrap(x0 + 1, period);
		const uint32_t wy0 = Wrap(y0, period), wy1 = Wrap(y0 + 1, period);

		const float v00 = HashFloat(wx0, wy0, seed);
		const float v10 = HashFloat(wx1, wy0, seed);
		const float v01 = HashFloat(wx0, wy1, seed);
		const float v11 = HashFloat(wx1, wy1, seed);

		const float top = v00 + (v10 - v00) * fx;
		const float bottom = v01 + (v11 - v01) * fx;
		return top + (bottom - top) * fy;
	}

	float Noise::FBM(float x, float y, uint32_t period, uint32_t seed,
		int octaves, float lacunarity, float gain)
	{
		float sum = 0.0f;
		float amplitude = 1.0f;
		float total = 0.0f;
		float frequency = 1.0f;
		uint32_t octavePeriod = period;

		for (int i = 0; i < octaves; i++)
		{
			sum += Noise::Value(x * frequency, y * frequency, octavePeriod, seed + i * 7919u) * amplitude;
			total += amplitude;

			frequency *= lacunarity;
			amplitude *= gain;

			// The period has to scale with the frequency or the higher octaves
			// stop tiling and reintroduce the seam the wrapping avoids.
			octavePeriod = (uint32_t)std::max(1.0f, octavePeriod * lacunarity);
		}

		return total > 0.0f ? sum / total : 0.0f;
	}

	ImageBuffer::ImageBuffer(uint32_t width, uint32_t height)
		: Width(width), Height(height), Pixels((size_t)width * height * 4, 0)
	{
	}

	void ImageBuffer::Set(uint32_t x, uint32_t y, const glm::vec4& color)
	{
		const size_t index = ((size_t)y * Width + x) * 4;
		Pixels[index + 0] = ToByte(color.r);
		Pixels[index + 1] = ToByte(color.g);
		Pixels[index + 2] = ToByte(color.b);
		Pixels[index + 3] = ToByte(color.a);
	}

	glm::vec4 ImageBuffer::Get(uint32_t x, uint32_t y) const
	{
		const size_t index = ((size_t)y * Width + x) * 4;
		return glm::vec4(
			Pixels[index + 0] / 255.0f,
			Pixels[index + 1] / 255.0f,
			Pixels[index + 2] / 255.0f,
			Pixels[index + 3] / 255.0f);
	}

	void ImageBuffer::Fill(const std::function<glm::vec4(float, float)>& shader)
	{
		for (uint32_t y = 0; y < Height; y++)
		{
			const float v = (float)y / (float)Height;
			for (uint32_t x = 0; x < Width; x++)
				Set(x, y, shader((float)x / (float)Width, v));
		}
	}

	namespace {

		Ref<Texture2D> Upload(const ImageBuffer& image, bool srgb)
		{
			TextureSpecification spec;
			spec.SRGB = srgb;
			spec.GenerateMips = true;
			// Trilinear: without mips a tiled road surface turns into aliasing
			// noise the moment it is seen at a shallow angle, which is most of
			// the time in a driving view.
			spec.MinFilter = TextureFilter::Linear;
			spec.MagFilter = TextureFilter::Linear;
			spec.WrapS = TextureWrap::Repeat;
			spec.WrapT = TextureWrap::Repeat;
			spec.Anisotropy = 16;

			auto texture = Texture2D::Create(image.Width, image.Height, spec);
			texture->SetData((void*)image.Pixels.data(), (uint32_t)image.Pixels.size());
			return texture;
		}
	}

	Ref<Texture2D> ImageBuffer::ToColorTexture() const { return Upload(*this, true); }
	Ref<Texture2D> ImageBuffer::ToDataTexture() const { return Upload(*this, false); }

	Ref<Texture2D> ProceduralTextures::NormalFromHeight(const std::vector<float>& height,
		uint32_t width, uint32_t height_, float strength)
	{
		ImageBuffer normal(width, height_);

		auto at = [&](int x, int y) -> float
		{
			// Wrapping sampling, so the normal map tiles exactly as the height
			// field it came from does.
			const uint32_t wx = Wrap(x, width);
			const uint32_t wy = Wrap(y, height_);
			return height[(size_t)wy * width + wx];
		};

		for (uint32_t y = 0; y < height_; y++)
		{
			for (uint32_t x = 0; x < width; x++)
			{
				const int ix = (int)x, iy = (int)y;

				// Sobel: a 3x3 weighted difference, less sensitive to
				// single-texel noise than a plain central difference, which
				// matters when the height field is itself noise.
				const float dx =
					(at(ix + 1, iy - 1) + 2.0f * at(ix + 1, iy) + at(ix + 1, iy + 1)) -
					(at(ix - 1, iy - 1) + 2.0f * at(ix - 1, iy) + at(ix - 1, iy + 1));

				const float dy =
					(at(ix - 1, iy + 1) + 2.0f * at(ix, iy + 1) + at(ix + 1, iy + 1)) -
					(at(ix - 1, iy - 1) + 2.0f * at(ix, iy - 1) + at(ix + 1, iy - 1));

				glm::vec3 n = glm::normalize(glm::vec3(-dx * strength, -dy * strength, 1.0f));

				// Tangent-space normals are stored biased into [0, 1].
				normal.Set(x, y, glm::vec4(n * 0.5f + 0.5f, 1.0f));
			}
		}

		return normal.ToDataTexture();
	}

	namespace {

		// glTF convention, which PBR.glsl follows: roughness in G, metallic in
		// B. R is unused (occlusion, in the full spec) and A is ignored.
		Ref<Texture2D> PackMetallicRoughness(uint32_t size,
			const std::function<void(float, float, float&, float&)>& sample)
		{
			ImageBuffer image(size, size);
			image.Fill([&](float u, float v)
			{
				float roughness = 1.0f, metallic = 0.0f;
				sample(u, v, roughness, metallic);
				return glm::vec4(1.0f, roughness, metallic, 1.0f);
			});
			return image.ToDataTexture();
		}
	}

	MaterialTextures ProceduralTextures::Asphalt(uint32_t size, uint32_t seed)
	{
		const uint32_t period = 8;
		const float scale = (float)period;

		std::vector<float> heights((size_t)size * size);
		ImageBuffer albedo(size, size);

		for (uint32_t y = 0; y < size; y++)
		{
			const float v = (float)y / (float)size;
			for (uint32_t x = 0; x < size; x++)
			{
				const float u = (float)x / (float)size;

				// Two scales of noise: coarse patches where the surface has
				// worn differently, and fine grain for the aggregate itself.
				const float patches = Noise::FBM(u * scale, v * scale, period, seed, 4);
				const float grain = Noise::FBM(u * scale * 14.0f, v * scale * 14.0f,
					period * 14, seed + 31u, 3);

				// Individual stones: the top of the grain distribution, pushed
				// hard so only the brightest few per cent show as aggregate.
				const float aggregate = std::pow(Clamp01((grain - 0.55f) * 3.2f), 1.6f);

				// Weathered rather than fresh: new asphalt is nearly black, but
				// a circuit surface is bleached by sun and dusted with rubber
				// and grit, which lifts it to something a camera can see into.
				glm::vec3 base = Mix(glm::vec3(0.085f, 0.084f, 0.090f),
					glm::vec3(0.155f, 0.152f, 0.155f), patches);
				base = Mix(base, glm::vec3(0.245f, 0.238f, 0.232f), aggregate * 0.65f);

				heights[(size_t)y * size + x] = grain * 0.7f + aggregate * 0.3f;

				albedo.Set(x, y, glm::vec4(base, 1.0f));
			}
		}

		MaterialTextures result;
		result.Albedo = albedo.ToColorTexture();
		// Asphalt is coarse but it is not gravel: a strong normal map turns
		// every grain into a boulder once the texture is mapped to metres of
		// road rather than viewed flat.
		result.Normal = NormalFromHeight(heights, size, size, 0.85f);
		result.MetallicRoughness = PackMetallicRoughness(size, [&](float u, float v, float& r, float& m)
		{
			const float patches = Noise::FBM(u * scale, v * scale, period, seed, 4);
			// Asphalt is uniformly rough; the variation is what stops the
			// highlight from being a single flat sheet.
			r = 0.78f + patches * 0.18f;
			m = 0.0f;
		});

		return result;
	}

	MaterialTextures ProceduralTextures::Kerb(uint32_t size)
	{
		ImageBuffer albedo(size, size);
		std::vector<float> heights((size_t)size * size);

		for (uint32_t y = 0; y < size; y++)
		{
			const float v = (float)y / (float)size;
			for (uint32_t x = 0; x < size; x++)
			{
				const float u = (float)x / (float)size;

				// Two stripes per tile. The caller maps one tile to two metres
				// of kerb, which puts each stripe at the metre-ish width every
				// circuit paints them.
				const float stripe = std::fmod(v * 2.0f, 1.0f);
				const bool red = stripe < 0.5f;

				const float dirt = Noise::FBM(u * 6.0f, v * 6.0f, 6, 4242u, 4);

				// Deliberately dark for a "red": ACES lifts midtones hard, and
				// a nominally correct 0.5 red comes out of the tonemapper as
				// pink, which averages with the white into a uniform band.
				glm::vec3 color = red
					? glm::vec3(0.235f, 0.017f, 0.020f)
					: glm::vec3(0.70f, 0.69f, 0.67f);

				// Kerbs are the most abused surface on a circuit; grey them
				// down where the noise says they have been run over.
				color = Mix(color, color * 0.7f, dirt * 0.35f);

				heights[(size_t)y * size + x] = dirt * 0.35f;
				albedo.Set(x, y, glm::vec4(color, 1.0f));
			}
		}

		MaterialTextures result;
		result.Albedo = albedo.ToColorTexture();
		result.Normal = NormalFromHeight(heights, size, size, 1.2f);
		result.MetallicRoughness = PackMetallicRoughness(size, [](float u, float v, float& r, float& m)
		{
			r = 0.62f;
			m = 0.0f;
		});

		return result;
	}

	MaterialTextures ProceduralTextures::Grass(uint32_t size, uint32_t seed)
	{
		const uint32_t period = 8;
		const float scale = (float)period;

		ImageBuffer albedo(size, size);
		std::vector<float> heights((size_t)size * size);

		for (uint32_t y = 0; y < size; y++)
		{
			const float v = (float)y / (float)size;
			for (uint32_t x = 0; x < size; x++)
			{
				const float u = (float)x / (float)size;

				const float clumps = Noise::FBM(u * scale * 2.0f, v * scale * 2.0f, period * 2, seed, 4);
				const float blades = Noise::FBM(u * scale * 22.0f, v * scale * 22.0f,
					period * 22, seed + 13u, 2);

				glm::vec3 color = Mix(glm::vec3(0.055f, 0.115f, 0.038f),
					glm::vec3(0.135f, 0.215f, 0.075f), clumps);
				color = Mix(color, color * 1.25f, blades * 0.5f);

				heights[(size_t)y * size + x] = blades * 0.6f + clumps * 0.4f;
				albedo.Set(x, y, glm::vec4(color, 1.0f));
			}
		}

		MaterialTextures result;
		result.Albedo = albedo.ToColorTexture();
		// Gentler than the asphalt: grass has no hard facets, and a strong
		// normal map on a large tiled surface turns into visible blotching.
		result.Normal = NormalFromHeight(heights, size, size, 1.4f);
		result.MetallicRoughness = PackMetallicRoughness(size, [](float u, float v, float& r, float& m)
		{
			r = 0.92f;
			m = 0.0f;
		});

		return result;
	}

	MaterialTextures ProceduralTextures::Concrete(uint32_t size, uint32_t seed)
	{
		const uint32_t period = 8;
		const float scale = (float)period;

		ImageBuffer albedo(size, size);
		std::vector<float> heights((size_t)size * size);

		for (uint32_t y = 0; y < size; y++)
		{
			const float v = (float)y / (float)size;
			for (uint32_t x = 0; x < size; x++)
			{
				const float u = (float)x / (float)size;

				const float stain = Noise::FBM(u * scale, v * scale, period, seed, 5);
				const float pores = Noise::FBM(u * scale * 20.0f, v * scale * 20.0f,
					period * 20, seed + 5u, 2);

				glm::vec3 color = Mix(glm::vec3(0.44f, 0.435f, 0.42f),
					glm::vec3(0.62f, 0.615f, 0.60f), stain);

				// Air bubbles left in the pour: small, dark, sparse.
				const float bubble = Clamp01((pores - 0.72f) * 6.0f);
				color = Mix(color, color * 0.55f, bubble);

				heights[(size_t)y * size + x] = stain * 0.3f - bubble * 0.7f;
				albedo.Set(x, y, glm::vec4(color, 1.0f));
			}
		}

		MaterialTextures result;
		result.Albedo = albedo.ToColorTexture();
		result.Normal = NormalFromHeight(heights, size, size, 2.0f);
		result.MetallicRoughness = PackMetallicRoughness(size, [&](float u, float v, float& r, float& m)
		{
			r = 0.70f + Noise::FBM(u * scale, v * scale, period, seed, 3) * 0.2f;
			m = 0.0f;
		});

		return result;
	}

	MaterialTextures ProceduralTextures::CarPaint(const glm::vec3& color, uint32_t size)
	{
		ImageBuffer albedo(size, size);
		std::vector<float> heights((size_t)size * size);

		for (uint32_t y = 0; y < size; y++)
		{
			const float v = (float)y / (float)size;
			for (uint32_t x = 0; x < size; x++)
			{
				const float u = (float)x / (float)size;

				// Orange peel: the low-amplitude ripple a sprayed clear coat
				// always has. Invisible in the albedo, obvious in reflections,
				// which is exactly why it goes in the normal map.
				const float peel = Noise::FBM(u * 40.0f, v * 40.0f, 40, 8081u, 2);

				heights[(size_t)y * size + x] = peel;
				albedo.Set(x, y, glm::vec4(color, 1.0f));
			}
		}

		MaterialTextures result;
		result.Albedo = albedo.ToColorTexture();
		// Barely there on purpose. Orange peel is a ripple a fraction of a
		// millimetre deep across a panel half a metre wide; anything stronger
		// reads as a car upholstered in fabric.
		result.Normal = NormalFromHeight(heights, size, size, 0.05f);
		result.MetallicRoughness = PackMetallicRoughness(size, [](float u, float v, float& r, float& m)
		{
			// A clear coat over pigment: smooth, and dielectric rather than
			// metallic. Metallic paint would tint its reflections with the
			// body colour, which is not what a road car does.
			r = 0.14f;
			m = 0.0f;
		});

		return result;
	}

	MaterialTextures ProceduralTextures::RoadPaint(uint32_t size)
	{
		ImageBuffer albedo(size, size);
		std::vector<float> heights((size_t)size * size);

		for (uint32_t y = 0; y < size; y++)
		{
			const float v = (float)y / (float)size;
			for (uint32_t x = 0; x < size; x++)
			{
				const float u = (float)x / (float)size;

				const float wear = Noise::FBM(u * 9.0f, v * 9.0f, 9, 5150u, 4);
				const float grit = Noise::FBM(u * 30.0f, v * 30.0f, 30, 5151u, 2);

				// Worn towards the grey of the road underneath rather than
				// towards black: paint wears off, it does not darken.
				glm::vec3 color = Mix(glm::vec3(0.66f, 0.655f, 0.635f),
					glm::vec3(0.30f, 0.295f, 0.29f), std::pow(Clamp01(wear), 2.2f) * 0.55f);

				heights[(size_t)y * size + x] = grit;
				albedo.Set(x, y, glm::vec4(color, 1.0f));
			}
		}

		MaterialTextures result;
		result.Albedo = albedo.ToColorTexture();
		result.Normal = NormalFromHeight(heights, size, size, 0.5f);
		result.MetallicRoughness = PackMetallicRoughness(size, [](float u, float v, float& r, float& m)
		{
			// Thermoplastic road paint has a slight sheen, unlike the asphalt.
			r = 0.55f;
			m = 0.0f;
		});

		return result;
	}

	MaterialTextures ProceduralTextures::Tyre(uint32_t size)
	{
		ImageBuffer albedo(size, size);
		std::vector<float> heights((size_t)size * size);

		for (uint32_t y = 0; y < size; y++)
		{
			const float v = (float)y / (float)size;
			for (uint32_t x = 0; x < size; x++)
			{
				const float u = (float)x / (float)size;

				// v runs across the tyre: the middle is tread, the edges are
				// sidewall.
				const float acrossCentre = std::abs(v - 0.5f) * 2.0f;
				const bool tread = acrossCentre < 0.62f;

				const float grooves = tread
					? (std::sin(u * 3.14159265f * 28.0f + v * 6.0f) * 0.5f + 0.5f)
					: 0.35f;

				const float grain = Noise::FBM(u * 24.0f, v * 24.0f, 24, 606u, 3);

				glm::vec3 color = glm::vec3(0.022f, 0.022f, 0.024f);
				color = Mix(color, glm::vec3(0.045f), grain * 0.6f);

				heights[(size_t)y * size + x] = tread ? grooves : grain * 0.4f + 0.5f;
				albedo.Set(x, y, glm::vec4(color, 1.0f));
			}
		}

		MaterialTextures result;
		result.Albedo = albedo.ToColorTexture();
		result.Normal = NormalFromHeight(heights, size, size, 3.5f);
		result.MetallicRoughness = PackMetallicRoughness(size, [](float u, float v, float& r, float& m)
		{
			r = 0.88f;
			m = 0.0f;
		});

		return result;
	}
}
