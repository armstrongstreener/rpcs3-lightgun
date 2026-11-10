#include <cassert>
#include <cmath>
#include <cstdio>
#include "../rpcs3/Input/lightgun_aim_scale.h"
int main(){
 auto near=[](float a,float b){return std::abs(a-b)<.000001f;};
 for(float p:{-.1f,0.f,.1f,.5f,.9f,1.f,1.1f})assert(lightgun_scale_axis(p,100)==p);
 assert(lightgun_scale_axis(.5f,90)==.5f);
 assert(near(lightgun_scale_axis(.9f,90),.86f));assert(near(lightgun_scale_axis(.1f,90),.14f));
 assert(lightgun_scale_axis(0,90)==0&&lightgun_scale_axis(1,90)==1);
 assert(lightgun_scale_axis(-.1f,90)==-.1f&&lightgun_scale_axis(1.1f,90)==1.1f);
 assert(lightgun_scale_axis(.9f,200)==1&&lightgun_scale_axis(.1f,200)==0);
 assert(near(lightgun_scale_axis(.9f,80),.82f));
 puts("PASS: default identity, fixed centre, 10% reduction, symmetry, independent scales, off-screen preservation and bounds");
}

