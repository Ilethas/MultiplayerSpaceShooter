#ifndef CIRCLE_COLLIDER_H_
#define CIRCLE_COLLIDER_H_
#include "Collider.h"
#include "Actor.h"


class CircleCollider : public Collider
{
	CLONEABLE_COMPONENT();

	public:
		double radius = 0;

		bool collisionTest(const Collider &other) override;
		void draw(sf::RenderWindow &window) const override;
};


#endif