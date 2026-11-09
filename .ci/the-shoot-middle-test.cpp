#include <cassert>
#include <cstdio>
#include "../rpcs3/Input/lightgun_spin.h"
int main(){
 lightgun_middle_gesture a,b; lightgun_spin spin;
 a.update(true,false,true);assert(!a.point_down&&a.held);spin.update(1000,a.held&&!a.point_down);assert(spin.active);
 a.update(true,true,true);assert(!a.point_down); // no mode change mid-press
 a.update(false,true,true);assert(!a.point_down&&!a.held);
 a.update(true,true,true);assert(a.point_down&&a.held);
 a.update(true,false,true);assert(a.point_down); // coming back on screen must not trigger Showtime
 a.update(false,false,true);assert(!a.point_down);
 b.update(true,true,true);assert(b.point_down&&!a.point_down);
 a.update(true,true,false);assert(!a.point_down);
 puts("PASS: on-screen spin, off-screen down, stable action while held, release, independent players and disabled gesture");
}

