﻿#include <string.h>

#include <cstdlib>

#include "client.h"
#include "world.h"
#include "server.h"
#include "storage.h"
#include "messageprocessor.h"
#include "../share/network/socket.h"
#include "../share/network/packet.h"
#include "../share/crypt/base64.h"
#include "../share/system/log.h"

/*
╔══════════════════════════════════════════════════════════════╗
║ 	implementation of connected clients 			                       ║
║ created by Dennis Yarikov						                       ║
║ jun 2016									                       ║
╚══════════════════════════════════════════════════════════════╝
*/

using namespace share;

namespace master {

	typedef void*(*client_processor)(client*, packet*);

	std::unordered_map<int, client*> client::all;
	share::mutex client::m;



	client_message::client_message(void* buf, short size):ready(0){
		$data=size;
		if ((data=(char*)malloc(sizeof(*data)*($data+1)))==0){
			perror("malloc");
		}
		memcpy(data, buf, $data);
		data[$data]=0;
		//packetAddData(&m->packet,buf,size);
	}

	client_message::~client_message(){
		withLock(mutex, num--);
		if (withLock(mutex, num==0)){	
			free(data);
		}
	}

	client::client(socket *sock):
		id(0),	
		broken(0),
		server_id(0),
		sock(sock),
		npc_id(0),
		map_id(-1), //it must be updated on first npc check
		connect_time(master::world.mtime),
		timestamp(0)
	{
		name[0]=0;
		login[0]=0;
		passwd[0]=0;
		view_position[0]=0;
		view_position[1]=0;
		view_area[0]=400;
		view_area[1]=400;
//		npc=new share::npc(&master::world, master::world.getId());
	}
	
	client::~client(){
		mutex.lock();
			if (sock)
				delete sock;
			for (auto &&mes:messages){
				delete mes;
			}
			master::world.m.lock();
				try{
					npc *n=master::world.npcs.at(npc_id);
					n->world->npcs.erase(n->id);
					withLock(n->m, n->owner_id=0);//set to not respawn
					n->remove();
				}catch(...){}
			master::world.m.unlock();
		mutex.unlock();
	}

	int client::add(client* c){
		if (c){
			if (c->id!=0){
				m.lock();
					all[c->id]=c;
				m.unlock();
			}
			return c->id;
		}
		return 0;
	}

	client* client::get(int id){
		client *c=0;
		m.lock();
			try{
				c=all.at(id);
				if (c->broken)
					c=0;
			}catch(...){}
		m.unlock();
		return c;
	}

	void client::check(){
		std::list<client*> l;
		m.lock();
			for (auto &&c:all){
				if (withLock(c.second->mutex, c.second->broken) || c.second->id==0){
					if (abs(share::time(0)-c.second->timestamp)>=0){//add 10 seconds for reconnect
						l.push_back(c.second);
					}
				}
			}
			for (auto &&c:l){
				printf("client %d removed\n", c->id);
				all.erase(c->id);
				delete c;
			}
//			bintreeForEach(&clients, checkC, &list);
//			worklistForEachRemove(&list, removeC, 0);
		m.unlock();
	}

	void client::remove(client* c){
		if (c){
			m.lock();
				all.erase(c->id);
				delete c;
			m.unlock();
		}
	}

	int client::proceed(packet *p){
		char* buf=(char*)p->data();
		client_processor processor;
		//void*(*processor)(packet*);
	//	printf("got message %d\n", *buf);
		if ((processor=(client_processor)messageprocessorClient(*buf))!=0){
			return processor(this, p)!=0;
		}
		return 0;
	}

	void client::messages_add(client_message *mes){
		if (mes){
			mes->num=1;
			mes->ready=1;
			mutex.lock();
				messages.push_back(mes);
			mutex.unlock();
				//find client and add
		}
	}
	
	void client::broadcast(client_message *mes){
		if (mes){
			//add to all, and then
			m.lock();
				for (auto &&i:all){
					client *c=i.second;
					c->mutex.lock();
						c->messages.push_back(mes);
						mes->mutex.lock();
							mes->num++;
						mes->mutex.unlock();
					c->mutex.unlock();
				}
//				bintreeForEach(&clients, clientAddEach, m);
				mes->mutex.lock();
					mes->ready=1;
				mes->mutex.unlock();
			m.unlock();
		}
	}

	void client::messages_proceed(){
		mutex.lock();
			for (auto i=messages.begin(), end=messages.end();i!=end;){
				client_message *m=*i;
				if (withLock(m->mutex, m->ready)){
					packet p;
					p.init(m->data, m->$data);
					sock->send(&p);
					messages.erase(i++);
					delete m;
				}else{
					i++;
				}
			}
		mutex.unlock();
	}

	
	int client::set_info(user_info *u){
		id=u->id;
		sprintf(name,"%s",u->name);
		sprintf(login,"%s",u->login);
		sprintf(passwd,"%s",u->passwd);
//		share::base64::decode((unsigned char*)u->passwd, (unsigned char*)passwd, strlen(u->passwd));
		//add other
		return 0;
	}

	void client::server_clear(){
		withLock(mutex, server_id=0);
	}
	
}
