#include "RacingScene.h"

#include <algorithm>
#include <cmath>

namespace Racing {

	using Gymon::Mesh;
	using Gymon::Vertex;

	namespace {

		constexpr float kPi = 3.14159265358979f;

		// Accumulates vertices and indices for a mesh being built piece by
		// piece. Every helper below appends into one of these rather than
		// returning its own array, so a car body and its wing end up as one
		// mesh with one draw call.
		struct MeshBuilder
		{
			std::vector<Vertex> Vertices;
			std::vector<uint32_t> Indices;

			uint32_t Add(const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv)
			{
				Vertices.push_back({ position, normal, uv });
				return (uint32_t)Vertices.size() - 1;
			}

			void Triangle(uint32_t a, uint32_t b, uint32_t c)
			{
				Indices.insert(Indices.end(), { a, b, c });
			}

			void Quad(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
			{
				Triangle(a, b, c);
				Triangle(a, c, d);
			}

			Gymon::Ref<Mesh> Build() const
			{
				return Gymon::CreateRef<Mesh>(Vertices, Indices);
			}

			bool Empty() const { return Indices.empty(); }
		};

		// An axis-aligned box, given its centre and half extents. Each face
		// gets its own four vertices so the box has hard edges.
		void AddBox(MeshBuilder& out, const glm::vec3& centre, const glm::vec3& halfExtents,
			float uvScale = 1.0f)
		{
			const glm::vec3 lo = centre - halfExtents;
			const glm::vec3 hi = centre + halfExtents;

			struct Face { glm::vec3 normal; glm::vec3 corners[4]; };
			const Face faces[6] = {
				{ {  0,  0,  1 }, { { lo.x, lo.y, hi.z }, { hi.x, lo.y, hi.z }, { hi.x, hi.y, hi.z }, { lo.x, hi.y, hi.z } } },
				{ {  0,  0, -1 }, { { hi.x, lo.y, lo.z }, { lo.x, lo.y, lo.z }, { lo.x, hi.y, lo.z }, { hi.x, hi.y, lo.z } } },
				{ { -1,  0,  0 }, { { lo.x, lo.y, lo.z }, { lo.x, lo.y, hi.z }, { lo.x, hi.y, hi.z }, { lo.x, hi.y, lo.z } } },
				{ {  1,  0,  0 }, { { hi.x, lo.y, hi.z }, { hi.x, lo.y, lo.z }, { hi.x, hi.y, lo.z }, { hi.x, hi.y, hi.z } } },
				{ {  0,  1,  0 }, { { lo.x, hi.y, hi.z }, { hi.x, hi.y, hi.z }, { hi.x, hi.y, lo.z }, { lo.x, hi.y, lo.z } } },
				{ {  0, -1,  0 }, { { lo.x, lo.y, lo.z }, { hi.x, lo.y, lo.z }, { hi.x, lo.y, hi.z }, { lo.x, lo.y, hi.z } } }
			};

			for (const Face& face : faces)
			{
				const uint32_t a = out.Add(face.corners[0], face.normal, { 0.0f, 0.0f });
				const uint32_t b = out.Add(face.corners[1], face.normal, { uvScale, 0.0f });
				const uint32_t c = out.Add(face.corners[2], face.normal, { uvScale, uvScale });
				const uint32_t d = out.Add(face.corners[3], face.normal, { 0.0f, uvScale });
				out.Quad(a, b, c, d);
			}
		}

