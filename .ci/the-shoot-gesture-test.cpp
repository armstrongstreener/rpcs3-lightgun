#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
#include <array>
#include "../rpcs3/Input/lightgun_spin.h"
struct Detector {
 std::vector<int> dirs; std::vector<std::array<float,2>> positions; double since_front=0; bool recognised=false;
 void step(float angle, std::array<float,2> position, double dt) {
  since_front+=dt; const float x=std::sin(angle),z=std::cos(angle);
  int d=-1; if(z>.85f)d=0;else if(x<-.85f)d=1;else if(x>.85f)d=3;else if(z<-.85f)d=2;
  if(d<0)return;
  if(dirs.empty()||dirs.back()!=d){dirs.push_back(d);positions.push_back(position);if(dirs.size()>5){dirs.erase(dirs.begin());positions.erase(positions.begin());}}
  if(d==0){
   if(dirs.size()==5&&dirs[0]==0&&dirs[2]==2&&dirs[4]==0&&((dirs[1]==1&&dirs[3]==3)||(dirs[1]==3&&dirs[3]==1))){
    float distance=0;for(int i=1;i<5;i++)distance+=std::hypot(positions[i][0]-positions[i-1][0],positions[i][1]-positions[i-1][1]);
    if(since_front<1.8&&distance>1500)recognised=true;
   }
   since_front=0;
  }
 }
};
void run(int fps,bool moving,bool should_pass){
 lightgun_spin spin;Detector detector;const std::uint64_t tick=1000000/fps;
 for(std::uint64_t t=100000;t<2100000;t+=tick){const float angle=spin.update(t,t>=400000);detector.step(angle,moving?lightgun_spin::orbit(angle):std::array<float,2>{0,0},double(tick)/1000000);}
 assert(detector.recognised==should_pass);
}
int main(){
 for(int fps:{10,20,30,60,120}){run(fps,true,true);run(fps,false,false);}
 const auto start=lightgun_spin::orbit(0),end=lightgun_spin::orbit(6.283185307179586f);
 assert(std::hypot(start[0]-end[0],start[1]-end[1])<.001f);
 auto right=lightgun_spin::orbit(1.57079632679f);assert(std::abs(right[0]-600)<.01f&&std::abs(right[1]+600)<.01f);
 puts("PASS: facing order, <1.8 second completion and >1500 mm travel at 10/20/30/60/120 fps; stationary turn rejected; closed orbit");
}

