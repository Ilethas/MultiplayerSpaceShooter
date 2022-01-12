#include "CircleCollider.h"


bool CircleCollider::collisionTest(const Collider &other)
{
	auto ownerActorShared = getOwnerActor().lock();
	auto otherOwnerActorShared = other.getOwnerActor().lock();

	if (!ownerActorShared || !otherOwnerActorShared)
		return false;

	const CircleCollider *coll = dynamic_cast<const CircleCollider*>(&other);
	if (coll)
	{
		sf::Vector2f posSelf = Tools::ScaleVector(
			Tools::rotate(
				relativePosition,
				ownerActorShared->getGlobalRotation()),
			ownerActorShared->getGlobalScale()) + ownerActorShared->getGlobalPosition();

		sf::Vector2f posOther = Tools::ScaleVector(
			Tools::rotate(
				other.relativePosition,
				otherOwnerActorShared->getGlobalRotation()),
			otherOwnerActorShared->getGlobalScale()) + otherOwnerActorShared->getGlobalPosition();

		float xDiff = posOther.x - posSelf.x;
		float yDiff = posOther.y - posSelf.y;
		float distance = std::sqrt(xDiff*xDiff + yDiff*yDiff);

		float avgScale = Tools::dot(ownerActorShared->getGlobalScale(), sf::Vector2f(1.0f, 1.0f)) / 2;
		float avgScaleOther = Tools::dot(otherOwnerActorShared->getGlobalScale(), sf::Vector2f(1.0f, 1.0f)) / 2;

		return radius*avgScale + coll->radius*avgScaleOther >= distance;
	}
	else
		return false;
}


//void CircleCollider::copy(const Component &other)
//{
//	const CircleCollider *component = dynamic_cast<const CircleCollider*>(&other);
//	if (component)
//	{
//		radius = component->radius;
//	}
//}


void CircleCollider::draw(sf::RenderWindow &window) const
{
	auto ownerActorShared = getOwnerActor().lock();
	if (!ownerActorShared)
		return;

	float avgScale = Tools::dot(ownerActorShared->getGlobalScale(), sf::Vector2f(1.0f, 1.0f)) / 2;
	float scaledRadius = static_cast<float>(radius) * avgScale;

	sf::CircleShape circle;
	circle.setRadius(scaledRadius);
	circle.setFillColor(sf::Color::Red);
	circle.setOrigin(scaledRadius, scaledRadius);

	circle.setPosition(Tools::ScaleVector(
		Tools::rotate(
			relativePosition,
			ownerActorShared->getGlobalRotation()),
		ownerActorShared->getGlobalScale()) + ownerActorShared->getGlobalPosition());
	
	window.draw(circle);
}