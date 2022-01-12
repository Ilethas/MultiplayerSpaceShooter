#include "PlayerController.h"
#include "ProjectileController.h"
#include "GameController.h"
#include "Game.h"


void PlayerController::awake()
{
	gameController = Game::get().getRootActor().lock()->getChild("gameActor").lock()->getComponent<GameController>();
	auto ownerActor = getOwnerActor().lock();

	mainEngineJet = ownerActor->getChildRecursive("mainEngineJet");
	reverseEngineJet = ownerActor->getChildRecursive("reverseEngineJet");
	leftEngineJet = ownerActor->getChildRecursive("leftEngineJet");
	rightEngineJet = ownerActor->getChildRecursive("rightEngineJet");
}


void PlayerController::update()
{
	Game &game = Game::get();
	auto ownerActor = getOwnerActor().lock();
	auto controller = static_cast<GameController*>(gameController.lock().get());

	// Check if the ship goes outside the map
	sf::Vector2f pos = ownerActor->getLocalPosition();

	if (pos.x > controller->getMapWidth() / 2)
		ownerActor->setLocalPosition(-pos.x + 50.0f, pos.y);
	else if (pos.x < -controller->getMapWidth() / 2)
		ownerActor->setLocalPosition(-pos.x - 50.0f, pos.y);
	else if (pos.y > controller->getMapHeight() / 2)
		ownerActor->setLocalPosition(pos.x, -pos.y + 50.0f);
	else if (pos.y < -controller->getMapHeight() / 2)
		ownerActor->setLocalPosition(pos.x, -pos.y - 50.0f);

	// Check if max speed exceeded
	if (Tools::length(velocity) > maxVelocity)
		velocity = velocity / Tools::length(velocity) * maxVelocity;

	// Update position
	ownerActor->setLocalPosition(ownerActor->getGlobalPosition() + velocity * static_cast<float>(Game::get().deltaTime));

	if (controller->getPlayerId() == playerId)
	{
		// Update the camera to track the player
		sf::View newView = game.window.getView();
		newView.setCenter(ownerActor->getGlobalPosition());
		game.window.setView(newView);

		// Update facing angle
		sf::Vector2f mousePos = game.window.mapPixelToCoords(sf::Mouse::getPosition(game.window), game.window.getView());
		mousePos -= ownerActor->getGlobalPosition();

		float angle = Tools::rad2deg(std::atan2(mousePos.y, mousePos.x));
		ownerActor->setLocalRotation(angle);
		controller->sendSetRotation(angle, ownerActor->getId());
	}
}


void PlayerController::onKeyboardEvent(sf::Event event)
{
	auto controller = static_cast<GameController*>(gameController.lock().get());
	if (controller->getPlayerId() != playerId)
		return;

	reactToKeyboard(event);
}


void PlayerController::onCollision(std::weak_ptr<Actor> other)
{
	//getOwnerActor().lock()->destroy();
}


void PlayerController::setNetworkManager(Network::NetworkManager *networkManager)
{
	this->networkManager = networkManager;
}


