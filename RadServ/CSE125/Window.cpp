#define _USE_MATH_DEFINES
#include <string>
#include <iostream>
#include <time.h>
#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
#include "Server.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "Window.h"
#include <time.h>
#include "Object.h"
#include "VAO.h"
#include "glslprogram.h"
#include "Cube.h"
#include "ShaderController.h"
#include "Ground.h"
#include <Qt/QtGui/QImage> 
#include <Qt/QtOpenGL/QGLWidget>
#include "SkyBox.h"
#include "Structures.h"
#include "Sphere.h"
#include "TextureScreen.h"
#include "Camera.h"
#include "Scene.h"
#include "Mesh.h"
#include "Texture.h"
#include "ConfigSettings.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "gameState.h"
#include "constants.h"
using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::mat3;
using glm::quat;
using namespace std;
Ground* ground;
float cam_sp = (float)0.1;
string configBuf;
int counter = 0;
std::vector <pair<string, mat4>>* sendVec = new vector<pair<string, mat4>>;
std::vector <pair<string, mat4>>* recvVec = new vector<pair<string, mat4>>;
Scene* scene;
boost::asio::io_service io_service;
tcp_server* server;
int cam_rot = 0;
int playerID = -1;
int numOfVecs = 4;
int keyState = 0;
bool player1shoot, player2shoot, player3shoot, player4shoot;

std::string str;
gameState gs;

bool sendddddddddddedededed = false;
int p0Shots = 0;
int p1Shots = 0;
int p2Shots = 0;
int p3Shots = 0;


bool hasShot[4] = { false };
int powerUpStatus[4] = { 0 };

#define PLAYER_PROJECTILE_COUNT 1000;
int p1ShotID = 0;
int p2ShotID = 0;
int p3ShotID = 0;
int p4ShotID = 0;
int p_shoot_counter;

string int_to_string(int num, int length){
	string t = "";
	int num_zero;
	if (num > 1){
		num_zero = length - (int)floor(log10(num*10));
	}
	else{
		num_zero = length - 1;
	} 
	for (int i = 0; i < num_zero;i++)
		t += "0";
	t += std::to_string(num);
	assert(t.length() == length);
	return t;
}

