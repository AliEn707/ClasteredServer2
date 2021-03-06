#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <math.h>

#include "map.h"
//#include "system/log.h"
#include "crypt/crc32.h"
#include "NLTmxMap/NLTmxMap.h"

namespace share{

	static void* loadFile( const char * filename, bool appendNull ) {
    
		FILE* f = fopen( filename, "r" );
		if ( !f ) {
			return 0;
		}
		
		fseek( f, 0, SEEK_END );
		int length = ftell( f ) + appendNull;
		fseek( f, 0, SEEK_SET );
		
		void* buffer = malloc( length );
		fread( buffer, length, 1, f );
		fclose( f );
		
		if ( appendNull ) {
			((char*)buffer)[ length-1 ] = 0;
		}
		
		return buffer;
	}
	
	map::map(char* path): offset(50){
		name=std::string(strrchr(path,'/')+1,strlen(path)-(int)(strrchr(path,'.')-path)-1);
		id=getId((char*)name.data());
		cell.x=30;//25;
		cell.y=30;//25;
//		grid=0;
		map_size[0]=200;
		map_size[1]=200;
		clean_segments();
		reconfigure(path);
	}
	
	map::~map(){
//		if (grid)
//			delete[] grid;
		grid.resize(0);
		clean_segments();
	}
	
	void map::clean_segments(){
		for(int i=0, size=segments.size();i<size;i++){
			delete segments[i];
		}
		segments.clear();
	}
	
	void map::reconfigure(char* path){
		char * xml = (char*) loadFile( path, true );
		if (xml){
			try{
				NLTmxMap* map = NLLoadTmxMap( xml );
				//fill data
				try{cell.x=atoi(map->properties.at("cell_x").c_str());}catch(...){}
				try{cell.y=atoi(map->properties.at("cell_y").c_str());}catch(...){}
				map_size[0]=map->tileWidth*map->width;
				map_size[1]=map->tileHeight*map->height;
				for(auto group: map->groups){
					if (group->name==std::string("collision")){
						for(auto obj: group->objects){
							if (obj->type==OBJECT_QUAD){
								segments.push_back(new segment(obj->x, obj->y, obj->x+obj->width, obj->y));
								segments.push_back(new segment(obj->x+obj->width, obj->y, obj->x+obj->width, obj->y+obj->height));
								segments.push_back(new segment(obj->x+obj->width, obj->y+obj->height, obj->x, obj->y+obj->height));
								segments.push_back(new segment(obj->x, obj->y+obj->height, obj->x, obj->y));
							}else{
								for(int i=1, end=obj->points.size();i<end;i++){
									auto s = new segment(obj->x+obj->points[i-1].x, obj->y+obj->points[i-1].y, obj->x+obj->points[i].x, obj->y+obj->points[i].y);
									if (obj->properties["directed"]=="true")
										s->directed=1;
									segments.push_back(s);
								}
							}
						}
					}else if (group->name==std::string("safezones")){
						for(auto obj: group->objects){
							if (obj->type==OBJECT_QUAD){
								safezones[obj->gid]=quad(obj->x, obj->y, obj->width, obj->height);
							}
						}
					}else if (group->name==std::string("portals")){
						for(auto obj: group->objects){
							if (obj->type==OBJECT_QUAD){
								portal* p=new portal();
								p->name=obj->name;
								p->id=getId((char*)p->name.c_str());
								p->area=quad(obj->x, obj->y, obj->width, obj->height);
								p->target=getId((char*)obj->properties["target"].c_str());
								try{p->map_id=getId((char*)obj->properties.at("map").c_str());}catch(...){}
								portals[p->id]=p;
							}
						}
					}// add teleport zones with id fix
				}
				delete map;
				free(xml);
//				printf("Map %s loaded\n", path);				
			}catch(...){
				printf("Error loading map %s\n", path);
			}
		}else{
			printf("Can't loadFile map %s\n", path);
		}
//		if (grid)
//			delete[] grid;
		
		{//set main map borders
			point lt(0, 0);
			point rt(0, map_size[1]-1);
			point rb(map_size[0]-1, map_size[1]-1);
			point lb(map_size[0]-1, 0);
			segments.push_back(new segment(lt,rt));
			segments.push_back(new segment(rt,rb));
			segments.push_back(new segment(rb,lb));
			segments.push_back(new segment(lb,lt));
		}
		size.x=ceil((1.0*map_size[0])/cell.x);//TODO: change to local server area
		size.y=ceil((1.0*map_size[1])/cell.y);
		int grid_size=size.x*size.y;
//		printf("size %d %d\n", grid_size, grid_size*sizeof(share::cell));
		
//		grid=new share::cell[grid_size+1];
		grid.resize(grid_size+1);
		for(int i=0;i<grid_size;i++){
			vector<segment> &&borders=cell_borders(i);
//			printf("%d: ",i);
			for(int j=0,jend=segments.size();j<jend;j++){
				for(int b=0;b<4;b++){
					if (borders[b].cross(segments[j])){
						grid[i].segments.push_back(segments[j]);
						//printf("%d|%d[%g,%g %g,%g](%g,%g %g,%g) \n", j, b,segments[j]->a.x,segments[j]->a.y,segments[j]->b.x,segments[j]->b.y,borders[b].a.x,borders[b].a.y,borders[b].b.x,borders[b].b.y);
						break;
					}
				}
			}
			auto &&q=cell_quad(id);
			for(auto &&pi: portals){
				portal *p=pi.second;
				if (
					p->area.contains(borders[0].a) || //0 - left top 2 - right bottom
					p->area.contains(borders[0].b) ||
					p->area.contains(borders[2].a) ||
					p->area.contains(borders[2].a) ||
					q.contains(p->area.a) ||
					q.contains(p->area.b) 
				){
						grid[i].portals.push_back(p);
				}
			}
			for(auto &&pi: safezones){
				quad *p=&std::get<1>(pi);
				if (
					p->contains(borders[0].a) || //0 - left top 2 - right bottom
					p->contains(borders[0].b) ||
					p->contains(borders[2].a) ||
					p->contains(borders[2].a) ||
					q.contains(p->a) ||
					q.contains(p->b) 
				){
						grid[i].safezones.push_back(p);
				}
			}
			
//			printf("\n");
		}
		near_cells(35, 5);
//		printf("map initialized\n");
	}
	
