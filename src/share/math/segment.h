#ifndef CLASTERED_SERVER_SLAVE_SEGMENT_HEADER
#define CLASTERED_SERVER_SLAVE_SEGMENT_HEADER

#include <iostream>

#include "point.h"

namespace share {
	class segment{
		public:
			point a,b;
			bool directed;
			
			segment();
			segment(point _a, point _b);
			segment(typeof(point::x) _ax, typeof(point::y) _ay, typeof(point::x) _bx, typeof(point::y) _by);
			float distanse(point &p);
			float signed_area2(point &p);
			float length();
			float length(float l);
			float mul(float l);
			char cross(segment *b);
			bool has_inside(point &p); 
			typeof(point::x) vector(point &p); //vector mul
			typeof(segment::a) to_vector(); 
			typeof(segment::a) cross_point(segment s);
			typeof(segment::a) mirror_by(segment s, char &a);
			segment to_quad();
			//quad
			typeof(segment::a) rand_point_in();
			float qdistanse(point &p);
			template<class T>
				bool contains(point_<T> &p){
					return a.x<=p.x && a.y<=p.y && a.x+b.x>=p.x && a.y+b.y>=p.y;
				};
		
			friend std::ostream& operator<<(std::ostream &stream, const segment &s);
	};
	
	//TODO: move to another class
	typedef segment quad;
}

#endif
