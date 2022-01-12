#include <cstdlib>
#include <ctime>
#include "NetworkManager.h"
#include "Game.h"
#include "GameController.h"
#include "Button.h"
#include "ThreadsafeDataQueue.h"


//std::list<Button> buttons;
//sf::Texture buttonTexture;


int main()
{
	/*buttonTexture.loadFromFile("button.png");
	buttons.push_back(Button(buttonTexture, 200, 150, 300, 123.5, 200, 100, Game::get().window, []()
	{
		MessageBoxA(0, "I was clicked!", "Click!", MB_OK);
	}));*/
	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		std::cerr << "WSAStartup failed: " << iResult << std::endl;
		return 1;
	}

	ThreadsafeDataQueue networkDataQueue;
	Network::NetworkManager networkManager;
	Game &game = Game::get();
	std::srand(static_cast<unsigned>(std::time(0)));

	// Spawn game controller actor
	auto gameActor = Actor::createActor(Game::get().getRootActor(), "gameActor");
	auto controllerPtr = gameActor->addComponent<GameController>();

	GameController *controller = static_cast<GameController*>(controllerPtr.lock().get());
	controller->setNetworkManager(&networkManager);
	controller->setDataQueue(&networkDataQueue);

	game.previousFrame = Game::steady_clock::now();
	while (game.window.isOpen())
	{
		// Update the time between last two frames
		game.deltaTime = static_cast<double>((Game::steady_clock::now() - game.previousFrame).count()) / Game::steady_clock::period::den;
		game.previousFrame = Game::steady_clock::now();

		// Handle incoming events
		sf::Event event;
		while (game.window.pollEvent(event))
		{
			switch (event.type)
			{
				case sf::Event::Closed:
					game.window.close();
					break;

				case sf::Event::Resized:
					{
						sf::View newView = game.window.getView();
						newView.setSize(static_cast<float>(event.size.width) * game.initialGameViewWidth / game.initialWindowWidth,
							static_cast<float>(event.size.height) * game.initialGameViewHeight / game.initialWindowHeight);
						game.window.setView(newView);
					}
					break;

				case sf::Event::KeyPressed:
				case sf::Event::KeyReleased:
					game.notifyScripts(&BehaviourScript::onKeyboardEvent, event);
					break;

				case sf::Event::MouseButtonPressed:
				case sf::Event::MouseButtonReleased:
				case sf::Event::MouseEntered:
				case sf::Event::MouseLeft:
				case sf::Event::MouseMoved:
				case sf::Event::MouseWheelMoved:
				case sf::Event::MouseWheelScrolled:
					game.notifyScripts(&BehaviourScript::onMouseEvent, event);
					if (event.type == sf::Event::MouseButtonPressed)
					{
						sf::Vector2f clickPos = game.window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y), game.window.getView());
						std::cout << Tools::toString(clickPos) << std::endl;
					}

					if (event.type != sf::Event::MouseButtonPressed)
						break;

					/*for (auto &button : buttons)
					{
						if (button.isClicked(event.mouseButton.x, event.mouseButton.y))
							button.click();
					}*/
					break;

				default:
					break;
			}
		}

		game.window.clear();

		game.drawActors();
		game.testCollisions();
		game.update();
		game.executeActorCoroutines();
		game.removeDestroyedActors();
		/*for (auto &button : buttons)
			button.draw();*/

		game.window.display();
	}

	WSACleanup();
	return 0;
}