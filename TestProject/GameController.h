#ifndef GAME_CONTROLLER_H_
#define GAME_CONTROLLER_H_
#include <SFML/Window/Mouse.hpp>
#include <regex>
#include "BehaviourScript.h"
#include "CircleCollider.h"
#include "Actor.h"
#include "NetworkManager.h"
#include "PlayerInfo.h"
#include "MessageHeader.h"
#include "ThreadsafeDataQueue.h"


class GameController : public BehaviourScript
{
	CLONEABLE_COMPONENT();

	Network::NetworkManager *networkManager;
	ThreadsafeDataQueue *networkDataQueue;
	std::list<PlayerInfo> otherPlayers;
	unsigned long playerId;
	bool host = false;

	float mapWidth;
	float mapHeight;
	float minimapViewportScale;
	std::list<std::weak_ptr<Actor>> playerShips;
	std::list<std::weak_ptr<Actor>> projectiles;

	struct reponseHandler
	{
		std::string choice;
		std::string choiceText;
		std::function<void(std::string)> handler;
	};
	void consoleGetInput(std::vector<reponseHandler> responseHandlers);
	void onHostGame(std::string response, bool &readyToStart);
	void onJoinGame(std::string response, bool &readyToStart);
	void onReceiveData(unsigned long connectionId, const char *data, int dataSize);

	void start() override;
	void update() override;
	void onKeyboardEvent(sf::Event event) override;
	void onMouseEvent(sf::Event event) override;
	void drawMinimap();

	void onCreateMap(std::vector<char> dataChunk);
	void onCreatePlayerShip(std::vector<char> dataChunk);
	void onEvent(std::vector<char> dataChunk);
	void onSetRotation(std::vector<char> dataChunk);
	void onSetPosition(std::vector<char> dataChunk);
	void onSetVelocity(std::vector<char> dataChunk);
	void onDestroyActor(std::vector<char> dataChunk);

	// Coroutines
	class SynchronizationUpdate;

	public:
		std::shared_ptr<Actor> createPlayerShip(float x, float y, float rotation, unsigned long playerId, std::string name);
		std::shared_ptr<Actor> createProjectile(float x, float y, float rotation, unsigned long playerId, std::string name);
		void createMap(float width, float height, unsigned int seed);

		void setNetworkManager(Network::NetworkManager *networkManager);
		void setDataQueue(ThreadsafeDataQueue *networkDataQueue);
		float getMapWidth() const;
		float getMapHeight() const;
		bool isHost() const;
		unsigned long getPlayerId() const;
		std::list<PlayerInfo> getPlayers() const;

		void sendCreateMap(unsigned int seed);
		void sendCreatePlayerShip(float x, float y, float rotation, unsigned long playerId, std::string name);
		void sendEvent(sf::Event event);
		void sendSetRotation(float angle, unsigned long actorId);
		void sendSetPosition(sf::Vector2f position, unsigned long actorId);
		void sendSetVelocity(sf::Vector2f velocity, unsigned long actorId);
		void sendDestroyActor(unsigned long actorId);
};


class GameController::SynchronizationUpdate : public Coroutine
{
	int state = 0;
	GameController *controller = nullptr;
	std::unique_ptr<CoroutineResultType> operator()();

	public:
		SynchronizationUpdate(GameController *controller);
};


#endif