	cell* map::cells(int id){
		int s;
//		return &grid[id];
		return &grid[id<0?0:(id>=(s=sqr(size.x))?(s-1):id)];
	}
	
	cell* map::cells(point &p){
		return cells(to_grid(p.x, p.y));
	}
	
	cell* map::cells(typeof(point::x) x, typeof(point::y) y){
		return cells(to_grid(x, y));
	}
	
	std::list<int> map::cells(point &a, float r){
		return cells(a.x-r, a.y-r, a.x+r, a.y+r);
	}
	
	std::list<int> map::cells(point &&a, point &&b){
		return cells(a.x, a.y, b.x, b.y);;
	}
	
	std::list<int> map::cells(typeof(point::x) l, typeof(point::y) t, typeof(point::x) r, typeof(point::y) b){
		std::list<int> v;
		//TODO: check returned ids
		for(int x=to_grid_x(l), xend=to_grid_x(r);x<=xend;x++){
			for(int y=to_grid_y(t), yend=to_grid_y(b);y<=yend;y++){
				v.push_back(x+y*size.y);
			}
		}
		return v;
	}
	
	int map::to_grid(typeof(point::x) x, typeof(point::y) y){
		return to_grid_x(x)+to_grid_y(y)*size.y;
	}
	
	int map::to_grid_x(typeof(point::x) x){
		int o=x/cell.x;
//		return x/cell.x;
		return o<0?0:(o>=size.x?size.x-1:o);//check
	}
	
	int map::to_grid_y(typeof(point::y) y){
		int o=y/cell.y;
//		return y/cell.y;
		return o<0?0:(o>=size.y?size.y-1:o);//check
	}
	
	int map::id_to_x(int id){
		return id%size.y;
	}
	
	int map::id_to_y(int id){
		return id/size.y;
	}
	
	std::vector<segment> map::cell_borders(int id){
		vector<segment> v;
		int x=id_to_x(id);
		int y=id_to_y(id);
		point p1(x*cell.x, y*cell.y);
		point p2(x*cell.x, y*cell.y+cell.y);
		point p3(x*cell.x+cell.x, y*cell.y+cell.y);
		point p4(x*cell.x+cell.x, y*cell.y);
		segment s1(p1, p2);
		segment s2(p2, p3);
		segment s3(p3, p4);
		segment s4(p4, p1);
		v.push_back(s1);
		v.push_back(s2);
		v.push_back(s3);
		v.push_back(s4);
		return v;
	}
	
	quad map::cell_quad(int id){
		int x=id_to_x(id);
		int y=id_to_y(id);
		return quad(point(x*cell.x, y*cell.y), point((x+1)*cell.x, (y+1)*cell.y));
	}
	
	std::list<int> map::near_cells(int id, typeof(npc::r) r){
		return near_cells(id_to_x(id)*cell.x+cell.x/2, id_to_y(id)*cell.y+cell.y/2, r+(cell.x+cell.y)/4);
	}
	
	std::list<int> map::near_cells(typeof(point::x) x, typeof(point::y) y, typeof(npc::r) r){
		//TODO: check returned data
		return cells(x-r, y-r, x+r, y+r);
	}

	int map::nearest_safezone_id(point& p){
		int out=-1;
		float d=0;
		for (auto &&i: safezones){
			float $=i.second.qdistanse(p);
			//printf("{%g %g %g %g} <-> [%g %g] = %g\n", i.second.a.x,i.second.a.y,i.second.b.x,i.second.b.y,p.x,p.y,$);
			if (out<0 || $<d){
				d=$;
				out=i.first;
			}
		}
		return out;
	}
	
	quad& map::nearest_safezone(typeof(point::x) x, typeof(point::y) y){
		point p(x, y);
		return nearest_safezone(p);
	}
	
	quad& map::nearest_safezone(point& p){
		return safezones.at(nearest_safezone_id(p));
	}
	
	quad& map::nearest_safezone(point&& p){
		return safezones.at(nearest_safezone_id(p));
	}
	
	int map::getId(char *s){
		return crc32((const void*)s, (size_t)strlen(s));
	}
}