		// A cylinder lying along the Z axis, which is how a wheel sits on a
		// car whose length runs down X.
		void AddWheel(MeshBuilder& out, const glm::vec3& centre, float radius, float halfWidth,
			uint32_t segments = 28)
		{
			const uint32_t ringStart = (uint32_t)out.Vertices.size();

			for (uint32_t i = 0; i <= segments; i++)
			{
				const float t = (float)i / (float)segments;
				const float angle = t * 2.0f * kPi;
				const float x = std::cos(angle), y = std::sin(angle);
				const glm::vec3 normal(x, y, 0.0f);

				out.Add(centre + glm::vec3(x * radius, y * radius, -halfWidth), normal, { t * 4.0f, 0.0f });
				out.Add(centre + glm::vec3(x * radius, y * radius,  halfWidth), normal, { t * 4.0f, 1.0f });
			}

			for (uint32_t i = 0; i < segments; i++)
			{
				const uint32_t base = ringStart + i * 2;
				out.Quad(base, base + 2, base + 3, base + 1);
			}

			// Wheel faces, flat discs at each side. Real wheels have spokes;
			// at the size these are drawn, a disc with a rim ring reads the
			// same and costs a twentieth of the triangles.
			for (int side = 0; side < 2; side++)
			{
				const float z = side == 0 ? halfWidth : -halfWidth;
				const glm::vec3 normal(0.0f, 0.0f, side == 0 ? 1.0f : -1.0f);

				const uint32_t hub = out.Add(centre + glm::vec3(0.0f, 0.0f, z), normal, { 0.5f, 0.5f });
				const uint32_t rimStart = (uint32_t)out.Vertices.size();

				for (uint32_t i = 0; i <= segments; i++)
				{
					const float angle = (float)i / (float)segments * 2.0f * kPi;
					const float x = std::cos(angle), y = std::sin(angle);
					out.Add(centre + glm::vec3(x * radius, y * radius, z), normal,
						{ x * 0.5f + 0.5f, y * 0.5f + 0.5f });
				}

				for (uint32_t i = 0; i < segments; i++)
				{
					if (side == 0)
						out.Triangle(hub, rimStart + i, rimStart + i + 1);
					else
						out.Triangle(hub, rimStart + i + 1, rimStart + i);
				}
			}
		}
	}

	Spline::Spline(std::vector<glm::vec3> controlPoints)
		: m_Points(std::move(controlPoints))
	{
		GY_ASSERT(m_Points.size() >= 4, "A Catmull-Rom loop needs at least four control points");
	}

	const glm::vec3& Spline::At(int index) const
	{
		const int count = (int)m_Points.size();
		return m_Points[((index % count) + count) % count];
	}

