#include <cmath>
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include "../rpcs3/Input/lightgun_spin.h"
struct Q{float a[4];float&x(){return a[0];}float&y(){return a[1];}float&z(){return a[2];}float&w(){return a[3];}float x()const{return a[0];}float y()const{return a[1];}float z()const{return a[2];}float w()const{return a[3];}};
Q pitch(Q quat,int vertical){
			constexpr float c = 0.7071067811865475f;
			const float s = c * vertical;
			const auto base = quat;
			quat.x() = c*base.x() + s*base.w();
			quat.y() = c*base.y() - s*base.z();
			quat.z() = c*base.z() + s*base.y();
			quat.w() = c*base.w() - s*base.x();
return quat;}
int main(){
 assert(lightgun_vertical_direction(true,true,true,true)==0);
 assert(lightgun_vertical_direction(false,true,true,true)==-1);
 assert(lightgun_vertical_direction(false,false,true,true)==1);
 assert(lightgun_vertical_direction(false,false,false,true)==0);
 assert(lightgun_vertical_direction(false,false,true,false)==0);
 for(int direction:{-1,1})for(float yaw:{-.08f,0.f,.08f}){Q q{{0,std::sin(yaw/2),0,std::cos(yaw/2)}};auto r=pitch(q,direction);float y=2*(r.y()*r.z()-r.x()*r.w());assert(direction<0?y>.8f:y<-.8f);float n=0;for(float v:r.a)n+=v*v;assert(std::abs(n-1)<.00001f);}
 puts("PASS: up/down facing thresholds, normal aim, spin priority, down override and disabled off-screen up");}