void PlayerController::reactToKeyboard(sf::Event event)
{
	auto ownerActor = getOwnerActor().lock();
	auto controller = static_cast<GameController*>(gameController.lock().get());

	if (event.type == sf::Event::KeyPressed)
	{
		switch (event.key.code)
		{
			case sf::Keyboard::W:
				coroutineMaster.stopCoroutine("litMainEngine");
				coroutineMaster.startCoroutine(SetTransparency(mainEngineJet, 255, 0.01f, 0.1f), "litMainEngine");
				coroutineMaster.stopCoroutine("accelerateForward");
				coroutineMaster.startCoroutine(Accelerate(getHandle(), sf::Vector2f(1.0f, 0.0f), mainEngineAcceleration), "accelerateForward");
				break;

			case sf::Keyboard::S:
				coroutineMaster.stopCoroutine("litReverseEngine");
				coroutineMaster.startCoroutine(SetTransparency(reverseEngineJet, 255, 0.01f, 0.1f), "litReverseEngine");
				coroutineMaster.stopCoroutine("accelerateBackward");
				coroutineMaster.startCoroutine(Accelerate(getHandle(), sf::Vector2f(-1.0f, 0.0f), sideEngineAcceleration), "accelerateBackward");
				break;

			case sf::Keyboard::A:
				coroutineMaster.stopCoroutine("litRightEngine");
				coroutineMaster.startCoroutine(SetTransparency(rightEngineJet, 255, 0.01f, 0.1f), "litRightEngine");
				coroutineMaster.stopCoroutine("accelerateLeft");
				coroutineMaster.startCoroutine(Accelerate(getHandle(), sf::Vector2f(0.0f, -1.0f), sideEngineAcceleration), "accelerateLeft");
				break;

			case sf::Keyboard::D:
				coroutineMaster.stopCoroutine("litLeftEngine");
				coroutineMaster.startCoroutine(SetTransparency(leftEngineJet, 255, 0.01f, 0.1f), "litLeftEngine");
				coroutineMaster.stopCoroutine("accelerateRight");
				coroutineMaster.startCoroutine(Accelerate(getHandle(), sf::Vector2f(0.0f, 1.0f), sideEngineAcceleration), "accelerateRight");
				break;

			case sf::Keyboard::Space:
				{
					sf::Vector2f pos = ownerActor->getLocalPosition();
					auto projectile = controller->createProjectile(pos.x, pos.y, ownerActor->getLocalRotation(), playerId, "");

					auto component = projectile->getComponent<ProjectileController>().lock();
					if (!component)
						break;

					auto projectileController = static_cast<ProjectileController*>(component.get());
					projectileController->velocity = velocity + Tools::rotate(sf::Vector2f(750.0f, 0.0f), ownerActor->getLocalRotation());
				}
				break;
		}
	}
	else if (event.type == sf::Event::KeyReleased)
	{
		switch (event.key.code)
		{
			case sf::Keyboard::W:
				coroutineMaster.stopCoroutine("litMainEngine");
				coroutineMaster.startCoroutine(SetTransparency(mainEngineJet, 0, 0.01f, 0.1f), "litMainEngine");
				coroutineMaster.stopCoroutine("accelerateForward");
				break;

			case sf::Keyboard::S:
				coroutineMaster.stopCoroutine("litReverseEngine");
				coroutineMaster.startCoroutine(SetTransparency(reverseEngineJet, 0, 0.01f, 0.1f), "litReverseEngine");
				coroutineMaster.stopCoroutine("accelerateBackward");
				break;

			case sf::Keyboard::A:
				coroutineMaster.stopCoroutine("litRightEngine");
				coroutineMaster.startCoroutine(SetTransparency(rightEngineJet, 0, 0.01f, 0.1f), "litRightEngine");
				coroutineMaster.stopCoroutine("accelerateLeft");
				break;

			case sf::Keyboard::D:
				coroutineMaster.stopCoroutine("litLeftEngine");
				coroutineMaster.startCoroutine(SetTransparency(leftEngineJet, 0, 0.01f, 0.1f), "litLeftEngine");
				coroutineMaster.stopCoroutine("accelerateRight");
				break;
		}
	}
}


std::unique_ptr<CoroutineResultType> PlayerController::Accelerate::operator()()
{
	coroutineBegin();
	while (true)
	{
		static_cast<PlayerController*>(script.lock().get())->velocity += Tools::rotate(direction, script.lock()->getOwnerActor().lock()->getGlobalRotation())*acceleration*static_cast<float>(Game::get().deltaTime);
		yieldReturn(nullptr);
	}
	coroutineEnd(mkUniq(CoroutineFinished()));
}


// Accelerates actor to which the script 'script' is attached.
// The actor is then moved using the velocity stored in the script
// in the update method.
PlayerController::Accelerate::Accelerate(std::weak_ptr<Component> script, sf::Vector2f direction, float acceleration)
	: script(script)
	, acceleration(acceleration)
	, direction(direction)
{}