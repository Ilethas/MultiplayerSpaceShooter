#ifndef PLAYER_CONTROLLER_H_
#define PLAYER_CONTROLLER_H_
#include <SFML/Window/Mouse.hpp>
#include "BehaviourScript.h"
#include "Actor.h"
#include "SetTransparency.h"
#include "NetworkManager.h"
#include "MessageHeader.h"


class PlayerController : public BehaviourScript
{
	CLONEABLE_COMPONENT();

	Network::NetworkManager *networkManager;

	// Handle to game controller
	std::weak_ptr<Component> gameController;

	// Handles to jet actors
	std::weak_ptr<Actor> mainEngineJet;
	std::weak_ptr<Actor> reverseEngineJet;
	std::weak_ptr<Actor> leftEngineJet;
	std::weak_ptr<Actor> rightEngineJet;

	void awake() override;
	void update() override;
	void onKeyboardEvent(sf::Event event) override;
	void onCollision(std::weak_ptr<Actor> other) override;

	// Coroutines to manage ship movement
	class Accelerate;

	public:
		unsigned long playerId;

		// Acceleration applied by engines to move the ship
		float mainEngineAcceleration = 600.0f;
		float sideEngineAcceleration = 375.0f;

		// Speed vector of the actor
		int hitPoints;
		float maxVelocity = 750.0f;
		sf::Vector2f velocity = sf::Vector2f();

		void setNetworkManager(Network::NetworkManager *networkManager);
		void reactToKeyboard(sf::Event event);
};


class PlayerController::Accelerate : public Coroutine
{
	int state = 0;
	std::weak_ptr<Component> script;
	float acceleration;
	sf::Vector2f direction;

	std::unique_ptr<CoroutineResultType> operator()();

	public:
		Accelerate(std::weak_ptr<Component> script, sf::Vector2f direction, float acceleration);
};


#endif