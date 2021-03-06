#include <cstdlib>

#include "shoots.h"
#include "../npc.h"
#include "../world.h"

namespace share{
	
	typeof(npc::shoots) npc::shoots;
	npc_shoots npc::_shoots;

#define addShootFunc(id)\
	shoots[id]=(shoot_func)&npc_shoots::shoot ## id;
	
	npc_shoots::npc_shoots(){
		addShootFunc(0);
		addShootFunc(1);
		addShootFunc(2);
	}
#undef addShootFunc

#define attackCheck(n)\
		(n->attackable && n->id!=id && n->id!=bot.owner_id)//check if it needs n &&
//		(n && n->attackable && n->id!=id && n->id!=bot.owner_id)

#define attack(n, n0, a)\
	do{\
		n->hurt(n0);\
		if (a--==0)\
			break;\
	}while(0)\
	
	//melee shot
	void npc_shoots::shoot0(typeof(point::x) x, typeof(point::y) y){
		do_on_map([&](map *m)->int{
			//init vars
			short ang_diap=weapon.ang_diap;//pdegree
			short ang_shift=weapon.ang_shift;//pdegree
			short attacks=weapon.attacks;
			int dist=weapon.dist;

			short ang=angle+ang_shift;

			//for cells
			std::unordered_set<npc*> npcs;
			auto cells=m->cells(position, dist);
			for(auto c: cells){
				auto cell=m->cells(c);
				for(auto n: cell->npcs)
					if (attackCheck(n.second)){
	//					printf("added %d !=%d\n", n.second->id, id);
						npcs.insert(n.second);
					}
			}
			switch(ang_diap){
				case 0:{
					segment s(position, position+point::from_angle(ang, dist));//center
					for(auto n: npcs)
						if (s.distanse(n->position)<=n->r) //check inside circle
							attack(n, this, attacks);
					break;
				}
				case 240:{
					for(auto n: npcs)
						if (position.distanse2(n->position)<=sqr(dist+n->r)) //check near segment
							attack(n, this, attacks);
					break;
				}
				default:{
					segment sr(position, position+point::from_angle(ang-ang_diap/2, dist));//right
					segment sl(position+point::from_angle(ang+ang_diap/2, dist), position);//left
					for(auto n: npcs)
						if (position.distanse2(n->position)<=sqr(dist+n->r) &&
								sr.signed_area2(n->position)>0 && 
								sl.signed_area2(n->position)>0) //check in sector
							attack(n, this, attacks);
					break;
				}
			}
			return 0;
		});
	}
#undef attack
	
	//spawn bullet npcs
	void npc_shoots::shoot1(typeof(point::x) x, typeof(point::y) y){
		//ask master to spawn
		short ang_diap=weapon.ang_diap; //pdegree
		short ang_shift=weapon.ang_shift; //pdegree
		short attacks=weapon.attacks;
		for (auto i=0;i<attacks;i++){
			short ang_=ang_diap?((rand()%ang_diap)-ang_diap/2):0; //calc random angle in diap
			short ang=((short)angle+ang_shift)+ang_;
			while(ang<-PPI)
				ang=PPI*2+ang;
			while(ang>PPI)
				ang=PPI*2-ang;
//			printf("angle %d (%d %d) shot %d\n", angle, (short)angle+ang_shift, ang_, ang);
			make_shot((char)(ang)); //TODO: check 
		}
	}
	
	//bullet shot
	//if enemy near self, attack it than suicide
	//bullet r is attack dist
	//bullet weapon.dist is  move dist
	void npc_shoots::shoot2(typeof(point::x) x, typeof(point::y) y){
		do_on_map([&](map *m)->int{
			short attacks=weapon.attacks;
			std::unordered_set<npc*> npcs;
			auto cells=m->cells(position, 2*r+vel);
			for(auto c: cells){
				auto cell=m->cells(c);
				for(auto n: cell->npcs)
					if (attackCheck(n.second)){//macro
	//					printf("added %d !=%d\n", n.second->id, id);
						npcs.insert(n.second);
					}
			}
			segment s(position, position+point::from_angle(angle, -vel));//way passed on last step 
			for(auto n: npcs){
				if(s.distanse(n->position)<=r+n->r){//TODO:check
					n->hurt(this);//hurt if touch
					if(--attacks==0){
						suicide();
						state=STATE_IDLE;//do not attack more
						break;
					}
				}
			}
			return 0;
		});
	}
	
}
