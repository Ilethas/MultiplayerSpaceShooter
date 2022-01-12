#ifndef GAME_H_
#define GAME_H_
#include <SFML/Graphics.hpp>
#include <Windows.h>
#include <functional>
#include <chrono>
#include <string>
#include <future>
#include <iostream>
#include "Tools.h"
#include "Actor.h"
#include "Collider.h"
#include "BehaviourScript.h"


class Game
{
	private:
		std::shared_ptr<Actor> actorRoot = Actor::createActor(std::weak_ptr<Actor>());
		Game();

		// Collision management
		using Collision = std::pair<Actor*, Actor*>;

		struct CollThreadData
		{
			CollThreadData();
			CollThreadData(const CollThreadData &colThreadData);

			bool dataReady = false;
			std::mutex dataBlockade;
			std::condition_variable dataAvailability;

			std::list<Collision> resultList;
			std::list<std::shared_ptr<Actor>>::const_iterator firstActor;
			int actorsToTest;
		};

		std::mutex stopConditionBlocade;
		bool stopCollisionThreads = false;

		std::vector<CollThreadData> perThreadCollisions;
		std::vector<std::thread> threads;

		void initCollision();
		void perThreadTest(CollThreadData &collThreadData);

	public:
		~Game();
		using steady_clock = std::chrono::steady_clock;

		//===== Globals
		const unsigned int initialWindowWidth;
		const unsigned int initialWindowHeight;
		const unsigned int initialGameViewWidth;
		const unsigned int initialGameViewHeight;

		// Used to make the game flow independet from the CPU speed
		sf::RenderWindow window;
		steady_clock::time_point previousFrame;
		double deltaTime = 0.0;

		// Game state
		bool drawColliders = false;

		// Game resources

		struct TexturesStruct
		{
			sf::Texture starsSmall;
			sf::Texture starsMedium;
			sf::Texture starsLarge;

			sf::Texture dust;
			sf::Texture blueDust;
			sf::Texture violetDust;
			sf::Texture yellowDust;

			sf::Texture hotNebula;
			sf::Texture coldNebula;

			sf::Texture exoplanet;
			sf::Texture exoplanet2;
			sf::Texture exoplanet3;
			sf::Texture iceGiant;
			sf::Texture gasGiant;

			sf::Texture playerShip1;
			sf::Texture engineJet1;
			sf::Texture playerShipEngine;

			sf::Texture plasmaFire;
			sf::Texture plasmaProjectile;
			sf::Texture plasmaSplash;
		} textures;

		static Game& get();
		void loadTextures(sf::Texture &texture, const std::string &path);

		void setView(const sf::Vector2f &center, unsigned int newWidth, unsigned int newHeight);
		sf::View fitViewIn(const sf::View &view, float newWidth, float newHeight);

		void update();
		void notifyScripts(void(BehaviourScript::*notifyMethod)(sf::Event event), sf::Event event);
		void drawActors();
		void executeActorCoroutines();
		void removeDestroyedActors();
		void testCollisions();

		std::weak_ptr<Actor> getRootActor();
};


#endif