#include "stdafx.h"
#include "Player.h"
#include <vector>
#include <tlhelp32.h>
#include <tchar.h>

#define PI 3.141592

#define SELF_PTR 0x0050F4F4
#define OP_PTR 0x0050F4F8


DWORD pid;
HANDLE proc;	//Proccess handle for this session of the game
std::vector<Player> player;
Player self;
Player bot;


/*
Obtain the process ID via game name
*/
DWORD getPID() {	
	LPCWSTR name = L"AssaultCube";
	pid = 0;
	HWND window = FindWindow(NULL, name);
	GetWindowThreadProcessId(window, &pid);
	if (pid == 0) printf("[ERROR]: Couldn't find open instance of Assult Cube\r\n");
	return pid; //hardcode for now;
}

/*
Return the distance between two given players (3D Pythagorean Calculation)
*/
float getDistance(Player p1, Player p2) {
	auto s1 = p1.getCoord();
	auto s2 = p2.getCoord();
	float distance = sqrt(pow((s1[0] - s2[0]), 2) + pow((s1[1] - s2[1]), 2) + pow((s1[2] - s2[2]), 2)); //dw, I'm prob gonna forget i can do this for SAT Math 2 anyways
	return distance;
}

/*
Return the nearest player
*/
Player findNearestPlayer(Player self, std::vector<Player> players) { //pretty self explanatory iterator and min finder
	Player closest;
	float distance = -1.0; 
	std::vector<Player>::iterator p;
	for (p = players.begin(); p != players.end(); p++) {
		float d = getDistance(self, *p);
		if (distance < 0.0 || d < distance) {
			closest = *p;
			distance = d;
		}
	}
	return closest;
}


/*
Create yaw and pitch look command. 
Yaw command is simple 2D arctangent angle
Pitch command is also 2D arctangent angle but one axis is calculuated from 2D distance formula
*/
void computeLookPitchYaw(Player self, Player target) { //avoid coordinate lock w/ atan2, compute position vector in the future?
													   //technically abusing pass by value as reference since the Player contains memory writes
	auto s = self.getCoord();
	auto t = target.getCoord();
	float yaw = (180.0/PI) * (atan2((s[1] - t[1]), (s[0] - t[0])) + PI); //Shift arctan2 for range of [0, 360] rather than [-180,180] than convert to deg
	float pitch = (180.0 / PI) * (atan((t[2] - s[2])/sqrt( pow((t[0] - s[0]), 2)+pow((t[1] - s[1]),2) )));  //sorry future self, you'll have to redraw this
																											//p.s. fuck me
	self.look(yaw, pitch);
}


int main()
{
	proc = OpenProcess(PROCESS_ALL_ACCESS , FALSE, getPID());
	if (!proc) printf("[ERROR]: Couldn't create handle for Assult Cube (check permissions)\r\n");
	int self_base;
	int bot_base;
	ReadProcessMemory(proc, (LPCVOID)(SELF_PTR), &self_base, 4, NULL);	//is it bad to hardcode 4 in there? Like should I do sizeof(SIZE_T)? But TBH does it matter?
	ReadProcessMemory(proc, (LPCVOID)(OP_PTR), &bot_base, 4, NULL);
	self = Player(proc, self_base);
	bot = Player(proc, bot_base);
		
	while (1) {
		printf("Self coord: %f | %f | %f %d\r\n", self.getCoord()[0], self.getCoord()[1], self.getCoord()[2]);
		printf("Self health: %d Bot Health %d \r\n", self.getHealth(), bot.getHealth());
		computeLookPitchYaw(self, findNearestPlayer(self, player));
	}
	
	CloseHandle(proc);
	return 0;
}


/*
I'll probably get screwed over somewhere since I passed everyhting by value but nvm that 

Memory Adresses Neeeded: 
	-X Y Z offsets 
	-Look Y Z offsets 
	-Player Base Offset 
	-Enemy Base Offset

IF THE POINTER DOESN'T WORK
	1. Memory scan for health value 
	2. Find what accesses address to find base adddress (health-F8)
	3. Search for Base Address as hex. Find green static pointer 
	4. This is new pointer

Code Structure:
	Player.cpp/h 
		-Player class with construction Player(Base Address) 
	Aimbot.cpp
		-Process handle
		-Array of players and your player 
		-Target selection and tracking
*/