	glm::vec3 Spline::Position(float t) const
	{
		const int count = (int)m_Points.size();

		// t is normalised over the whole loop; scale it into "which segment,
		// and how far along that segment".
		const float scaled = t * (float)count;
		const int segment = (int)std::floor(scaled);
		const float local = scaled - (float)segment;

		const glm::vec3& p0 = At(segment - 1);
		const glm::vec3& p1 = At(segment);
		const glm::vec3& p2 = At(segment + 1);
		const glm::vec3& p3 = At(segment + 2);

		// Uniform Catmull-Rom. Non-uniform (centripetal) parameterisation
		// would avoid cusps on very unevenly spaced points; the circuit's
		// control points are spaced deliberately, so uniform is enough.
		const float t2 = local * local;
		const float t3 = t2 * local;

		return 0.5f * ((2.0f * p1) +
			(-p0 + p2) * local +
			(2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
			(-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
	}

	glm::vec3 Spline::Tangent(float t) const
	{
		// Central difference rather than the analytic derivative: it is a few
		// lines shorter, and at the sample counts used here the difference is
		// far below a pixel.
		const float h = 1e-4f;
		return glm::normalize(Position(t + h) - Position(t - h));
	}

	float Spline::Curvature(float t) const
	{
		const float h = 1.5e-3f;

		const glm::vec3 before = Tangent(t - h);
		const glm::vec3 after = Tangent(t + h);

		// Signed by which side of the tangent the turn goes, taken about the
		// world up axis. A right-hander banks the opposite way to a left.
		const glm::vec3 change = after - before;
		const glm::vec3 right = glm::normalize(glm::cross(Tangent(t), glm::vec3(0.0f, 1.0f, 0.0f)));

		return glm::dot(change, right) / (2.0f * h) * 0.01f;
	}

	Spline DefaultCircuit()
	{
		// A layout with a bit of everything, so the demo shows a long straight
		// (where the road texture is seen at a shallow angle), tight radius
		// (where banking and kerbs read), and fast sweepers.
		return Spline({
			{  -70.0f, 0.0f,  -95.0f },  // start-finish straight, southern end
			{   35.0f, 0.0f,  -95.0f },
			{   85.0f, 0.0f,  -78.0f },  // turn 1, fast right
			{  100.0f, 0.0f,  -30.0f },
			{   82.0f, 0.0f,   12.0f },
			{   38.0f, 0.0f,   22.0f },  // into the hairpin
			{   18.0f, 0.0f,   48.0f },
			{   44.0f, 0.0f,   74.0f },  // hairpin apex
			{   82.0f, 0.0f,   66.0f },
			{   92.0f, 0.0f,  100.0f },
			{   48.0f, 0.0f,  122.0f },  // chicane
			{    2.0f, 0.0f,  104.0f },
			{  -36.0f, 0.0f,  120.0f },
			{  -80.0f, 0.0f,   98.0f },  // long left sweeper
			{ -104.0f, 0.0f,   50.0f },
			{  -96.0f, 0.0f,   -6.0f },
			{ -112.0f, 0.0f,  -52.0f },
			{  -98.0f, 0.0f,  -88.0f }
		});
	}

	TrackMeshes BuildTrack(const Spline& spline, uint32_t samples, float roadHalfWidth)
	{
		MeshBuilder road, kerbs, verge, barriers, markings;

		const float kerbWidth = 1.1f;
		const float kerbRise = 0.09f;
		const float vergeWidth = 14.0f;
		const float barrierHeight = 1.05f;
		const float barrierOffset = vergeWidth * 0.82f;

		// Metres of track per texture repeat. Chosen so the aggregate in the
		// asphalt reads at roughly life size from a driving eye height.
		const float roadTileLength = 6.0f;

		// One extra sample that duplicates the first, so the last quad closes
		// the loop without special-casing the wrap.
		const uint32_t ringCount = samples + 1;

		struct Ring
		{
			glm::vec3 Position;
			glm::vec3 Right;
			glm::vec3 Up;
			float Distance;
		};

		std::vector<Ring> rings;
		rings.reserve(ringCount);

		float distance = 0.0f;
		glm::vec3 previous = spline.Position(0.0f);

		for (uint32_t i = 0; i < ringCount; i++)
		{
			const float t = (float)i / (float)samples;

			Ring ring;
			ring.Position = spline.Position(t);

			distance += glm::length(ring.Position - previous);
			previous = ring.Position;
			ring.Distance = distance;

			const glm::vec3 tangent = spline.Tangent(t);
			const glm::vec3 flatRight = glm::normalize(glm::cross(tangent, glm::vec3(0.0f, 1.0f, 0.0f)));

			// Bank into the corner, capped: past about twelve degrees a road
			// circuit stops looking like a road circuit.
			const float bank = std::clamp(spline.Curvature(t) * 26.0f, -0.21f, 0.21f);

			ring.Right = glm::normalize(flatRight * std::cos(bank) + glm::vec3(0.0f, 1.0f, 0.0f) * std::sin(bank));
			ring.Up = glm::normalize(glm::cross(ring.Right, tangent));

			rings.push_back(ring);
		}

		// A strip of quads between consecutive rings, at two offsets from the
		// centre line. Every surface of the track is one of these.
		auto addStrip = [&](MeshBuilder& out, float leftOffset, float rightOffset,
			float leftLift, float rightLift, float uvWidth, float tileLength)
		{
			for (uint32_t i = 0; i + 1 < ringCount; i++)
			{
				const Ring& a = rings[i];
				const Ring& b = rings[i + 1];

				const glm::vec3 a0 = a.Position + a.Right * leftOffset + a.Up * leftLift;
				const glm::vec3 a1 = a.Position + a.Right * rightOffset + a.Up * rightLift;
				const glm::vec3 b0 = b.Position + b.Right * leftOffset + b.Up * leftLift;
				const glm::vec3 b1 = b.Position + b.Right * rightOffset + b.Up * rightLift;

				// Face normal from the quad itself, so a banked or lifted
				// strip is shaded correctly without tracking normals through
				// the offsets.
				glm::vec3 normal = glm::cross(b0 - a0, a1 - a0);
				const float length = glm::length(normal);
				normal = length > 1e-6f ? normal / length : glm::vec3(0.0f, 1.0f, 0.0f);
				if (normal.y < 0.0f)
					normal = -normal;

				const float va = a.Distance / tileLength;
				const float vb = b.Distance / tileLength;

				const uint32_t i0 = out.Add(a0, normal, { 0.0f, va });
				const uint32_t i1 = out.Add(a1, normal, { uvWidth, va });
				const uint32_t i2 = out.Add(b1, normal, { uvWidth, vb });
				const uint32_t i3 = out.Add(b0, normal, { 0.0f, vb });

				out.Quad(i0, i1, i2, i3);
			}
		};

		// Road surface.
		addStrip(road, -roadHalfWidth, roadHalfWidth, 0.02f, 0.02f,
			roadHalfWidth * 2.0f / roadTileLength, roadTileLength);

		// Kerbs, raised slightly and sloping up away from the racing line.
		addStrip(kerbs, -roadHalfWidth - kerbWidth, -roadHalfWidth, kerbRise, 0.02f, 1.0f, 2.0f);
		addStrip(kerbs, roadHalfWidth, roadHalfWidth + kerbWidth, 0.02f, kerbRise, 1.0f, 2.0f);

		// Grass run-off, dropped below the kerb so the track sits proud of it.
		addStrip(verge, -barrierOffset, -roadHalfWidth - kerbWidth, -0.35f, kerbRise, 6.0f, 10.0f);
		addStrip(verge, roadHalfWidth + kerbWidth, barrierOffset, kerbRise, -0.35f, 6.0f, 10.0f);

		// Painted edge lines, set in from the kerb the way a circuit paints
		// them, and lifted a few millimetres so they never z-fight the road.
		const float lineInset = 0.28f;
		const float lineWidth = 0.16f;
		const float paintLift = 0.035f;

		addStrip(markings, -roadHalfWidth + lineInset, -roadHalfWidth + lineInset + lineWidth,
			paintLift, paintLift, 1.0f, 3.0f);
		addStrip(markings, roadHalfWidth - lineInset - lineWidth, roadHalfWidth - lineInset,
			paintLift, paintLift, 1.0f, 3.0f);

		// Start/finish line: a band across the full width, placed far enough
		// along the straight that a car sitting on the grid is behind it.
		{
			const uint32_t lineStart = ringCount / 20;
			const uint32_t lineEnd = lineStart + 1;

			for (uint32_t i = lineStart; i < lineEnd && i + 1 < ringCount; i++)
			{
				const Ring& a = rings[i];
				const Ring& b = rings[i + 1];

				const glm::vec3 a0 = a.Position - a.Right * roadHalfWidth + a.Up * paintLift;
				const glm::vec3 a1 = a.Position + a.Right * roadHalfWidth + a.Up * paintLift;
				const glm::vec3 b0 = b.Position - b.Right * roadHalfWidth + b.Up * paintLift;
				const glm::vec3 b1 = b.Position + b.Right * roadHalfWidth + b.Up * paintLift;

				const uint32_t i0 = markings.Add(a0, a.Up, { 0.0f, 0.0f });
				const uint32_t i1 = markings.Add(a1, a.Up, { 1.0f, 0.0f });
				const uint32_t i2 = markings.Add(b1, b.Up, { 1.0f, 1.0f });
				const uint32_t i3 = markings.Add(b0, b.Up, { 0.0f, 1.0f });

				markings.Quad(i0, i1, i2, i3);
			}
		}

		// Barrier walls, as vertical strips at the edge of the run-off.
		for (int side = 0; side < 2; side++)
		{
			const float offset = side == 0 ? -barrierOffset : barrierOffset;

			for (uint32_t i = 0; i + 1 < ringCount; i++)
			{
				const Ring& a = rings[i];
				const Ring& b = rings[i + 1];

				const glm::vec3 base0 = a.Position + a.Right * offset - a.Up * 0.35f;
				const glm::vec3 base1 = b.Position + b.Right * offset - b.Up * 0.35f;
				const glm::vec3 top0 = base0 + glm::vec3(0.0f, barrierHeight, 0.0f);
				const glm::vec3 top1 = base1 + glm::vec3(0.0f, barrierHeight, 0.0f);

				// Facing the track, whichever side this is.
				glm::vec3 normal = a.Right * (side == 0 ? 1.0f : -1.0f);

				const float va = a.Distance / 6.0f;
				const float vb = b.Distance / 6.0f;

				const uint32_t i0 = barriers.Add(base0, normal, { 0.0f, va });
				const uint32_t i1 = barriers.Add(base1, normal, { 0.0f, vb });
				const uint32_t i2 = barriers.Add(top1, normal, { 1.0f, vb });
				const uint32_t i3 = barriers.Add(top0, normal, { 1.0f, va });

				if (side == 0)
					barriers.Quad(i0, i1, i2, i3);
				else
					barriers.Quad(i3, i2, i1, i0);
			}
		}

		TrackMeshes result;
		result.Road = road.Build();
		result.Kerbs = kerbs.Build();
		result.Verge = verge.Build();
		result.Barriers = barriers.Build();
		result.Markings = markings.Build();

		// Start on the straight, a little to the inside, facing the way the
		// spline runs.
		const glm::vec3 startTangent = spline.Tangent(0.02f);
		result.StartPosition = spline.Position(0.02f) + glm::vec3(0.0f, 0.02f, 0.0f);
		result.StartYawDegrees = glm::degrees(std::atan2(startTangent.x, startTangent.z));

		return result;
	}

	CarMeshes BuildCar()
	{
		MeshBuilder body, wheels, glass;

		// Overall dimensions of a GT-class car, in metres. The origin is on
		// the ground at the centre of the wheelbase, which is the sane place
		// to put it for anything that later drives the car around.
		const float length = 4.55f;
		const float halfLength = length * 0.5f;
		const float wheelRadius = 0.35f;
		const float wheelHalfWidth = 0.15f;

		// The body is lofted: a series of cross-sections down the length,
		// connected into a hull. Boxes stacked into a car shape read as boxes
		// stacked into a car shape; a swept profile reads as a car.
		struct Section { float t; float halfWidth; float centreY; float halfHeight; };
		const Section sections[] = {
			{ 0.00f, 0.50f, 0.44f, 0.13f },  // nose
			{ 0.08f, 0.70f, 0.46f, 0.17f },
			{ 0.20f, 0.78f, 0.50f, 0.21f },  // over the front axle
			{ 0.34f, 0.84f, 0.60f, 0.32f },  // base of the windscreen
			{ 0.46f, 0.86f, 0.72f, 0.43f },
			{ 0.56f, 0.86f, 0.76f, 0.47f },  // roof
			{ 0.68f, 0.85f, 0.69f, 0.40f },
			{ 0.80f, 0.80f, 0.58f, 0.29f },  // over the rear axle
			{ 0.92f, 0.74f, 0.52f, 0.22f },
			{ 1.00f, 0.58f, 0.50f, 0.16f }   // tail
		};

		const int sectionCount = (int)(sizeof(sections) / sizeof(sections[0]));
		const int ringResolution = 24;

		// Superellipse cross-section: a rounded rectangle, which is what a car
		// body actually is in section. An ellipse would be too soft and a
		// rectangle too hard.
		auto sectionPoint = [](const Section& section, float angle)
		{
			const float c = std::cos(angle), s = std::sin(angle);
			const float exponent = 2.0f / 3.4f;

			const float x = std::copysign(std::pow(std::abs(c), exponent), c) * section.halfWidth;
			const float y = std::copysign(std::pow(std::abs(s), exponent), s) * section.halfHeight;

			// Flatten the underside: a car has a floor, not a rounded belly.
			return glm::vec2(x, section.centreY + (y < 0.0f ? y * 0.55f : y));
		};

		// Positions first, normals second.
		//
		// An earlier version guessed each normal from the direction out of the
		// section's centre, which ignores how fast the hull is widening or
		// rising along its length. A wrong normal on a smooth, shiny surface
		// is not subtle: it puts a dark band down the side of the car where
		// the shading disagrees with the shape.
		std::vector<std::vector<glm::vec3>> positions(sectionCount);

		for (int s = 0; s < sectionCount; s++)
		{
			const Section& section = sections[s];
			const float x = -halfLength + section.t * length;

			positions[s].reserve(ringResolution);
			for (int i = 0; i < ringResolution; i++)
			{
				const float angle = (float)i / (float)ringResolution * 2.0f * kPi;
				const glm::vec2 point = sectionPoint(section, angle);
				positions[s].push_back(glm::vec3(x, point.y, point.x));
			}
		}

		// Central differences across the position grid: the cross product of
		// the two surface tangents is the true normal of the lofted surface.
		auto surfaceNormal = [&](int s, int i)
		{
			const int prevSection = s > 0 ? s - 1 : s;
			const int nextSection = s < sectionCount - 1 ? s + 1 : s;
			const int prevRing = (i - 1 + ringResolution) % ringResolution;
			const int nextRing = (i + 1) % ringResolution;

			const glm::vec3 alongLength = positions[nextSection][i] - positions[prevSection][i];
			const glm::vec3 aroundRing = positions[s][nextRing] - positions[s][prevRing];

			glm::vec3 normal = glm::cross(aroundRing, alongLength);
			const float lengthSquared = glm::dot(normal, normal);
			if (lengthSquared < 1e-12f)
				return glm::vec3(0.0f, 1.0f, 0.0f);

			normal = normal / std::sqrt(lengthSquared);

			// Point outwards, away from the section's centre line.
			const glm::vec3 centre(positions[s][i].x, sections[s].centreY, 0.0f);
			return glm::dot(normal, positions[s][i] - centre) < 0.0f ? -normal : normal;
		};

		std::vector<std::vector<uint32_t>> ringIndices(sectionCount);

		for (int s = 0; s < sectionCount; s++)
		{
			ringIndices[s].reserve(ringResolution);
			for (int i = 0; i < ringResolution; i++)
			{
				ringIndices[s].push_back(body.Add(positions[s][i], surfaceNormal(s, i),
					{ (float)i / (float)ringResolution, sections[s].t * 2.0f }));
			}
		}

		for (int s = 0; s + 1 < sectionCount; s++)
		{
			for (int i = 0; i < ringResolution; i++)
			{
				const int next = (i + 1) % ringResolution;
				body.Quad(
					ringIndices[s][i], ringIndices[s][next],
					ringIndices[s + 1][next], ringIndices[s + 1][i]);
			}
		}

		// Cap the nose and tail with fans, so the hull is closed.
		for (int end = 0; end < 2; end++)
		{
			const int s = end == 0 ? 0 : sectionCount - 1;
			const Section& section = sections[s];
			const float x = -halfLength + section.t * length;
			const glm::vec3 normal(end == 0 ? -1.0f : 1.0f, 0.0f, 0.0f);

			const uint32_t centre = body.Add({ x, section.centreY, 0.0f }, normal, { 0.5f, 0.5f });

			// Its own ring of vertices carrying the cap's flat normal: sharing
			// the hull's smooth normals here would round the nose off into the
			// bodywork instead of leaving a defined edge.
			//
			// The UVs are a radial projection of the cap rather than a
			// constant. A face whose vertices all share one UV has a zero UV
			// derivative, and any shader deriving a tangent basis from screen
			// -space derivatives then normalises a zero vector.
			std::vector<uint32_t> rim;
			rim.reserve(ringResolution);
			for (int i = 0; i < ringResolution; i++)
			{
				const float angle = (float)i / (float)ringResolution * 2.0f * kPi;
				rim.push_back(body.Add(positions[s][i], normal,
					{ std::cos(angle) * 0.5f + 0.5f, std::sin(angle) * 0.5f + 0.5f }));
			}

			for (int i = 0; i < ringResolution; i++)
			{
				const int next = (i + 1) % ringResolution;
				if (end == 0)
					body.Triangle(centre, rim[next], rim[i]);
				else
					body.Triangle(centre, rim[i], rim[next]);
			}
		}

		// The greenhouse: a shell just inside the hull over the cabin
		// sections. Its own mesh so it can be dark, smooth glass rather than
		// paint, which is what makes the shape read as a car at a glance.
		{
			const int glassFirst = 3;
			const int glassLast = 6;
			// Just proud of the hull rather than inside it: the bodywork is a
			// closed surface, so a canopy tucked within it would never be
			// seen. A few millimetres is enough to win the depth test without
			// leaving a visible gap at the edges.
			const float inset = 1.008f;

			// Only the upper part of each ring is glazed.
			const int glassStart = 2;
			const int glassEnd = ringResolution / 2 - 2;

			std::vector<std::vector<uint32_t>> glassIndices(glassLast - glassFirst + 1);

			for (int s = glassFirst; s <= glassLast; s++)
			{
				for (int i = glassStart; i <= glassEnd; i++)
				{
					const glm::vec3 hull = positions[s][i];
					const glm::vec3 centre(hull.x, sections[s].centreY, 0.0f);
					const glm::vec3 position = centre + (hull - centre) * inset;

					glassIndices[s - glassFirst].push_back(
						glass.Add(position, surfaceNormal(s, i),
							{ (float)i / (float)ringResolution, sections[s].t }));
				}
			}

			for (int s = 0; s + 1 < (int)glassIndices.size(); s++)
			{
				for (size_t i = 0; i + 1 < glassIndices[s].size(); i++)
				{
					glass.Quad(
						glassIndices[s][i], glassIndices[s][i + 1],
						glassIndices[s + 1][i + 1], glassIndices[s + 1][i]);
				}
			}
		}

		// Rear wing: two end plates and the aerofoil between them.
		const float wingX = halfLength - 0.28f;
		AddBox(body, { wingX, 1.06f, 0.0f }, { 0.16f, 0.025f, 0.82f });
		AddBox(body, { wingX, 0.90f, 0.80f }, { 0.22f, 0.19f, 0.02f });
		AddBox(body, { wingX, 0.90f, -0.80f }, { 0.22f, 0.19f, 0.02f });

		// Front splitter.
		AddBox(body, { -halfLength + 0.10f, 0.20f, 0.0f }, { 0.16f, 0.02f, 0.90f });

		// Side sills, which give the flanks a line and close the gap between
		// the floor and the road.
		AddBox(body, { 0.10f, 0.26f, 0.86f }, { 1.30f, 0.06f, 0.06f });
		AddBox(body, { 0.10f, 0.26f, -0.86f }, { 1.30f, 0.06f, 0.06f });

		// Wing mirrors.
		AddBox(body, { -0.30f, 0.80f, 0.92f }, { 0.09f, 0.05f, 0.10f });
		AddBox(body, { -0.30f, 0.80f, -0.92f }, { 0.09f, 0.05f, 0.10f });

		// Wheels, set wider than the body is at axle height so they sit proud
		// of the arches the way they do on anything with a wide track.
		const float axleFront = -halfLength + 1.00f;
		const float axleRear = halfLength - 1.05f;
		const float track = 0.95f;

		AddWheel(wheels, { axleFront, wheelRadius,  track }, wheelRadius, wheelHalfWidth);
		AddWheel(wheels, { axleFront, wheelRadius, -track }, wheelRadius, wheelHalfWidth);
		AddWheel(wheels, { axleRear,  wheelRadius,  track }, wheelRadius, wheelHalfWidth);
		AddWheel(wheels, { axleRear,  wheelRadius, -track }, wheelRadius, wheelHalfWidth);

		CarMeshes result;
		result.Body = body.Build();
		result.Wheels = wheels.Build();
		result.Glass = glass.Build();
		return result;
	}
}
