#include "GameController.h"
#include "PlayerController.h"
#include "ProjectileController.h"
#include "Game.h"


void GameController::consoleGetInput(std::vector<reponseHandler> responseHandlers)
{
	for (auto &i : responseHandlers)
	{
		printf("%s. %s\n", i.choice.c_str(), i.choiceText.c_str());
	}

	bool inputAcquired = false;
	while (inputAcquired == false)
	{
		std::string response;
		std::getline(std::cin, response);
		for (auto &i : responseHandlers)
		{
			if (response == i.choice)
			{
				std::cout << std::endl;
				if (i.handler != nullptr)
					i.handler(response);

				inputAcquired = true;
				break;
			}
		}

		if (inputAcquired == false)
		{
			printf("\nNieprawidlowa odpowiedz, sproboj ponownie.\n");
		}
	}
}


void GameController::onHostGame(std::string response, bool &readyToStart)
{
	host = true;
	networkManager->setOnConnect([&](unsigned long connectionId)
	{
		printf("Gracz dolacza do poczekalni.\n");
	});
	networkManager->setOnDisconnect([&](unsigned long connectionId)
	{
		printf("Gracz opuszcza poczekalnie.\n");
	});
	networkManager->startListening("2701");

	std::vector<reponseHandler> hostGameMenu =
	{
		{ "1", "Rozpocznij rozgrywke" },
		{ "2", "Anuluj" },
	};

	// Game start handler
	hostGameMenu[0].handler = [&](std::string response)
	{
		readyToStart = true;
		networkManager->stopListening();
		auto connections = networkManager->getConnections();

		// Give each player an identifier
		playerId = 1;

		unsigned long i = playerId + 1;
		for (auto &connectionId : connections)
		{
			otherPlayers.push_back({ connectionId, i++ });
		}

		// Send information about players to all clients (send client's player number as the last one
		std::vector<char> buffer(sizeof(MessageHeader) + sizeof(unsigned long));
		auto sendBuffer = [&](unsigned long connectionId, MessageHeader header, unsigned long playerId)
		{
			*reinterpret_cast<MessageHeader*>(buffer.data()) = header;
			*reinterpret_cast<unsigned long*>(buffer.data() + sizeof(MessageHeader)) = playerId;
			networkManager->send(connectionId, buffer.data(), sizeof(MessageHeader) + sizeof(unsigned long));
		};

		for (auto &connectionId : connections)
		{
			// Send host player identifier
			sendBuffer(connectionId, MessageHeader::OTHER_PLAYER_ID, playerId);

			// Send identifiers of players with id different to @connectionId
			for (auto &player : otherPlayers)
			{
				if (player.connectionId != connectionId)
					sendBuffer(connectionId, MessageHeader::OTHER_PLAYER_ID, player.playerId);
			}

			// Send id of the client player as the last one
			for (auto &player : otherPlayers)
			{
				if (player.connectionId == connectionId)
				{
					sendBuffer(connectionId, MessageHeader::THIS_PLAYER_ID, player.playerId);
					break;
				}
			}
		}

		networkManager->setOnDisconnect([&](unsigned long connectionId)
		{
			printf("Gracz opuszcza gre.\n");
		});
	};

	// Cancel handler
	hostGameMenu[1].handler = [&](std::string response)
	{
		networkManager->setOnDisconnect([&](unsigned long connectionId) {});
		networkManager->stopListening();
		networkManager->distonnectAll();
	};

	consoleGetInput(hostGameMenu);
}


