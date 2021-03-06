﻿#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "client.h"
#include "../server.h"
#include "../client.h"
#include "../world.h"
#include "../messageprocessor.h"
#include "../../share/object.h"
#include "../../share/network/packet.h"
#include "../../share/system/log.h"
#include "../../share/system/copy.h"
#include "../../share/messages.h"


/*
╔══════════════════════════════════════════════════════════════╗
║ 	server messages processors 			                       ║
║ created by Dennis Yarikov						                       ║
║ jun 2016									                       ║
╚══════════════════════════════════════════════════════════════╝
*/

#define serverMessageProcessor(id)\
	messageprocessorServerAdd(id, (void*)&message ## id)

#define voidMessageProcessor(i)\
	static void *message ## i(server*s, packet* p){\
	return 0;\
}

using namespace share;

///get client attributes 
namespace master {

	///[nid, enemyid]
	static void* message_NPC_HURT(server *sv, packet *p){
		if (p->chanks.size()<2)
			return p;//strange
		//find npc and hurt it
		master::world.m.lock();
			try{
				npc *n=master::world.npcs.at(p->chanks[0].value.i);
				npc *e=master::world.npcs.at(p->chanks[1].value.i);
				n->m.lock();
					auto d=withLock(e->m, e->weapon.damage);
					n->hurt(d);
					n->damagers[e->id]+=d;
				n->m.unlock();
				printf("%d hurted by %d for %d (%d)\n", n->id, e->id, d, n->health);
			}catch(...){}
		master::world.m.unlock();
		return 0;
	}

	///[nid]
	static void* message_NPC_SUICIDE(server *sv, packet *p){
		if (p->chanks.size()<1)
			return p;//strange
		//find npc kill it
		master::world.m.lock();
			try{
				npc *n=master::world.npcs.at(p->chanks[0].value.i);
//				printf("inited? -> %d\n", n->inited);
				withLock(n->m, n->set_attr(n->health, 0));
			}catch(...){}
		master::world.m.unlock();
//		printf("npc %d suiside\n", n->id);
		return 0;
	}

	///[sid]
	static void* message_SERVER_READY(server *sv, packet *p_){
		master::world.m.lock();
			server::setPause(1);
				master::grid->add_server(sv->id);
			server::setPause(0);
		master::world.m.unlock();
		printf("server %d ready\n", sv->id);
		return 0;
	}

	static void* message_NPC_UPDATE(server *sv, packet* p){
		if (p->chanks.size()<1)
			return p;//strange
		int id=p->chanks[0].value.i;
		npc* n=0;
		master::world.m.lock();
			try{
				n=master::world.npcs.at(id);
				if (sv->id==n->slave_id || n->slave_id==0){ //update only by owner or new npc
					n->m.lock();
						n->update(p);
					n->m.unlock();
				}
			}catch(...){
				//slave can't create npc
			}
			
//			printf("updated %d\n", id);
		master::world.m.unlock();
		return 0;
	}
	
	///[id, angle]
	static void* message_NPC_MAKE_SHOT(server *sv, packet *p){
		if (p->chanks.size()<2)
			return p;//strange
		master::world.m.lock();
			try{
				npc *n=master::world.npcs.at(p->chanks[0].value.i);
				auto c=master::world.maps.at(n->map_id)->cells(n->position);
				if ([&]()->bool{
					for(auto&& sf:c->safezones){
						if (sf->contains(n->position))
							return 0;
					}
					return 1;
				}()){//can't shoot in safezones
					n->m.lock();
						npc *nn = new npc(n->world, n->world->getId());
						//set params
						nn->angle=p->chanks[1].value.c;
						nn->direction.by_angle(nn->angle, 1); //right dir and full speed
	//					printf("%g %g \n", nn->direction.x, nn->direction.y);
						nn->position=n->position+point::from_angle(nn->angle, n->r*2);//+copy(n->weapon.bullet_offset).rotate(nn->angle);
						//correction for gun offset
						point bpoint=n->weapon.bullet_offset;
						bpoint.rotate(nn->angle);
					//	printf("bullet offset %g %g -> %g %g\n", n->weapon.bullet_offset.x, n->weapon.bullet_offset.y, bpoint.x, bpoint.y);
						nn->position+=bpoint;
						nn->state=STATE_ATTACK;//attacking on every tick
						nn->bot.owner_id=n->id;
						nn->weapon_id=n->bullet_id;//TODO:change to base id
						nn->body_id=0;//TODO:change
						
						nn->recalculate_type();
						if (nn->_health==0){
							nn->_health=1;
						}
						nn->restore_attrs();
	//					try{ nn->apply(share::object::all.at(n->bullet_id)); }catch(...){}
						
						nn->weapon.ricochet=n->weapon.ricochet;
						nn->weapon.damage=n->weapon.damage;
						nn->weapon.dist=n->weapon.dist; //set max move dist
						nn->weapon.attacks=n->weapon.attacks; //set max targets
						nn->weapon.next_shot=0;//shoot every tick
						nn->weapon.latency=0;//shoot every tick
						nn->attackable=n->weapon.attackable;
						
						nn->type=2;//TODO: do not use for srite
						nn->move_id=1;//TODO: change to choose bullet move id 
						nn->shoot_id=2;//TODO: change to choose bullet shoot id 
					n->m.unlock();
					master::world.npcs_m.lock();
						master::world.new_npcs.push_back(nn);
					master::world.npcs_m.unlock();
				}else{
					printf("npc %d try to shoot in safezone\n", n->id);
				}
			}catch(...){}
		master::world.m.unlock();
		return 0;
	}


//	voidMessageProcessor(1)


	void serverMessageProcessorInit(){
//		serverMessageProcessor(1);
		messageprocessorServerAdd(MESSAGE_NPC_HURT, (void*)&message_NPC_HURT);
		messageprocessorServerAdd(MESSAGE_NPC_SUICIDE, (void*)&message_NPC_SUICIDE);
		messageprocessorServerAdd(MESSAGE_SERVER_READY, (void*)&message_SERVER_READY);
		messageprocessorServerAdd(MESSAGE_NPC_UPDATE, (void*)&message_NPC_UPDATE);
		messageprocessorServerAdd(MESSAGE_NPC_MAKE_SHOT, (void*)&message_NPC_MAKE_SHOT);
	}
}
