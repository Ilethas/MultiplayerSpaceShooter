#ifndef NETWORK_MANAGER_H_
#define NETWORK_MANAGER_H_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "Ws2_32.lib")

#include <string>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <list>
#include <functional>


namespace Network
{
	class NetworkManager
	{
		private:
			struct Connection;
			unsigned long IdSeed = 0;

			addrinfo *result = nullptr;
			SOCKET managedSocket = INVALID_SOCKET;

			mutable std::mutex managerBlockade;
			mutable std::condition_variable cleanupConditional;
			std::list<std::unique_ptr<Connection>> connections;
			bool listeningAsServer = false;

			std::function<void(unsigned long)> onConnect = nullptr;
			std::function<void(unsigned long)> onDisconnect = nullptr;
			std::function<void(unsigned long, const char*, int)> onReceive = nullptr;
			
			std::thread connectionListenerThread;
			void connectionListenerTask();
			void receiverTask(Connection *connection);

		public:
			~NetworkManager();

			void setOnConnect(std::function<void(unsigned long)> callback);
			void setOnDisconnect(std::function<void(unsigned long)> callback);
			void setOnReceive(std::function<void(unsigned long, const char*, int)> callback);

			void startListening(std::string port);
			void stopListening();
			void connectAsClient(std::string server, std::string port);

			void disconnect(unsigned long connectionId);
			void distonnectAll();

			std::list<unsigned long> getConnections() const;
			std::size_t getConnectionsCount() const;

			void send(unsigned long connectionId, void *data, std::size_t dataSize);
			void sendToAll(void *data, std::size_t dataSize);
	};


	struct NetworkManager::Connection
	{
		unsigned long identifier;
		SOCKET socket;
		std::thread receiverThread;
		std::list<std::unique_ptr<Connection>>::iterator self;
	};
}


#endif