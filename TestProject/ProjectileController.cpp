#include "ProjectileController.h"
#include "PlayerController.h"
#include "GameController.h"
#include "Game.h"

void ProjectileController::awake()
{
	gameController = Game::get().getRootActor().lock()->getChild("gameActor").lock()->getComponent<GameController>();
}


void ProjectileController::update()
{
	auto ownerActor = getOwnerActor().lock();
	auto controller = static_cast<GameController*>(gameController.lock().get());

	// Check if the projectile goes outside the map
	sf::Vector2f pos = ownerActor->getLocalPosition();

	if (pos.x > controller->getMapWidth() / 2 || pos.x < -controller->getMapWidth() / 2 ||
		pos.y > controller->getMapHeight() / 2 || pos.y < -controller->getMapHeight() / 2)
	{
		ownerActor->destroy();
	}

	// Update position
	ownerActor->setLocalPosition(ownerActor->getLocalPosition() + velocity * static_cast<float>(Game::get().deltaTime));
}


void ProjectileController::onCollision(std::weak_ptr<Actor> other)
{
	auto controller = static_cast<GameController*>(gameController.lock().get());

	auto otherLocked = other.lock();
	if (!otherLocked)
		return;

	auto component = otherLocked->getComponent<PlayerController>().lock();
	if (!component)
		return;

	auto playerController = static_cast<PlayerController*>(component.get());
	playerController->hitPoints -= damageDealt;

	if (controller->isHost())
	{
		auto ownerActor = getOwnerActor().lock();
		ownerActor->destroy();
		controller->sendDestroyActor(ownerActor->getId());

		if (playerController->hitPoints < 0)
		{
			otherLocked->destroy();
			controller->sendDestroyActor(otherLocked->getId());
		}
	}
}


void ProjectileController::setNetworkManager(Network::NetworkManager *networkManager)
{
	this->networkManager = networkManager;
}