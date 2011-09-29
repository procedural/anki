#ifndef SPHERE_H
#define SPHERE_H

#include "CollisionShape.h"
#include "m/Math.h"


class Plane;


/// @addtogroup Collision
/// @{

/// Sphere collision shape
class Sphere: public CollisionShape
{
	public:
		/// @name Constructors
		/// @{

		/// Default constructor
		Sphere()
		:	CollisionShape(CST_SPHERE)
		{}

		/// Copy constructor
		Sphere(const Sphere& other);

		/// Constructor
		Sphere(const Vec3& center_, float radius_);
		/// @}

		/// @name Accessors
		/// @{
		const Vec3& getCenter() const
		{
			return center;
		}
		Vec3& getCenter()
		{
			return center;
		}
		void setCenter(const Vec3& x)
		{
			center = x;
		}

		float getRadius() const
		{
			return radius;
		}
		float& getRadius()
		{
			return radius;
		}
		void setRadius(const float x)
		{
			radius = x;
		}
		/// @}

		Sphere getTransformed(const Transform& transform) const;

		/// Get the sphere that includes this sphere and the given. See a
		/// drawing in the docs dir for more info about the algorithm
		Sphere getCompoundShape(const Sphere& b) const;

		/// @see CollisionShape::testPlane
		float testPlane(const Plane& plane) const;

		/// Calculate from a set of points
		template<typename Container>
		void set(const Container& container);

	private:
		Vec3 center;
		float radius;
};
/// @}


inline Sphere::Sphere(const Sphere& b)
:	CollisionShape(CST_SPHERE),
	center(b.center),
	radius(b.radius)
{}


inline Sphere::Sphere(const Vec3& center_, float radius_)
:	CollisionShape(CST_SPHERE),
	center(center_),
	radius(radius_)
{}


#include "Sphere.inl.h"


#endif