void handle_mouse_state(int pid, int mouseState){
	int shootID = 0;
	if (mouseState & 1){
		scene->basicAttack(pid);
	}
	else if (mouseState & 1 << 1){
		//std::cout << "projectile attack from client" << std::endl;
		
		if (pid == 0)
		{
			if (!hasShot[0])
			{
				player1shoot = true;
				p1ShotID = p_shoot_counter;
				shootID = p_shoot_counter;
				p_shoot_counter++;
				p_shoot_counter %= PLAYER_PROJECTILE_COUNT;
				hasShot[0] = true;
			}
		}
		else if (pid == 1)
		{
			if (!hasShot[1])
			{
				player2shoot = true;
				p2ShotID = p_shoot_counter;
				shootID = p_shoot_counter;
				p_shoot_counter++;
				p_shoot_counter %= PLAYER_PROJECTILE_COUNT;
				hasShot[1] = true;
			}
		}
		else if (pid == 2)
		{
			if (!hasShot[2])
			{
				player3shoot = true;
				p3ShotID = p_shoot_counter;
				shootID = p_shoot_counter;
				p_shoot_counter++;
				p_shoot_counter %= PLAYER_PROJECTILE_COUNT;
				hasShot[2] = true;
			}
		}
		else if (pid == 3)
		{
			if (!hasShot[3])
			{
				player4shoot = true;
				p4ShotID = p_shoot_counter;
				shootID = p_shoot_counter;
				p_shoot_counter++;
				p_shoot_counter %= PLAYER_PROJECTILE_COUNT;
				hasShot[3] = true;
			}
		}
		//std::cout << player1shoot << player2shoot << player3shoot << player4shoot << std::endl;

		scene->projectileAttack(pid, &(*recvVec)[playerID * 4 + 2].second, shootID);
	}
	else if (!(mouseState & 1 << 1))
	{
		hasShot[pid] = false;
	}
}
void handle_key_state(int pid, int keyState){
	if (scene->getPlayerObj(pid) == NULL)
		return;
	if (keyState & 1){ //'a'
		//cout << "move left" << endl;
		scene->setHMove(pid, -((scene->getPlayer(playerID))->getBoots())->getMoveSpeed());
	}
	else{
		scene->cancelHMove(pid, -((scene->getPlayer(playerID))->getBoots())->getMoveSpeed());
	}
	if (keyState & 1 << 1){ //'d'
		//cout << "move right" << endl;
		scene->setHMove(pid, ((scene->getPlayer(playerID))->getBoots())->getMoveSpeed());
	}
	else{
		scene->cancelHMove(pid, ((scene->getPlayer(playerID))->getBoots())->getMoveSpeed());
	}
	if (keyState & 1 << 2){ //'w'
		//cout << "move up" << endl;
		scene->setVMove(pid, ((scene->getPlayer(playerID))->getBoots())->getMoveSpeed());
	}
	else{
		scene->cancelVMove(pid, ((scene->getPlayer(playerID))->getBoots())->getMoveSpeed());
	}
	if (keyState & 1 << 3){ //'s'
		//cout << "move down" << endl;
		scene->setVMove(pid, -((scene->getPlayer(playerID))->getBoots())->getMoveSpeed());
	}
	else{
		scene->cancelVMove(pid, -((scene->getPlayer(playerID))->getBoots())->getMoveSpeed());
	}
	if (keyState & 1 << 4){ //' '
		//cout << "jump" << endl;
		scene->jump(pid);
	}
	else
	{

	}
	//if (keyState & 1 << 5){ //'W' for sprint
	//	scene->setVMove(pid, ((scene->getPlayer(playerID))->getBoots())->getSprintSpeed());
	//}
	//else{
	//	scene->cancelVMove(pid, ((scene->getPlayer(playerID))->getBoots())->getSprintSpeed());
	//}
}
void handle_cam_rot(int pid, float cam_rot){
	if (scene->getPlayerObj(pid) == NULL)
		return;
	scene->pushRot(pid, -cam_sp*cam_rot);
	cam_rot = 0; // possibly a problem
}
void handle_cam_mat(int pid, mat4 camM)
{
	scene->setCamM(pid, camM);
}
int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // open an OpenGL context with double buffering, RGB colors, and depth buffering
	glutInitWindowSize(10, 10); // set initial window size
	glutCreateWindow("CSE 125 - Group 4 (RadioactiveChipmunks)");
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		fprintf(stderr, "Error initializing GLEW: %s\n",
			glewGetErrorString(err));
		system("pause");
	}
	scene = new Scene();
	scene->setGravity(vec3(0, -9.8, 0));

	//Payer Mats
	sendVec->push_back(std::make_pair("", mat4(0.0f)));
	sendVec->push_back(std::make_pair("", mat4(0.0f)));
	sendVec->push_back(std::make_pair("", mat4(0.0f)));
	sendVec->push_back(std::make_pair("", mat4(0.0f)));

	//Tower Mats
	sendVec->push_back(std::make_pair("", mat4(0.0f)));
	sendVec->push_back(std::make_pair("", mat4(0.0f)));
	sendVec->push_back(std::make_pair("", mat4(0.0f)));
	sendVec->push_back(std::make_pair("", mat4(0.0f)));

	//Player Projectile Despawn List
	sendVec->push_back(std::make_pair("", mat4(0.0f)));

	
	//Player Cam Mats
	sendVec->push_back(std::make_pair("", mat4(0.0f)));
	sendVec->push_back(std::make_pair("", mat4(0.0f)));
	sendVec->push_back(std::make_pair("", mat4(0.0f)));
	sendVec->push_back(std::make_pair("", mat4(0.0f)));
	
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));

	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));

	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));

	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));

	srand(time(NULL));

	try
	{
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(tcp::v4(), "localhost", "13"); // does nothing for the moment
		tcp::resolver::iterator itr = resolver.resolve(query);
		tcp::endpoint endpoint = *itr;
		server = new tcp_server(io_service, endpoint);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	LARGE_INTEGER freq, last, current, loop_end;
	double diff;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&last);
	string p0, p1, p2, p3;

	bool newData[4] = { false };
	while (true){
		QueryPerformanceCounter(&current);
		diff = (double)(current.QuadPart - last.QuadPart) / (double)freq.QuadPart;
		last = current;
		recvVec = server->getState();
		io_service.poll();
		server->getDataState(newData);
		server->age();//tell server the data is deprecated
		player1shoot = false;
		player2shoot = false;
		player3shoot = false;
		player4shoot = false;
		if (strcmp((*recvVec)[RECVINDEXP0].first.c_str(), ""))
		{
			playerID = atoi((*recvVec)[0].first.c_str()); 
			handle_key_state(playerID, (int)(*recvVec)[playerID * VECSPERPLAYER].second[0][0]);
			if (newData[0]){
				handle_mouse_state(playerID, (int)(*recvVec)[playerID * VECSPERPLAYER + 1].second[0][0]);
				handle_cam_mat(playerID, (*recvVec)[playerID * VECSPERPLAYER + 2].second);
				handle_cam_rot(playerID, (int)(*recvVec)[playerID * VECSPERPLAYER + 3].second[0][0]);
			}
		}
		// VECTOR INDICES NEED UPDATE FOR MOUSE
		if (strcmp((*recvVec)[RECVINDEXP1].first.c_str(), ""))
		{
			playerID = atoi((*recvVec)[numOfVecs].first.c_str());
			handle_key_state(playerID, (int)(*recvVec)[playerID * VECSPERPLAYER].second[0][0]);
			if (newData[1]){
				handle_mouse_state(playerID, (int)(*recvVec)[playerID * VECSPERPLAYER + 1].second[0][0]);
				handle_cam_mat(playerID, (*recvVec)[playerID * VECSPERPLAYER + 2].second);
				handle_cam_rot(playerID, (int)(*recvVec)[playerID * VECSPERPLAYER + 3].second[0][0]);
			}
			
		}
		if (strcmp((*recvVec)[RECVINDEXP2].first.c_str(), ""))
		{
			playerID = atoi((*recvVec)[numOfVecs * 2].first.c_str());
			handle_key_state(playerID, (int)(*recvVec)[playerID * 4].second[0][0]);
			if (newData[2]){
				handle_mouse_state(playerID, (int)(*recvVec)[playerID * VECSPERPLAYER + 1].second[0][0]);
				handle_cam_mat(playerID, (*recvVec)[playerID * VECSPERPLAYER + 2].second);
				handle_cam_rot(playerID, (int)(*recvVec)[playerID * VECSPERPLAYER + 3].second[0][0]);
			}
		}
		if (strcmp((*recvVec)[RECVINDEXP3].first.c_str(), ""))
		{
			playerID = atoi((*recvVec)[numOfVecs * 3].first.c_str());
			handle_key_state(playerID, (int)(*recvVec)[playerID * VECSPERPLAYER].second[0][0]);
			if (newData[3]){
				handle_mouse_state(playerID, (int)(*recvVec)[playerID * VECSPERPLAYER + 1].second[0][0]);
				handle_cam_mat(playerID, (*recvVec)[playerID * VECSPERPLAYER + 2].second);
				handle_cam_rot(playerID, (int)(*recvVec)[playerID * VECSPERPLAYER + 3].second[0][0]);
			}
		}

		scene->simulate(diff, (float)(1.0 / 100));
		boost::array<mat4, 4> mp = scene->getPlayerMats();
		boost::array<mat4, 4> mt = scene->getTowerMats();
		boost::array<mat4, 4> ca = scene->getPlayerCams();

		// Print out matrix contents
		/*
		cout << (m[0])[0][0] << (m[0])[0][1] << (m[0])[0][2] << (m[0])[0][3] << endl;
		cout << (m[0])[1][0] << (m[0])[1][1] << (m[0])[1][2] << (m[0])[1][3] << endl;
		cout << (m[0])[2][0] << (m[0])[2][1] << (m[0])[2][2] << (m[0])[2][3] << endl;
		cout << (m[0])[3][0] << (m[0])[3][1] << (m[0])[3][2] << (m[0])[3][3] << endl;
		*/

		if (player1shoot == true){
			p0Shots += 2;
		}
		if (player2shoot == true){
			p1Shots += 2;
		}
		if (player3shoot == true){
			p2Shots += 2;
		}
		if (player4shoot == true){
			p3Shots += 2;
		}

		if (p0Shots > 0){
			p0 = "0s";
			p0Shots--;
		}else{
			p0 = "0S";
		}

		if (p1Shots > 0){
			p1 = "1s";
			p1Shots--;
		}else{
			p1 = "1S";
		}

		if (p2Shots > 0){
			p2 = "2s";
			p2Shots--;
		}else{
			p2 = "2S";
		}

		if (p3Shots > 0){
			p3 = "3s";
			p3Shots--;
		}else{
			p3 = "3S";
		}

		//SEND SHIT HERE by adding to the p0-p3 strings//

		//sending if a player was damaged
		if (scene->getPlayerDamaged(PLAYER0))
			p0 += "d";
		else
			p0 += "D";
		if (scene->getPlayerDamaged(PLAYER1))
			p1 += "d";
		else
			p1 += "D";
		if (scene->getPlayerDamaged(PLAYER2))
			p2 += "d";
		else
			p2 += "D";
		if (scene->getPlayerDamaged(PLAYER3))
			p3 += "d";
		else
			p3 += "D";
		//reset the playerDamaged flags
		if (sendddddddddddedededed)
		{
			scene->setPlayerDamaged(PLAYER0, false);
			scene->setPlayerDamaged(PLAYER1, false);
			scene->setPlayerDamaged(PLAYER2, false);
			scene->setPlayerDamaged(PLAYER3, false);
		}

		//sending if a player was killed
		if (scene->getPlayerDead(PLAYER0))
			p0 += "k";
		else
			p0 += "K";
		if (scene->getPlayerDead(PLAYER1))
			p1 += "k";
		else
			p1 += "K";
		if (scene->getPlayerDead(PLAYER2))
			p2 += "k";
		else
			p2 += "K";
		if (scene->getPlayerDead(PLAYER3))
			p3 += "k";
		else
			p3 += "K";

		p0 += int_to_string(p1ShotID, 3);
		p1 += int_to_string(p2ShotID, 3);
		p2 += int_to_string(p3ShotID, 3);
		p3 += int_to_string(p4ShotID, 3);

		if (sendddddddddddedededed)
		{
			scene->setPlayerDead(0, false);
			scene->setPlayerDead(1, false);
			scene->setPlayerDead(2, false);
			scene->setPlayerDead(3, false);
		}

		p0 += int_to_string(scene->getPlayerHealth(PLAYER0), 3);
		p1 += int_to_string(scene->getPlayerHealth(PLAYER1), 3);
		p2 += int_to_string(scene->getPlayerHealth(PLAYER2), 3);
		p3 += int_to_string(scene->getPlayerHealth(PLAYER3), 3);

		p0 += int_to_string(scene->getPlayerKills(PLAYER0), 3);
		p1 += int_to_string(scene->getPlayerKills(PLAYER1), 3);
		p2 += int_to_string(scene->getPlayerKills(PLAYER2), 3);
		p3 += int_to_string(scene->getPlayerKills(PLAYER3), 3);

		// Powerup data encoding
		// P0
		if (scene->getPlayerPowerUp(PLAYER0)[0])
		{
			powerUpStatus[PLAYER0] = SPEEDUP;
		}
		else if (scene->getPlayerPowerUp(PLAYER0)[1])
		{
			powerUpStatus[PLAYER0] = DOUBLEDAMAGE;
		}
		else if (scene->getPlayerPowerUp(PLAYER0)[2])
		{
			powerUpStatus[PLAYER0] = HEALTHBOOST;
		}
		else if (scene->getPlayerPowerUp(PLAYER0)[3])
		{
			powerUpStatus[PLAYER0] = FASTERSHOOT;
		}
		else if (scene->getPlayerPowerUp(PLAYER0)[4])
		{
			powerUpStatus[PLAYER0] = FARTHERSHOOT;
		}
		else
		{
			powerUpStatus[PLAYER0] = NOPOWER;
		}
		// P1
		if (scene->getPlayerPowerUp(PLAYER1)[0])
		{
			powerUpStatus[PLAYER1] = SPEEDUP;
		}
		else if (scene->getPlayerPowerUp(PLAYER1)[1])
		{
			powerUpStatus[PLAYER1] = DOUBLEDAMAGE;
		}
		else if (scene->getPlayerPowerUp(PLAYER1)[2])
		{
			powerUpStatus[PLAYER1] = HEALTHBOOST;
		}
		else if (scene->getPlayerPowerUp(PLAYER1)[3])
		{
			powerUpStatus[PLAYER1] = FASTERSHOOT;
		}
		else if (scene->getPlayerPowerUp(PLAYER1)[4])
		{
			powerUpStatus[PLAYER1] = FARTHERSHOOT;
		}
		else
		{
			powerUpStatus[PLAYER1] = NOPOWER;
		}
		// P2
		if (scene->getPlayerPowerUp(PLAYER2)[0])
		{
			powerUpStatus[PLAYER2] = SPEEDUP;
		}
		else if (scene->getPlayerPowerUp(PLAYER2)[1])
		{
			powerUpStatus[PLAYER2] = DOUBLEDAMAGE;
		}
		else if (scene->getPlayerPowerUp(PLAYER2)[2])
		{
			powerUpStatus[PLAYER2] = HEALTHBOOST;
		}
		else if (scene->getPlayerPowerUp(PLAYER2)[3])
		{
			powerUpStatus[PLAYER2] = FASTERSHOOT;
		}
		else if (scene->getPlayerPowerUp(PLAYER2)[4])
		{
			powerUpStatus[PLAYER2] = FARTHERSHOOT;
		}
		else
		{
			powerUpStatus[PLAYER2] = NOPOWER;
		}
		// P3
		if (scene->getPlayerPowerUp(PLAYER3)[0])
		{
			powerUpStatus[PLAYER3] = SPEEDUP;
		}
		else if (scene->getPlayerPowerUp(PLAYER3)[1])
		{
			powerUpStatus[PLAYER3] = DOUBLEDAMAGE;
		}
		else if (scene->getPlayerPowerUp(PLAYER3)[2])
		{
			powerUpStatus[PLAYER3] = HEALTHBOOST;
		}
		else if (scene->getPlayerPowerUp(PLAYER3)[3])
		{
			powerUpStatus[PLAYER3] = FASTERSHOOT;
		}
		else if (scene->getPlayerPowerUp(PLAYER3)[4])
		{
			powerUpStatus[PLAYER3] = FARTHERSHOOT;
		}
		else
		{
			powerUpStatus[PLAYER3] = NOPOWER;
		}

		p0 += int_to_string(powerUpStatus[PLAYER0], 1);
		p1 += int_to_string(powerUpStatus[PLAYER1], 1);
		p2 += int_to_string(powerUpStatus[PLAYER2], 1);
		p3 += int_to_string(powerUpStatus[PLAYER3], 1);

		//Trampoline status
		



		//despawn player projectile list
		string ppdl_str;
		if (sendddddddddddedededed){
			vector<int> ppdl = scene->getPlayerProjectileDespawnList();
			scene->clearPlayerProjectileDespawnList();
			ppdl_str = "";
			for (uint i = 0; i < ppdl.size(); i++){
				ppdl_str += int_to_string(ppdl[i], 3);
			}
		}


		(*sendVec)[PLAYER_MAT_BEGIN + PLAYER0] = std::make_pair(p0.c_str(), mp[PLAYER0]);
		(*sendVec)[PLAYER_MAT_BEGIN + PLAYER1] = std::make_pair(p1.c_str(), mp[PLAYER1]);
		(*sendVec)[PLAYER_MAT_BEGIN + PLAYER2] = std::make_pair(p2.c_str(), mp[PLAYER2]);
		(*sendVec)[PLAYER_MAT_BEGIN + PLAYER3] = std::make_pair(p3.c_str(), mp[PLAYER3]);

		(*sendVec)[TOWER_MAT_BEGIN + 0] = std::make_pair("t0", mt[0]);
		(*sendVec)[TOWER_MAT_BEGIN + 1] = std::make_pair("t1", mt[1]);
		(*sendVec)[TOWER_MAT_BEGIN + 2] = std::make_pair("t2", mt[2]);
		(*sendVec)[TOWER_MAT_BEGIN + 3] = std::make_pair("t3", mt[3]);

		(*sendVec)[PPDL_MAT] = std::make_pair(ppdl_str, mat4(1.0));

		
		(*sendVec)[CAM_MAT_BEGIN + PLAYER0] = std::make_pair("c0", ca[PLAYER0]);
		(*sendVec)[CAM_MAT_BEGIN + PLAYER1] = std::make_pair("c1", ca[PLAYER1]);
		(*sendVec)[CAM_MAT_BEGIN + PLAYER2] = std::make_pair("c2", ca[PLAYER2]);
		(*sendVec)[CAM_MAT_BEGIN + PLAYER3] = std::make_pair("c3", ca[PLAYER3]);
		
		//std::cout << gs.getPosString(sendVec) << std::endl;
		//std::cout << "pair 0: " << ((*sendVec)[0].first.c_str()) << std::endl;
		//std::cout << "pair 1: " << ((*sendVec)[1].first.c_str()) << std::endl;
		//std::cout << "pair 2: " << ((*sendVec)[2].first.c_str()) << std::endl;
		//std::cout << "pair 3: " << ((*sendVec)[3].first.c_str()) << std::endl;
		//server->send(*sendVec);

		if (sendddddddddddedededed){
			str = gs.getPosString(sendVec);

			server->send(str + '\n');
			io_service.poll();
		}
		
		sendddddddddddedededed = (!sendddddddddddedededed);

		//str = "";

		//limit the speed of server
		QueryPerformanceCounter(&loop_end);
		diff = (double)(loop_end.QuadPart - last.QuadPart) / (double)freq.QuadPart * 1000;
		if (diff < 15){
			//cout << diff << endl;
			Sleep(15 - diff);			
		}
		else{
			//cout << diff << endl;
		}
	}
	return 0;
}