#pragma once

#include <Gymon.h>

#include <vector>

// Procedural content for the racing demo.
//
// Everything here is generated rather than authored: a circuit modelled in a
// DCC tool would be a large binary asset in a repository that is otherwise
// source, and a track defined by a spline is far easier to change than one
// defined by fifty thousand triangles.
namespace Racing {

	// A closed Catmull-Rom spline through a fixed set of control points, with
	// the frame (position, tangent, right, up) needed to loft geometry along
	// it. Catmull-Rom because it passes through its control points, so moving
	// a corner moves the track to exactly where you put it.
	class Spline
	{
	public:
		explicit Spline(std::vector<glm::vec3> controlPoints);

		// t runs 0..1 around the whole loop.
		glm::vec3 Position(float t) const;
		glm::vec3 Tangent(float t) const;

		// Approximate curvature, used to bank the track into corners. Signed:
		// positive turns one way, negative the other.
		float Curvature(float t) const;

		size_t ControlPointCount() const { return m_Points.size(); }

	private:
		// Wraps the index, which is what makes the spline closed.
		const glm::vec3& At(int index) const;

		std::vector<glm::vec3> m_Points;
	};

	// The meshes a circuit is made of. Separate meshes rather than one, so
	// each can carry the material it actually needs -- asphalt, painted kerb,
	// grass -- instead of all three being approximated by one texture.
	struct TrackMeshes
	{
		Gymon::Ref<Gymon::Mesh> Road;
		Gymon::Ref<Gymon::Mesh> Kerbs;
		Gymon::Ref<Gymon::Mesh> Verge;
		Gymon::Ref<Gymon::Mesh> Barriers;
		// Painted lines: the white edge on each side and the start/finish
		// band. Their own mesh because paint is a different material from the
		// asphalt under it, not a different colour of it.
		Gymon::Ref<Gymon::Mesh> Markings;

		// Where the car should start, and which way it should face.
		glm::vec3 StartPosition{ 0.0f };
		float StartYawDegrees = 0.0f;
	};

	// Lofts the circuit. `samples` sets how finely the spline is walked; too
	// few and the corners visibly facet, too many and the mesh is wasteful.
	TrackMeshes BuildTrack(const Spline& spline, uint32_t samples = 600,
		float roadHalfWidth = 5.0f);

	// The default circuit layout: a start-finish straight, a fast right, a
	// hairpin, a chicane and two sweepers back onto the straight.
	Spline DefaultCircuit();

	// A car, in two meshes: the painted body (with its wing and mirrors) and
	// the four wheels merged into one mesh so the rubber can be its own
	// material. Both are in car-local space with the origin on the ground
	// between the front wheels' axis and the rear.
	struct CarMeshes
	{
		Gymon::Ref<Gymon::Mesh> Body;
		Gymon::Ref<Gymon::Mesh> Wheels;
		// The greenhouse. Separate because glass is dark and smooth where
		// paint is coloured and matte, and because a car without windows
		// reads as a lozenge however good its silhouette is.
		Gymon::Ref<Gymon::Mesh> Glass;
	};

	CarMeshes BuildCar();
}
