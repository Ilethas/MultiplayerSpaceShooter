#include "NetworkManager.h"


namespace Network
{
	void NetworkManager::connectionListenerTask()
	{
		while (true)
		{
			std::unique_lock<std::mutex> managerLock(managerBlockade);
			if (listeningAsServer == false)
				break;
			managerLock.unlock();

			// Accept a client socket
			SOCKET connectionSocket = accept(managedSocket, nullptr, nullptr);
			if (connectionSocket != INVALID_SOCKET)
			{
				managerLock.lock();
				connections.push_back(std::make_unique<Connection>());
				auto &newConnection = *connections.back();

				newConnection.socket = connectionSocket;
				newConnection.identifier = IdSeed++;
				newConnection.receiverThread = std::thread(&NetworkManager::receiverTask, this, &newConnection);
				newConnection.self = std::prev(connections.end());
				newConnection.receiverThread.detach();
				managerLock.unlock();

				if (onConnect != nullptr)
					onConnect(newConnection.identifier);
			}
		}
	}


	void NetworkManager::receiverTask(Connection *connection)
	{
		char buffer[200];
		int bufferLength = 200;
		int iResult = 0;

		do
		{
			iResult = recv(connection->socket, buffer, bufferLength, 0);
			if (iResult > 0)
			{
				std::unique_lock<std::mutex> managerLock(managerBlockade);
				if (onReceive != nullptr)
					onReceive(connection->identifier, buffer, iResult);
			}

		} while (iResult > 0);

		std::unique_lock<std::mutex> managerLock(managerBlockade);
		closesocket(connection->socket);
		connections.erase(connection->self);
		managerLock.unlock();

		if (onDisconnect != nullptr)
			onDisconnect(connection->identifier);

		cleanupConditional.notify_all();
	}


	NetworkManager::~NetworkManager()
	{
		stopListening();
		distonnectAll();

		std::unique_lock<std::mutex> managerLock(managerBlockade);
		while (connections.empty() == false)
			cleanupConditional.wait(managerLock);
	}


	void NetworkManager::setOnConnect(std::function<void(unsigned long)> callback)
	{
		std::unique_lock<std::mutex> managerLock(managerBlockade);
		onConnect = callback;
	}


	void NetworkManager::setOnDisconnect(std::function<void(unsigned long)> callback)
	{
		std::unique_lock<std::mutex> managerLock(managerBlockade);
		onDisconnect = callback;
	}


	void NetworkManager::setOnReceive(std::function<void(unsigned long, const char*, int)> callback)
	{
		std::unique_lock<std::mutex> managerLock(managerBlockade);
		onReceive = callback;
	}


	void NetworkManager::startListening(std::string port)
	{
		std::unique_lock<std::mutex> managerLock(managerBlockade);
		if (listeningAsServer == true)
			return;
		managerLock.unlock();

		addrinfo *ptr = nullptr, hints;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		// Resolve the local address and port to be used by the server
		int iResult = getaddrinfo(nullptr, port.c_str(), &hints, &result);
		if (iResult != 0)
		{
			throw std::runtime_error("getaddrinfo failed: " + std::to_string(iResult));
		}

		// Create a SOCKET for connecting to server
		managedSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (managedSocket == INVALID_SOCKET)
		{
			freeaddrinfo(result);
			throw std::runtime_error("Error at socket(): " + std::to_string(WSAGetLastError()));
		}

		// Setup the TCP listening socket
		iResult = bind(managedSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			freeaddrinfo(result);
			closesocket(managedSocket);
			managedSocket = INVALID_SOCKET;
			throw std::runtime_error("bind failed with error: " + std::to_string(WSAGetLastError()));
		}

		iResult = listen(managedSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR)
		{
			closesocket(managedSocket);
			managedSocket = INVALID_SOCKET;
			throw std::runtime_error("listen failed with error: " + std::to_string(WSAGetLastError()));
		}

		managerLock.lock();
		listeningAsServer = true;
		managerLock.unlock();

		connectionListenerThread = std::thread(&NetworkManager::connectionListenerTask, this);
	}


