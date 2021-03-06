#ifndef SERVER_HEADER
#define SERVER_HEADER

#include <map>
#include <string>

#include "client.h"
#include "../share/system/mutex.h"
#include "../share/network/socket.h"
#include "../share/network/packet.h"

#define MSG_SERVER 2

namespace master { 
	class server{
		public:
			int id;
			bool checked;
			bool ready;
			bool remote;
			share::socket* sock;
			std::string host;
			int port;
			share::mutex mutex;
			
			server(share::socket* sock, std::string host, int port);
			~server();
			void proceed(share::packet* p);
			void set_ready();
		
			static std::map<int, server*> all;
			static share::mutex m;
			
			static server* create(std::string host, int port);
			static int add(server* s);
			static server* get(int id);
			static void remove(server* s);
			static void check();
			static void sendAll(share::packet* p);
			static int idByAddress(std::string s, int port);
			static void setPause(bool on);
	};
/*
//initialization
void serversInit();
void serversClear();

//connect to specified server
server *serverNew(char* host, short port);
void serverClear(server* s);

//work with servers container
int serversAdd(server* s);
server *serversGet(int id);
void serversRemove(server* s);
void serversCheck();
//void serversForEach(void*(*f)(bintree_key k, void *v, void *arg), void *a);

short serversTotal();
void serversTotalInc();
void serversTotalDec();

//get id of less busy server
int serversGetIdAuto();

//processor for packet from server
void serverPacketProceed(server* s, packet* p);
void serversPacketSendAll(packet* p);

//create uniq id by server address and port
int serverIdByAddress(char* address, short port); 

//
int serverClientsAdd(server *s, void* c);
void* serverClientsGet(server *s, int id);
int serverClientsRemove(server *s, void* c);
void serverClientsErase(server *s);

void serverSetReady(server *s);
*/
}
#endif
