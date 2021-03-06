#include <cstdlib>

#include "moves.h"
#include "../npc.h"
#include "../world.h"

namespace share{
	
	typeof(npc::moves) npc::moves;
	npc_moves npc::_moves;
	
#define addMoveFunc(id)\
	moves[id]=(move_func)&npc_moves::move ## id;
	
	npc_moves::npc_moves(){
		addMoveFunc(0);
		addMoveFunc(1);
		addMoveFunc(2);
	}
#undef addMoveFunc
	
	//common move
	void npc_moves::move0(typeof(point::x) x, typeof(point::y) y){
		char moved=0;
		auto $=[&](point &p, segment *s)->bool{//stronger check
			return (s->distanse(p)<=r || segment(position, p).cross(s));//if cross need to return false from check_point
		};
		if (x!=0)
			if (check_point(position.x+x, position.y, $)){
				set_attr(position.x, position.x+x);
				moved++;
			}
		if (y!=0)
			if (check_point(position.x, position.y+y, $)){
				set_attr(position.y, position.y+y);
				moved++;
			}
		
		if (bot.used){
//			printf("bot %d on %g %g -> %g %g\n", id, position.x, position.y, bot.goal.x, bot.goal.y);
			if (!moved || position.distanse2(bot.goal)<=3*vel){//for testing
				do_on_map([&](map* m){
					bot.goal.x=(rand()%(((int)m->map_size[0]-20)*100))/100.0+10;
					bot.goal.y=(rand()%(((int)m->map_size[1]-20)*100))/100.0+10;
	//				printf("new goal on %d -> %g %g\n", id, bot.goal.x, bot.goal.y);
					set_dir();
					return 0;
				});
			}
		}
//		update_cells();
	}
	
	//bullet move, go straight for dist, enemy or wall, than suicide
	void npc_moves::move1(typeof(point::x) x, typeof(point::y) y){
		//it is not bot (for disable respawn)
		if (x!=0 || y!=0){
			if (
				(bot.dist<weapon.dist || weapon.dist<=0) &&
				[&]()->bool{
					try{
						for(auto&& sf:world->maps.at(map_id)->cells(position)->safezones){
							if (sf->contains(position))
								return 0;
						}
					}catch(...){}
					return 1;
				}()//if bullet in safezone it must be removed
			){
				if (check_point(position.x+x,position.y+y, [&](point &p, segment *s)->bool{//we don't check for bullet radius
					if (segment(position, p).cross(s)!=0){
						if (randInPercent(weapon.ricochet)){
							//add richochet move
							char half_angle=0;
							auto $=segment(position,position+point(x,y)).mirror_by(*s, half_angle);
							if (half_angle){
								set_attr(position.x, $.x);
								set_attr(position.y, $.y);
								direction.rotate(half_angle).rotate(half_angle);
								set_attr(direction.x, direction.x);
								set_attr(direction.y, direction.y);
								set_attr(angle, direction.to_angle());
								set_attr(bot.dist, bot.dist+vel*2);//richochet needed more energy
							}
						}else{
							suicide();
						}
						return 1;//if cross need to return false from check_point
					}
					return 0;
				})){
	//				point p=position; 
					set_attr(position.x, position.x+x);
					set_attr(position.y, position.y+y);
					set_attr(bot.dist, bot.dist+vel);//usually in full speed//p.distanse(position);
				} 
				return;
			}
			suicide();//suicide
		}
//		printf("%d ?< %d\n", bot.dist, weapon.dist);
	}
	
	//stiky move (move to the same position as target npc)
	void npc_moves::move2(typeof(point::x) x, typeof(point::y) y){
		//char angle=point(1,2).to_angle();
		//can't move independently
		if (bot.target){
			if (bot.target->attrs[bot.target->attr(&bot.target->position.x)]){
				set_attr(position.x, bot.target->position.x);
			}
			if (bot.target->attrs[bot.target->attr(&bot.target->position.y)]){
				set_attr(position.y, bot.target->position.y);
			}
			if (bot.target->attrs[bot.target->attr(&bot.target->angle)]){
				set_attr(angle, bot.target->angle);
			}
		}
	}
}