	void NetworkManager::stopListening()
	{
		std::unique_lock<std::mutex> managerLock(managerBlockade);
		if (listeningAsServer == false)
			return;

		closesocket(managedSocket);
		freeaddrinfo(result);
		managedSocket = INVALID_SOCKET;

		listeningAsServer = false;
		managerLock.unlock();

		connectionListenerThread.join();
	}


	void NetworkManager::connectAsClient(std::string server, std::string port)
	{
		std::unique_lock<std::mutex> managerLock(managerBlockade);
		if (listeningAsServer == true)
			return;
		managerLock.unlock();

		addrinfo *ptr = nullptr, hints;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		// Resolve the server address and port
		int iResult = getaddrinfo(server.c_str(), port.c_str(), &hints, &result);
		if (iResult != 0)
			throw std::runtime_error("getaddrinfo failed with error: " + std::to_string(iResult));

		// Attempt to connect to an address until one succeeds
		bool succeedToConnect = false;
		SOCKET clientSocket = INVALID_SOCKET;
		for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
		{
			// Create a SOCKET for connecting to server
			clientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
			if (clientSocket == INVALID_SOCKET)
				throw std::runtime_error("socket failed with error: " + std::to_string(WSAGetLastError()));

			// Connect to server
			iResult = connect(clientSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (iResult == SOCKET_ERROR)
			{
				closesocket(clientSocket);
				clientSocket = INVALID_SOCKET;
				continue;
			}

			succeedToConnect = true;
			break;
		}

		freeaddrinfo(result);
		if (succeedToConnect == false)
			throw std::runtime_error("failed to connect with server " + server + " on port " + port);

		managerLock.lock();
		connections.push_back(std::make_unique<Connection>());
		auto &newConnection = *connections.back();

		newConnection.socket = clientSocket;
		newConnection.identifier = IdSeed++;
		newConnection.receiverThread = std::thread(&NetworkManager::receiverTask, this, &newConnection);
		newConnection.self = std::prev(connections.end());
		newConnection.receiverThread.detach();

		if (onConnect != nullptr)
			onConnect(newConnection.identifier);
	}


	void NetworkManager::disconnect(unsigned long connectionId)
	{
		std::unique_lock<std::mutex> managerLock(managerBlockade);
		for (auto &connection : connections)
		{
			if (connection->identifier)
			{
				closesocket(connection->socket);
				return;
			}
		}
	}


	void NetworkManager::distonnectAll()
	{
		std::unique_lock<std::mutex> managerLock(managerBlockade);

		for (auto &connection : connections)
			closesocket(connection->socket);
		IdSeed = 0;

		managerLock.unlock();
	}


	std::list<unsigned long> NetworkManager::getConnections() const
	{
		std::list<unsigned long> result;
		std::unique_lock<std::mutex> managerLock(managerBlockade);

		for (auto &connection : connections)
			result.push_back(connection->identifier);

		return result;
	}


	std::size_t NetworkManager::getConnectionsCount() const
	{
		std::unique_lock<std::mutex> managerLock(managerBlockade);
		return connections.size();
	}


	void NetworkManager::send(unsigned long connectionId, void *data, std::size_t dataSize)
	{
		std::unique_lock<std::mutex> managerLock(managerBlockade);
		for (auto &connection : connections)
		{
			if (connection->identifier == connectionId)
			{
				int iResult = ::send(connection->socket, static_cast<const char*>(data), dataSize, 0);
				return;
			}
		}
	}


	void NetworkManager::sendToAll(void *data, std::size_t dataSize)
	{
		std::unique_lock<std::mutex> managerLock(managerBlockade);
		for (auto &connection : connections)
		{
			int iResult = ::send(connection->socket, static_cast<const char*>(data), dataSize, 0);
		}
	}
}