void GameController::onJoinGame(std::string, bool &readyToStart)
{
	host = false;
	networkManager->setOnConnect([&](unsigned long connectionId)
	{
		printf("Udalo sie polaczyc z hostem.\nOczekiwanie na rozpoczecie gry...\n");
	});

	// Get server IP
	std::string server;
	printf("Podaj IP hosta: ");
	std::getline(std::cin, server);

	std::mutex infoBlockade;
	std::condition_variable cv;
	bool playerInfoReceived = false;
	bool disconnected = false;

	networkManager->setOnDisconnect([&](unsigned long connectionId)
	{
		networkManager->setOnDisconnect([&](unsigned long) {});
		disconnected = true;
		cv.notify_all();
	});

	try
	{
		// Connect with server and ask for other players in game
		otherPlayers.clear();
		networkManager->setOnReceive([&](unsigned long connectionId, const char *data, int dataSize)
		{
			unsigned long receivedPlayerId = *reinterpret_cast<const unsigned long*>(data + sizeof(MessageHeader));

			if (*reinterpret_cast<const MessageHeader*>(data) == MessageHeader::THIS_PLAYER_ID)
			{
				playerId = receivedPlayerId;
				playerInfoReceived = true;
				cv.notify_all();
			}
			else if (*reinterpret_cast<const MessageHeader*>(data) == MessageHeader::OTHER_PLAYER_ID)
			{
				otherPlayers.push_back({ connectionId, receivedPlayerId });
			}
		});
		networkManager->connectAsClient(server, "2701");

		// Waiting for the server to send information about players
		std::unique_lock<std::mutex> infoLock(infoBlockade);
		while (playerInfoReceived == false && disconnected == false)
			cv.wait(infoLock);

		if (disconnected)
		{
			printf("Utracono polaczenie z hostem.\n\n");
			return;
		}
		else
			readyToStart = true;

		networkManager->setOnDisconnect([&](unsigned long connectionId)
		{
			MessageBoxA(0, "Utracono polaczenie z serwerem!", "Blad", MB_OK);
			std::exit(0);
		});
	}
	catch (std::exception&)
	{
		printf("Nie udalo sie polaczyc z serwerem.\n\n");
	}
}


void GameController::onReceiveData(unsigned long connectionId, const char *data, int dataSize)
{
	if (networkDataQueue != nullptr)
	{
		std::unique_lock<std::mutex> queueLock(networkDataQueue->blockade);

		std::vector<char> dataChunk(dataSize);
		std::memcpy(dataChunk.data(), data, dataSize);
		networkDataQueue->dataQueue.push({ std::move(dataChunk), connectionId });
	}
}


