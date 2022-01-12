#ifndef PROJECTILE_CONTROLLER_H_
#define PROJECTILE_CONTROLLER_H_
#include <SFML/Window/Mouse.hpp>
#include "BehaviourScript.h"
#include "Actor.h"
#include "SetTransparency.h"
#include "NetworkManager.h"
#include "MessageHeader.h"


class ProjectileController : public BehaviourScript
{
	CLONEABLE_COMPONENT();

	Network::NetworkManager *networkManager;

	// Handle to game controller
	std::weak_ptr<Component> gameController;

	void awake() override;
	void update() override;
	void onCollision(std::weak_ptr<Actor> other) override;

	public:
		unsigned long playerId;
		int damageDealt;
		sf::Vector2f velocity = sf::Vector2f();

		void setNetworkManager(Network::NetworkManager *networkManager);
};


#endif