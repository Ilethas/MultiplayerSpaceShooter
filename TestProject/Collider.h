#ifndef COLLIDER_H_
#define COLLIDER_H_
#include <SFML/Graphics.hpp>
#include "Component.h"


class Collider : public Component
{
	public:
		sf::Vector2f relativePosition;

		virtual ~Collider() = 0 {}
		virtual bool collisionTest(const Collider &other) = 0;
		virtual void draw(sf::RenderWindow &window) const {}
};


#endif