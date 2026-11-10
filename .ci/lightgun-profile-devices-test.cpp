#include <cassert>
#include <string>
#include <cstdio>
#include "../rpcs3/Input/raw_mouse_profile.h"
int main(){
 auto profile=YAML::Load("Player 1:\n  Device: OLD_GUN\n  Button 1: Button 2\n  Button 1 Alternate: Key 123\n  The Shoot Spin 360: true\n  Action 1 Label: My trigger\nPlayer 2:\n  Device: ''\n  Button 1: Button 3\n");
 strip_raw_mouse_profile_devices(profile);
 assert(!profile["Player 1"]["Device"]&&!profile["Player 2"]["Device"]);
 assert(profile["Player 1"]["Button 1"].as<std::string>()=="Button 2");
 assert(profile["Player 1"]["Button 1 Alternate"].as<std::string>()=="Key 123");
 assert(profile["Player 1"]["The Shoot Spin 360"].as<bool>());
 assert(profile["Player 1"]["Action 1 Label"].as<std::string>()=="My trigger");
 YAML::Emitter out;out<<profile;assert(out.good());assert(std::string(out.c_str()).find("Device:")==std::string::npos);
 auto saved=YAML::Load(out.c_str());strip_raw_mouse_profile_devices(saved);assert(saved["Player 2"]["Button 1"].as<std::string>()=="Button 3");
 auto global=YAML::Load("Player 1:\n  Device: NEW_GUN\nPlayer 2:\n  Device: ''\n");
 for(auto player:profile){if(!player.second.IsMap())continue;for(auto field:player.second)global[player.first.as<std::string>()][field.first.as<std::string>()]=field.second;}
 assert(global["Player 1"]["Device"].as<std::string>()=="NEW_GUN");assert(global["Player 2"]["Device"].as<std::string>().empty());
 puts("PASS: legacy IDs ignored, IDs omitted on save, global assignments preserved, buttons/alternatives/labels/gestures retained and round-trip");
}