void GameController::start()
{
	mapWidth = 12000;
	mapHeight = 12000;
	minimapViewportScale = 0.35f;

	if (networkManager == nullptr)
		std::cerr << "Error: NetworkManager pointer set to nullptr" << std::endl;

	bool readyToStart = false;
	while (readyToStart == false)
	{
		std::vector<reponseHandler> mainMenu =
		{
			{ "1", "Zahostuj gre", std::bind(&GameController::onHostGame, this, std::placeholders::_1, std::ref<bool>(readyToStart)) },
			{ "2", "Dolacz do gry", std::bind(&GameController::onJoinGame, this, std::placeholders::_1, std::ref<bool>(readyToStart)) },
			{ "3", "Wyjdz z gry", [](std::string) { std::exit(0); } }
		};
		consoleGetInput(mainMenu);

		if (readyToStart)
		{
			std::cout << "Moje id: " << playerId << std::endl;
			for (auto &i : otherPlayers)
			{
				std::cout << "Gracz " << i.playerId << " ma polaczenie nr " << i.connectionId << std::endl;
			}
		}
	}

	// Set data receive handler
	networkManager->setOnReceive(std::bind(&GameController::onReceiveData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	if (host)
	{
		// Returns random float from @a to @b
		auto random = [](float a, float b) { return (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * (b - a) + a); };

		// Create map and send the seed to all players
		createMap(mapWidth, mapHeight, static_cast<unsigned int>(std::time(0)));

		// Create a ship for all players
		createPlayerShip(random(-mapWidth / 2, mapWidth / 2), random(-mapHeight / 2, mapHeight / 2), random(0, 360), playerId, "playerShip" + std::to_string(playerId));
		for (auto &player : otherPlayers)
		{
			createPlayerShip(random(-mapWidth / 2, mapWidth / 2), random(-mapHeight / 2, mapHeight / 2), random(0, 360), player.playerId, "playerShip" + std::to_string(player.playerId));
		}

		// Run synchronization coroutine
		coroutineMaster.startCoroutine(SynchronizationUpdate(this), "synchronizationCoroutine");
	}

	//Game::get().drawColliders = true;
}


void GameController::update()
{
	if (networkDataQueue != nullptr)
	{
		std::unique_lock<std::mutex> queueLock(networkDataQueue->blockade);

		// Perform operations based on message header received from client/server
		while (networkDataQueue->dataQueue.empty() == false)
		{
			auto item = networkDataQueue->dataQueue.front();
			auto dataChunk = item.first;
			networkDataQueue->dataQueue.pop();

			MessageHeader header = *reinterpret_cast<MessageHeader*>(dataChunk.data());
			switch (header)
			{
				case MessageHeader::CREATE_MAP:
					onCreateMap(std::move(dataChunk));
					break;

				case MessageHeader::CREATE_PLAYER_SHIP:
					onCreatePlayerShip(std::move(dataChunk));
					break;

				case MessageHeader::PLAYER_INPUT_EVENT:
					onEvent(std::move(dataChunk));
					break;

				case MessageHeader::SET_ROTATION:
					onSetRotation(std::move(dataChunk));
					break;

				case MessageHeader::SET_POSITION:
					onSetPosition(std::move(dataChunk));
					break;

				case MessageHeader::SET_VELOCITY:
					onSetVelocity(std::move(dataChunk));
					break;

				case MessageHeader::DESTROY_ACTOR:
					onDestroyActor(std::move(dataChunk));
					break;
			}

			queueLock.unlock();
			if (isHost())
			{
				for (auto &player : otherPlayers)
				{
					if (player.connectionId != item.second)
						networkManager->send(player.connectionId, dataChunk.data(), dataChunk.size());
				}
			}
			queueLock.lock();
		}
	}

	// Remove destroyed ships and projectiles from lists
	playerShips.remove_if([](std::weak_ptr<Actor> &actor) { return actor.expired(); });
	projectiles.remove_if([](std::weak_ptr<Actor> &actor) { return actor.expired(); });

	drawMinimap();
}


void GameController::onKeyboardEvent(sf::Event event)
{
	sendEvent(event);
}


void GameController::onMouseEvent(sf::Event event)
{
	sendEvent(event);
}


void GameController::drawMinimap()
{
	Game &game = Game::get();

	// Save old game view and create one containing the whole game area
	sf::View oldView = game.window.getView();
	sf::View minimap = sf::View(sf::FloatRect(-mapWidth / 2, -mapHeight / 2, mapWidth, mapHeight));

	// Determine the viewport
	sf::Vector2f minimapSize;
	sf::Vector2u winSize = game.window.getSize();
	if (winSize.x > winSize.y)
		minimapSize = sf::Vector2f(minimapViewportScale * winSize.y / winSize.x, minimapViewportScale);
	else
		minimapSize = sf::Vector2f(minimapViewportScale, minimapViewportScale * winSize.x / winSize.y);

	minimap.setViewport(sf::FloatRect(1.0f - minimapSize.x, 0.0f, minimapSize.x, minimapSize.y));

	// Draw a black frame before drawing the minimap
	float scaleX = static_cast<float>(game.initialGameViewWidth) / game.initialWindowWidth * winSize.x;
	float scaleY = static_cast<float>(game.initialGameViewHeight) / game.initialWindowHeight * winSize.y;

	sf::RectangleShape rectangle(sf::Vector2f(minimapSize.x * scaleX, minimapSize.y * scaleY));
	rectangle.setOrigin(minimapSize.x * scaleX, 0.0f);
	rectangle.setPosition(game.window.mapPixelToCoords(sf::Vector2i(winSize.x, 0)));

	rectangle.setFillColor(sf::Color::Black);
	rectangle.setOutlineColor(sf::Color(50, 50, 50));
	rectangle.setOutlineThickness(3.0f);
	game.window.draw(rectangle);

	// Set the minimap as the current view and draw the whole world
	game.window.setView(minimap);
	game.drawActors();

	// Draw all player ships as dots
	sf::CircleShape circle(200);
	circle.setOrigin(circle.getRadius() / 2, circle.getRadius() / 2);

	for (auto &i : playerShips)
	{
		auto ship = i.lock();
		if (!ship)
			continue;

		auto controller = i.lock()->getComponent<PlayerController>().lock();
		unsigned long id = static_cast<PlayerController*>(controller.get())->playerId;

		// Color the ship red for enemy and green for the player himself
		if (playerId == id)
			circle.setFillColor(sf::Color::Green);
		else
			circle.setFillColor(sf::Color::Red);

		circle.setPosition(ship->getLocalPosition());
		game.window.draw(circle);
	}

	game.window.setView(oldView);
}


void GameController::onCreateMap(std::vector<char> dataChunk)
{
	unsigned int seed = *reinterpret_cast<unsigned int*>(dataChunk.data() + sizeof(MessageHeader));
	createMap(mapWidth, mapHeight, seed);
}


void GameController::onCreatePlayerShip(std::vector<char> dataChunk)
{
	float x = *reinterpret_cast<float*>(dataChunk.data() + sizeof(MessageHeader));
	float y = *reinterpret_cast<float*>(dataChunk.data() + sizeof(MessageHeader) + sizeof(float));
	float angle = *reinterpret_cast<float*>(dataChunk.data() + sizeof(MessageHeader) + 2 * sizeof(float));
	unsigned long id = *reinterpret_cast<unsigned long*>(dataChunk.data() + sizeof(MessageHeader) + 3 * sizeof(float));
	std::string name(dataChunk.data() + sizeof(MessageHeader) + 3 * sizeof(float) + sizeof(unsigned long));

	createPlayerShip(x, y, angle, id, name);
}


void GameController::onEvent(std::vector<char> dataChunk)
{
	sf::Event event = *reinterpret_cast<sf::Event*>(dataChunk.data() + sizeof(MessageHeader));
	unsigned long eventPlayerId = *reinterpret_cast<unsigned long*>(dataChunk.data() + sizeof(MessageHeader) + sizeof(sf::Event));

	for (auto &ship : playerShips)
	{
		auto shipLocked = ship.lock();
		if (shipLocked)
		{
			auto component = shipLocked->getComponent<PlayerController>().lock();
			if (!component)
				continue;

			auto playerController = static_cast<PlayerController*>(component.get());
			if (playerController->playerId == eventPlayerId)
			{
				playerController->reactToKeyboard(event);
				break;
			}
		}
	}
}


void GameController::onSetRotation(std::vector<char> dataChunk)
{
	float rotation = *reinterpret_cast<float*>(dataChunk.data() + sizeof(MessageHeader));
	unsigned long actorId = *reinterpret_cast<unsigned long*>(dataChunk.data() + sizeof(MessageHeader) + sizeof(float));

	for (auto &ship : playerShips)
	{
		auto shipLocked = ship.lock();
		if (!shipLocked)
			continue;

		if (shipLocked->getId() == actorId)
		{
			shipLocked->setLocalRotation(rotation);
			return;
		}
	}

	for (auto &projectile : projectiles)
	{
		auto projectileLocked = projectile.lock();
		if (!projectileLocked)
			continue;

		if (projectileLocked->getId() == actorId)
		{
			projectileLocked->setLocalRotation(rotation);
			break;
		}
	}
}


void GameController::onSetPosition(std::vector<char> dataChunk)
{
	sf::Vector2f position = *reinterpret_cast<sf::Vector2f*>(dataChunk.data() + sizeof(MessageHeader));
	unsigned long actorId = *reinterpret_cast<unsigned long*>(dataChunk.data() + sizeof(MessageHeader) + sizeof(sf::Vector2f));

	for (auto &ship : playerShips)
	{
		auto shipLocked = ship.lock();
		if (!shipLocked)
			continue;

		if (shipLocked->getId() == actorId)
		{
			shipLocked->setLocalPosition(position);
			return;
		}
	}

	for (auto &projectile : projectiles)
	{
		auto projectileLocked = projectile.lock();
		if (!projectileLocked)
			continue;

		if (projectileLocked->getId() == actorId)
		{
			projectileLocked->setLocalPosition(position);
			break;
		}
	}
}


void GameController::onSetVelocity(std::vector<char> dataChunk)
{
	sf::Vector2f velocity = *reinterpret_cast<sf::Vector2f*>(dataChunk.data() + sizeof(MessageHeader));
	unsigned long actorId = *reinterpret_cast<unsigned long*>(dataChunk.data() + sizeof(MessageHeader) + sizeof(sf::Vector2f));

	for (auto &ship : playerShips)
	{
		auto shipLocked = ship.lock();
		if (!shipLocked)
			continue;

		if (shipLocked->getId() == actorId)
		{
			auto component = shipLocked->getComponent<PlayerController>().lock();
			if (!component)
				return;

			auto playerController = static_cast<PlayerController*>(component.get());
			playerController->velocity = velocity;
			return;
		}
	}

	for (auto &projectile : projectiles)
	{
		auto projectileLocked = projectile.lock();
		if (!projectileLocked)
			continue;

		if (projectileLocked->getId() == actorId)
		{
			auto component = projectileLocked->getComponent<PlayerController>().lock();
			if (!component)
				return;

			auto projectileController = static_cast<ProjectileController*>(component.get());
			projectileController->velocity = velocity;
			break;
		}
	}
}


void GameController::onDestroyActor(std::vector<char> dataChunk)
{
	unsigned long actorId = *reinterpret_cast<unsigned long*>(dataChunk.data() + sizeof(MessageHeader));

	for (auto &ship : playerShips)
	{
		auto shipLocked = ship.lock();
		if (!shipLocked)
			continue;

		if (shipLocked->getId() == actorId)
		{
			shipLocked->destroy();
			return;
		}
	}

	for (auto &projectile : projectiles)
	{
		auto projectileLocked = projectile.lock();
		if (!projectileLocked)
			continue;

		if (projectileLocked->getId() == actorId)
		{
			projectileLocked->destroy();
			break;
		}
	}
}


std::shared_ptr<Actor> GameController::createPlayerShip(float x, float y, float rotation, unsigned long playerId, std::string name)
{
	Game &game = Game::get();

	auto playerShip = Actor::createActor(Game::get().getRootActor(), name);
	playerShip->setTexture(game.textures.playerShip1);
	playerShip->setOrigin(game.textures.playerShip1.getSize().x / 2.0f, game.textures.playerShip1.getSize().y / 2.0f);

	auto mainEngine = Actor::createActor(Game::get().getRootActor(), "mainEngine");
	mainEngine->setParent(playerShip);
	mainEngine->setLocalPosition(-60.0f, 0.0f);
	mainEngine->setLocalRotation(180.0f);

	auto reverseEngine = Actor::createActor(Game::get().getRootActor(), "reverseEngine");
	reverseEngine->setTexture(game.textures.playerShipEngine);
	reverseEngine->setOrigin(game.textures.playerShipEngine.getSize().x / 2.0f, game.textures.playerShipEngine.getSize().y / 2.0f);
	reverseEngine->setParent(playerShip);
	reverseEngine->setLocalPosition(-25.0f, 0.0f);
	reverseEngine->setDepth(2);

	auto leftEngine = reverseEngine->clone("leftEngine");
	leftEngine->setParent(playerShip);
	leftEngine->setLocalPosition(10.0f, -28.0f);
	leftEngine->setLocalRotation(-90.0f);

	auto rightEngine = reverseEngine->clone("rightEngine");
	rightEngine->setParent(playerShip);
	rightEngine->setLocalPosition(10.0f, 28.0f);
	rightEngine->setLocalRotation(90.0f);

	auto mainEngineJet = Actor::createActor(Game::get().getRootActor(), "mainEngineJet");
	mainEngineJet->setTexture(game.textures.engineJet1);
	mainEngineJet->setOrigin(0.0f, game.textures.engineJet1.getSize().y / 2.0f);
	mainEngineJet->setParent(mainEngine);
	mainEngineJet->setLocalRotation(0.0f);
	mainEngineJet->setDepth(-1);
	mainEngineJet->setOpacity(0);

	auto leftEngineJet = mainEngineJet->clone("leftEngineJet");
	leftEngineJet->setLocalScale(0.5f);
	leftEngineJet->setLocalPosition(10.0f, 0.0f);
	leftEngineJet->setParent(leftEngine);

	auto rightEngineJet = leftEngineJet->clone("rightEngineJet");
	rightEngineJet->setLocalScale(0.5f);
	rightEngineJet->setParent(rightEngine);

	auto reverseEngineJet = rightEngineJet->clone("reverseEngineJet");
	reverseEngineJet->setLocalScale(0.5f);
	reverseEngineJet->setDepth(1);
	reverseEngineJet->setParent(reverseEngine);

	// Add colliders to the player
	auto col = playerShip->addComponent<CircleCollider>().lock();
	static_cast<CircleCollider*>(col.get())->radius = 37;
	static_cast<CircleCollider*>(col.get())->relativePosition.x = 40;

	col = playerShip->addComponent<CircleCollider>().lock();
	static_cast<CircleCollider*>(col.get())->radius = 50;
	static_cast<CircleCollider*>(col.get())->relativePosition.x = -20;

	col = playerShip->addComponent<CircleCollider>().lock();
	static_cast<CircleCollider*>(col.get())->radius = 20;
	static_cast<CircleCollider*>(col.get())->relativePosition = sf::Vector2f(-40, -60);

	col = playerShip->addComponent<CircleCollider>().lock();
	static_cast<CircleCollider*>(col.get())->radius = 20;
	static_cast<CircleCollider*>(col.get())->relativePosition = sf::Vector2f(-40, 60);

	playerShip->setLocalPosition(x, y);
	playerShip->setLocalRotation(rotation);

	// Add scripts
	auto component = playerShip->addComponent<PlayerController>().lock();
	auto playerController = static_cast<PlayerController*>(component.get());
	playerController->playerId = playerId;
	playerController->hitPoints = 150;

	playerShips.push_back(playerShip);
	if (host)
	{
		sendCreatePlayerShip(x, y, rotation, playerId, name);
	}
	return playerShip;
}


std::shared_ptr<Actor> GameController::createProjectile(float x, float y, float rotation, unsigned long playerId, std::string name)
{
	Game &game = Game::get();

	auto projectile = Actor::createActor(Game::get().getRootActor(), name);
	projectile->setTexture(game.textures.plasmaFire);
	projectile->setOrigin(game.textures.plasmaFire.getSize().x / 2.0f, game.textures.plasmaFire.getSize().y / 2.0f);

	sf::Vector2f posOffset = Tools::rotate(sf::Vector2f(90.0f, 0.0f), rotation);
	projectile->setLocalPosition(x + posOffset.x, y + posOffset.y);
	projectile->setLocalRotation(rotation);
	projectile->setLocalScale(0.5f);

	// Add colliders to the projectile
	auto col = projectile->addComponent<CircleCollider>().lock();
	static_cast<CircleCollider*>(col.get())->radius = 20;

	// Add scripts
	auto component = projectile->addComponent<ProjectileController>().lock();
	auto projectileController = static_cast<ProjectileController*>(component.get());
	projectileController->playerId = playerId;
	projectileController->damageDealt = 10;

	projectiles.push_back(projectile);
	if (host)
	{
		//sendCreatePlayerShip(x, y, rotation, playerId, name);
	}
	return projectile;
}


void GameController::createMap(float width, float height, unsigned int seed)
{
	unsigned int newSeed = std::rand();
	std::srand(seed);
	Game &game = Game::get();

	// Returns random float from 0.0f to maxVal
	auto random = [](float maxVal) { return (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * maxVal); };

	auto createDecoration = [&](const sf::Texture &tex, const std::string &name, int numberSuffix, float size = 1.0f, int depth = -2)
	{
		auto newDecoration = Actor::createActor(Game::get().getRootActor(), name + std::to_string(numberSuffix));
		newDecoration->setTexture(tex);
		newDecoration->setOrigin(tex.getSize().x / 2.0f, tex.getSize().y / 2.0f);
		newDecoration->setDepth(depth);

		newDecoration->setLocalPosition(sf::Vector2f(random(mapWidth) - mapWidth / 2, random(mapHeight) - mapHeight / 2));
		newDecoration->setLocalScale(size);
		newDecoration->setLocalRotation(random(360.0f));
		return newDecoration;
	};

	// Generate random stars
	for (int i = 0; i < 100; i++)
	{
		createDecoration(game.textures.starsSmall, "starsSmall", i);
		createDecoration(game.textures.starsMedium, "starsMedium", i);
		createDecoration(game.textures.starsLarge, "starsLarge", i);
	}

	// Generate random dusts
	for (int i = 0; i < 10 + (std::rand() % 6); i++)
	{
		createDecoration(game.textures.dust, "dust", i, 1.0f + random(6.0f));
		createDecoration(game.textures.blueDust, "blueDust", i, 1.0f + random(2.25f));
		createDecoration(game.textures.violetDust, "violetDust", i, 1.0f + random(2.25f));
		createDecoration(game.textures.yellowDust, "yellowDust", i, 1.0f + random(2.25f));
	}

	// Create random nebulas
	for (int i = 0; i < std::rand() % 4; i++)
		createDecoration(game.textures.coldNebula, "coldNebula", i, 1.0f + random(2.0f));

	for (int i = 0; i < std::rand() % 4; i++)
		createDecoration(game.textures.hotNebula, "hotNebula", i, 1.0f + random(2.0f));


	// Create planets
	createDecoration(game.textures.exoplanet, "exoplanet", 0, 1.25f, -1);
	createDecoration(game.textures.exoplanet2, "exoplanet2", 0, 1.35f, -1);
	createDecoration(game.textures.exoplanet3, "exoplanet3", 0, 1.15f, -1);
	createDecoration(game.textures.iceGiant, "iceGiant", 0, 1.35f, -1);
	createDecoration(game.textures.gasGiant, "gasGiant", 0, 2.0f, -1);

	std::srand(newSeed);
	if (host)
	{
		sendCreateMap(seed);
	}
}


void GameController::setNetworkManager(Network::NetworkManager *networkManager)
{
	this->networkManager = networkManager;
}


void GameController::setDataQueue(ThreadsafeDataQueue *networkDataQueue)
{
	this->networkDataQueue = networkDataQueue;
}


float GameController::getMapWidth() const
{
	return mapWidth;
}


float GameController::getMapHeight() const
{
	return mapHeight;
}


bool GameController::isHost() const
{
	return host;
}


unsigned long GameController::getPlayerId() const
{
	return playerId;
}


std::list<PlayerInfo> GameController::getPlayers() const
{
	return otherPlayers;
}


void GameController::sendCreateMap(unsigned int seed)
{
	if (networkManager != nullptr)
	{
		std::vector<char> dataChunk(sizeof(MessageHeader) + sizeof(unsigned int));

		*reinterpret_cast<MessageHeader*>(dataChunk.data()) = MessageHeader::CREATE_MAP;
		memcpy(dataChunk.data() + sizeof(MessageHeader), &seed, sizeof(unsigned int));

		networkManager->sendToAll(dataChunk.data(), dataChunk.size());
	}
}


void GameController::sendCreatePlayerShip(float x, float y, float rotation, unsigned long playerId, std::string name)
{
	if (networkManager != nullptr)
	{
		std::vector<char> dataChunk(sizeof(MessageHeader) + 3 * sizeof(float) + sizeof(unsigned long) + name.size() + 1);

		*reinterpret_cast<MessageHeader*>(dataChunk.data()) = MessageHeader::CREATE_PLAYER_SHIP;
		memcpy(dataChunk.data() + sizeof(MessageHeader), &x, sizeof(float));
		memcpy(dataChunk.data() + sizeof(MessageHeader) + sizeof(float), &y, sizeof(float));
		memcpy(dataChunk.data() + sizeof(MessageHeader) + 2 * sizeof(float), &rotation, sizeof(float));
		memcpy(dataChunk.data() + sizeof(MessageHeader) + 3 * sizeof(float), &playerId, sizeof(unsigned long));
		memcpy(dataChunk.data() + sizeof(MessageHeader) + 3 * sizeof(float) + sizeof(unsigned long), name.c_str(), name.size());
		*reinterpret_cast<char*>(dataChunk.data() + sizeof(MessageHeader) + 3 * sizeof(float) + sizeof(unsigned long) + name.size()) = '\0';

		networkManager->sendToAll(dataChunk.data(), dataChunk.size());
	}
}


void GameController::sendEvent(sf::Event event)
{
	if (networkManager != nullptr)
	{
		std::vector<char> dataChunk(sizeof(MessageHeader) + sizeof(sf::Event) + sizeof(unsigned long));

		*reinterpret_cast<MessageHeader*>(dataChunk.data()) = MessageHeader::PLAYER_INPUT_EVENT;
		memcpy(dataChunk.data() + sizeof(MessageHeader), &event, sizeof(sf::Event));
		memcpy(dataChunk.data() + sizeof(MessageHeader) + sizeof(sf::Event), &playerId, sizeof(unsigned long));

		networkManager->sendToAll(dataChunk.data(), dataChunk.size());
	}
}


void GameController::sendSetRotation(float angle, unsigned long actorId)
{
	if (networkManager != nullptr)
	{
		std::vector<char> dataChunk(sizeof(MessageHeader) + sizeof(float) + sizeof(unsigned long));

		*reinterpret_cast<MessageHeader*>(dataChunk.data()) = MessageHeader::SET_ROTATION;
		memcpy(dataChunk.data() + sizeof(MessageHeader), &angle, sizeof(float));
		memcpy(dataChunk.data() + sizeof(MessageHeader) + sizeof(float), &actorId, sizeof(unsigned long));

		networkManager->sendToAll(dataChunk.data(), dataChunk.size());
	}
}


void GameController::sendSetPosition(sf::Vector2f position, unsigned long actorId)
{
	if (networkManager != nullptr)
	{
		std::vector<char> dataChunk(sizeof(MessageHeader) + sizeof(sf::Vector2f) + sizeof(unsigned long));

		*reinterpret_cast<MessageHeader*>(dataChunk.data()) = MessageHeader::SET_POSITION;
		memcpy(dataChunk.data() + sizeof(MessageHeader), &position, sizeof(sf::Vector2f));
		memcpy(dataChunk.data() + sizeof(MessageHeader) + sizeof(sf::Vector2f), &actorId, sizeof(unsigned long));

		networkManager->sendToAll(dataChunk.data(), dataChunk.size());
	}
}


void GameController::sendSetVelocity(sf::Vector2f velocity, unsigned long actorId)
{
	if (networkManager != nullptr)
	{
		std::vector<char> dataChunk(sizeof(MessageHeader) + sizeof(sf::Vector2f) + sizeof(unsigned long));

		*reinterpret_cast<MessageHeader*>(dataChunk.data()) = MessageHeader::SET_VELOCITY;
		memcpy(dataChunk.data() + sizeof(MessageHeader), &velocity, sizeof(sf::Vector2f));
		memcpy(dataChunk.data() + sizeof(MessageHeader) + sizeof(sf::Vector2f), &actorId, sizeof(unsigned long));

		networkManager->sendToAll(dataChunk.data(), dataChunk.size());
	}
}


void GameController::sendDestroyActor(unsigned long actorId)
{
	if (networkManager != nullptr)
	{
		std::vector<char> dataChunk(sizeof(MessageHeader) + sizeof(unsigned long));

		*reinterpret_cast<MessageHeader*>(dataChunk.data()) = MessageHeader::DESTROY_ACTOR;
		memcpy(dataChunk.data() + sizeof(MessageHeader), &actorId, sizeof(unsigned long));

		networkManager->sendToAll(dataChunk.data(), dataChunk.size());
	}
}


GameController::SynchronizationUpdate::SynchronizationUpdate(GameController *controller)
	: controller(controller)
{}


std::unique_ptr<CoroutineResultType> GameController::SynchronizationUpdate::operator()()
{
	coroutineBegin();
	while (true)
	{
		//printf("Co 20 milisekund\n");
		for (auto &ship : controller->playerShips)
		{
			auto shipLocked = ship.lock();
			if (!shipLocked)
				continue;

			controller->sendSetPosition(shipLocked->getLocalPosition(), shipLocked->getId());
			controller->sendSetRotation(shipLocked->getLocalRotation(), shipLocked->getId());

			auto component = shipLocked->getComponent<PlayerController>().lock();
			if (!component)
				continue;

			auto playerController = static_cast<PlayerController*>(component.get());
			controller->sendSetVelocity(playerController->velocity, shipLocked->getId());
		}
		yieldReturn(mkUniq(WaitForSeconds(0.020)));
	}
	coroutineEnd(mkUniq(CoroutineFinished()));
}