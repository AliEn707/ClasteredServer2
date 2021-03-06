#include <string>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "socketworkers.h"
#include "../../share/system/log.h"
#include "../client.h"
#include "../server.h"

/*
╔══════════════════════════════════════════════════════════════╗
║ workers for clients 			                       ║
║ created by Dennis Yarikov						                       ║
║ jun 2016									                       ║
╚══════════════════════════════════════════════════════════════╝
*/

#define MAX_WORKERS 400

using namespace share;

namespace master {
	
	std::map<int, socketworkers*> socketworkers::all;
	int socketworkers::checks=10;

	void socketworkers::init(){
		//add some actions for every work element
		printf("%s created\n",name.data());
	}

	void socketworkers::loop(){
		//add some actions for every iteration
	}

/*
	static void* clientMessageEach(void* d, void * _arg){
		client_message* m=d;
		voidp2_t *arg=_arg;
		client *c=arg->p1;
		packet *p=arg->p2;
		t_mutexLock(m->mutex);
		if (m->ready){
			t_mutexUnlock(m->mutex);
			packetInitFast(p);
			packetAddData(p, m->data,m->$data);
			packetSend(p, c->sock);
			clientMessageClear(m);
			return d;
		}
		t_mutexUnlock(m->mutex);
		return 0;
	}
*/
	void* socketworkers::proceed(client *c){
		int i;
		packet p;
//		clientMessagesProceed(c, clientMessageEach, &wd->packet);
		c->messages_proceed();
		for(i=0;i<checks;i++){
			if (c->sock->recv_check()){//WHY it doesn't enter here?
//				printf("can read\n");
				c->timestamp=share::time(0);
				do{
					if (c->sock->recv(&p)){
//						printf("(client) got message %d\n", ((char*)p.data())[0]);
						if (c->proceed(&p)==0)
							break;
					}
					c->mutex.lock();
						c->broken=1;
						c->sock->close();//check if need it
					c->mutex.unlock();
					if (c->id==0)
						delete c;
					printf("error with client\n");
					return c;
				}while(0);
			}else{
//				printf("can't read\n");
				break;
			}
		}
		if (i>=checks)
			recheck=1;
		return 0;
	}

	void* socketworkers::close(){
		//worker* w=_w;
		return 0;
	}

	#define socketworkersActionAll(action)\
		void socketworkers::action ## All(){\
			for(auto w:all)\
				w.second->action();\
		}

	socketworkersActionAll(start)
	socketworkersActionAll(stop)
	socketworkersActionAll(pause)
	socketworkersActionAll(unpause)

	#define socketworkersAction(action)\
		void socketworkers::action(int n){\
			if (n<(int)all.size() && n>=0)\
				all[n]->action();\
		}

	socketworkersAction(start)
	socketworkersAction(stop)
	socketworkersAction(pause)
	socketworkersAction(unpause)

	void socketworkers::addWork(int n, client *work){
		if (n<(int)all.size())
			all[n]->add_work(work);
	}

	int socketworkers::addWorkAuto(client *work){
		unsigned i;
		int n=0;
		if (all.size()==0)
			return -1;
		unsigned $works=all[n]->works.size();
		for(i=0;i<all.size();i++){
			if (all[i]->works.size()<$works){
				$works=all[i]->works.size();
				n=i;
			}
		}
		all[n]->add_work(work);
		return n;
	}

	int socketworkers::create(int num, int TPS){
		char str[100];
		for(int i=0;i<num;i++){
			sprintf(str, "Client worker %d", i);
			all[i]=new socketworkers(i, TPS, std::string(str));
		}
		return 0;
	}
}