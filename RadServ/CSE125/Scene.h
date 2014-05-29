#pragma once
#include <vector>
#include <map>
#include <math.h> 
#include <iostream>
#include "Object.h"
#include "Octree.h"
#include "Projectile.h"
#include "Window.h"
#include "MD5Model.h"
#include "Camera.h"
#include "Ground.h"
using namespace std;


#define RESPAWN_COUNTER 100
#define MAX_SPEED 100
#define MAX_DISTANCE 100

#define MAX_DAMAGE -50

#define GRAVITY_SCALE 2.5
#define PLAYER_SPEED 10


class Scene
{
public:
	Scene(){
		camM.push_back(mat4(1.0));
		camM.push_back(mat4(1.0));
		camM.push_back(mat4(1.0));
		camM.push_back(mat4(1.0));
		initialize();
	}
	~Scene(){
		for (uint i = 0; i<stationary.size(); i++){
			delete stationary[i];
		}
		stationary.clear();
		for (uint i = 0; i<player.size(); i++){
			delete player[i];
		}
		player.clear();
	}

	Object * getPlayerObj(int playerID)
	{
		for (uint i = 0; i < player.size(); i++)
		{
			if (player[i]->getPlayerID() == playerID)
				return player[i];
		}
		return NULL;
	}

	void simulate(float t, float sub){
		resolvePlayerRotation();
		while (t > sub){
			t -= sub;
			resolvePlayerTransition(sub);
			resolveProjectileTransition(sub);
			resolveTowerTransition(sub);
		//	//octree here
			collisionDetection();
			collisionDetectionPlayer();
			collisionDetectionProjectile();
			despawnProjectile();
			rechargeJump();
			respawnPlayer();
		}
		resolvePlayerTransition(t);
		resolveProjectileTransition(t);
		resolveTowerTransition(t);
		////octree here
		collisionDetection();
		collisionDetectionPlayer();
		collisionDetectionProjectile();
		despawnProjectile();
		rechargeJump();
		respawnPlayer();
	}
	void collisionDetection(Octree* octree);
	void collisionDetection(){
		for (uint i = 0; i < player.size(); i++){
			bool touchGround1 = false;
			bool touchGround2 = false;
			//player-stationary detection
			for (uint j = 0; j < stationary.size(); j++){
				if (strcmp(stationary[j]->getType().c_str(), "Ground") == 0){
					AABB pBox = player[i]->getAABB();
					vec3 mid = (pBox.max + pBox.min) / 2.0f;
					float disp = ((Ground*)stationary[j])->getDispY(mid[0], mid[2]);
					if (disp != -1){
						if (pBox.min[1] < disp){
							touchGround1 = true;
							player[i]->postTrans(glm::translate(vec3(0.0f,disp-pBox.min[1],0.0f)));
							player[i]->clearYVelocity();
						}
						if (pBox.min[1] - disp < 0.1)
							touchGround1 = true;
					}
				}
				else{
					AABB pBox = player[i]->getAABB();
					AABB sBox = stationary[j]->getAABB();
					bool collide = true;
					for (int v = 0; v < 3; v++){
						if (pBox.max[v] <= sBox.min[v] || sBox.max[v] <= pBox.min[v]){
							collide = false;
							break;
						}
					}
					if (collide){
						fixCollision(player[i], stationary[j], pBox, sBox, touchGround1, touchGround2);
					}
					stationary[j]->touchGround(touchGround2);
				}
			}
			player[i]->touchGround(touchGround1);
		}
		collisionDetectionTower();
	}

	void collisionDetectionTower(){
		//tower-stationary detection
		for (uint i = 0; i < tower.size(); i++){
			bool touchGround1 = false;
			bool touchGround2 = false;
			for (uint j = 0; j < stationary.size(); j++){
				if (strcmp(stationary[j]->getType().c_str(), "Ground") == 0){
					AABB pBox = tower[i]->getAABB();
					vec3 mid = (pBox.max + pBox.min) / 2.0f;
					float disp = ((Ground*)stationary[j])->getDispY(mid[0], mid[2]);
					if (disp != -1){
						if (pBox.min[1] < disp){
							touchGround1 = true;
							tower[i]->postTrans(glm::translate(vec3(0.0f, disp - pBox.min[1], 0.0f)));
							tower[i]->clearYVelocity();
						}
						if (pBox.min[1] - disp < 0.1)
							touchGround1 = true;
					}
				}
				else{
					AABB pBox = tower[i]->getAABB();
					AABB sBox = stationary[j]->getAABB();
					bool collide = true;
					for (int v = 0; v < 3; v++){
						if (pBox.max[v] <= sBox.min[v] || sBox.max[v] <= pBox.min[v]){
							collide = false;
							break;
						}
					}
					if (collide){
						fixCollision(tower[i], stationary[j], pBox, sBox, touchGround1, touchGround2);
					}
					stationary[j]->touchGround(touchGround2);
				}
			}

			tower[i]->touchGround(touchGround1);
		}
	}
	void collisionDetectionPlayer(){
		for (uint i = 0; i < player.size(); i++){
			bool touchGround1 = player[i]->getTouchGround();
			//player-tower detection
			for (uint j = 0; j < tower.size(); j++){
				bool touchGround2 = tower[j]->getTouchGround();
				AABB pBox = player[i]->getAABB();
				AABB sBox = tower[j]->getAABB();
				bool collide = true;
				for (int v = 0; v < 3; v++){
					if (pBox.max[v] <= sBox.min[v] || sBox.max[v] <= pBox.min[v]){
						collide = false;
						break;
					}
				}
				if (collide){
					fixCollision(player[i], tower[j], pBox, sBox, touchGround1, touchGround2);
				}
				tower[j]->touchGround(touchGround2);
			}
			//player-player detection
			for (uint j = i + 1; j < player.size(); j++){
				bool touchGround2 = player[j]->getTouchGround();
				AABB pBox = player[i]->getAABB();
				AABB sBox = player[j]->getAABB();
				bool collide = true;
				for (int v = 0; v < 3; v++){
					if (pBox.max[v] <= sBox.min[v] || sBox.max[v] <= pBox.min[v]){
						collide = false;
						break;
					}
				}
				if (collide){
					fixCollision(player[i], player[j], pBox, sBox, touchGround1, touchGround2);
				}
				player[j]->touchGround(touchGround2);
			}
			player[i]->touchGround(touchGround1);
		}
	}

	void fixCollision(Object* obj1, Object* obj2, AABB& box1, AABB& box2, bool& onGround1, bool& onGround2){
		float Rewind[3];
		float minRewind = 999;
		int minID = 0;
		vec3 v1 = vec3(obj1->getModelM()*vec4(obj1->getVelocity(), 0.0));
		vec3 v2 = vec3(obj2->getModelM()*vec4(obj2->getVelocity(), 0.0));
		vec3 vDiff = v1 - v2;

		for (int v = 0; v<3; v++){
			if (vDiff[v] > 0){
				Rewind[v] = (box1.max[v] - box2.min[v]) / vDiff[v];
			}
			else if (vDiff[v] < 0){
				Rewind[v] = (box1.min[v] - box2.max[v]) / vDiff[v];
			}
			else{//avoid division by 0
				Rewind[v] = 999;
			}
			if (Rewind[v] < minRewind){
				minRewind = Rewind[v];
				minID = v;
			}
		}
		vec3 rwVelocity1 = vec3(0.0);
		rwVelocity1[minID] = -v1[minID];
		obj1->preTrans(glm::translate(rwVelocity1*minRewind));
		vec3 rwVelocity2 = vec3(0.0);
		rwVelocity2[minID] = -v2[minID];
		obj2->preTrans(glm::translate(rwVelocity2*minRewind));
		if (minID == 1){
			obj1->clearYVelocity();
			onGround1 = true;
			obj2->clearYVelocity();
			onGround2 = true;
		}
		else{//stair effect
			AABB b1 = obj1->getAABB();
			AABB b2 = obj2->getAABB();
			if (b2.max[1] > b1.min[1] && b2.max[1] - b1.min[1] <= 0.11){
				obj1->preTrans(glm::translate(glm::vec3(0.0f, b2.max[1]-b1.min[1]+0.01f, 0.0f)));
			}
		}
	}
	void addPlayer(Object* p){ player.push_back(p); }
	void addTower(Object* t){ tower.push_back(t); }
	void addStationary(Object* s){ stationary.push_back(s); }
	void addProjectile(Projectile* p){ projectile.push_back(p); }
	void setGravity(vec3& g){ gravity = g; }
	void resolvePlayerRotation(){
		for (uint i = 0; i < player.size(); i++){
			player[i]->postRotate(glm::rotate(mat4(1.0), player[i]->getPendingRote(), vec3(0, 1, 0)));
			player[i]->setPendingRot(0);
		}
	}
	void setCamM(int playerID, mat4 m){
		if (playerID >= 4){
			cout << playerID << endl;
			system("pause");
		}
		camM[playerID] = m;
	}
	void resolvePlayerTransition(float t){
		float extra_speed = t*GRAVITY_SCALE;
		for (uint i = 0; i < player.size(); i++){
			player[i]->addVelocity(gravity*extra_speed);
			player[i]->postTrans(glm::translate(player[i]->getVelocity()*t));
		}
	}
	void resolveTowerTransition(float t){
		float extra_speed = t*GRAVITY_SCALE;
		for (uint i = 0; i < tower.size(); i++){
			tower[i]->addVelocity(gravity*extra_speed);
			tower[i]->postTrans(glm::translate(tower[i]->getVelocity()*t));
		}
	}
	void resolveProjectileTransition(float t){
		float extra_speed = t*GRAVITY_SCALE;
		for (uint i = 0; i < projectile.size(); i++){
			projectile[i]->addVelocity(gravity*t);
			projectile[i]->postTrans(glm::translate(projectile[i]->getVelocity()*t));
		}
	}

	void setHMove(int playerID, int m){ getPlayerObj(playerID)->setHMove(m); }
	void cancelHMove(int playerID, int m){ getPlayerObj(playerID)->cancelHMove(m); }
	void setVMove(int playerID, int m){ getPlayerObj(playerID)->setVMove(m); }
	void cancelVMove(int playerID, int m){ getPlayerObj(playerID)->cancelVMove(m); }

	void setPendingRot(int playerID, float f){ getPlayerObj(playerID)->setPendingRot(f); }
	void pushRot(int playerID, float f){ getPlayerObj(playerID)->pushRot(f); }
	void jump(int playerID){ getPlayerObj(playerID)->jump(); }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////// START OF PLAYER ACTIONS /////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void respawnPlayer()
	{
		Object * holder;
		//int id;
		for (uint i = 0; i < player.size(); i++)
		{
			if (player[i]->getHealth() < 1)
			{

				holder = player[i];
				holder->setRespawn(holder->getRespawn() - 1);
				
				if (holder->getRespawn() < 1)
				{
					//Window::addDrawList(holder);
					//Window::addPlayerList(holder);
					//Window::respawnPlayer(holder->getName());
					holder->putHealth(7);
					holder->setModelM(holder->getAliveModelM());
					//cout << holder->getName() << " " << holder->getRespawn() << " " << holder->getHealth() << " " << player[i]->getHealth() << endl;

					//id = holder->getPlayerID();
					////insert the player at position at playerID
					////removes all players after playerID and appends them to the list.
					//for (int i = id; i < player.size() - 1; i++)
					//{
					//	holder = player[id];
					//	player.erase(player.begin() + id);
					//	player.push_back(holder);
					//}

					cout << "" << endl;
				}
			}
		}
	}
	void collisionDetectionProjectile(){
		//player-stationary detection
		for (uint i = 0; i < projectile.size(); i++)
		{
			bool touchGround1 = projectile[i]->getTouchGround();
			for (uint j = 0; j < player.size(); j++)
			{
				if (projectile[i]->getPlayerID() == player[j]->getPlayerID())
					continue;
				AABB pBox = projectile[i]->getAABB();
				AABB sBox = player[j]->getAABB();
				bool touchGround2 = player[j]->getTouchGround();
				bool collide = false;
				//[p][s]
				if ((pBox.max[2] <= sBox.max[2]) && (pBox.max[2] >= sBox.max[2] - 1) && (pBox.max[0] <= sBox.max[0]) && (pBox.max[0] >= sBox.max[0] - 1) && !(*projectile[i]).checkHit(player[j]->getPlayerID()))
				{
					player[j]->postTrans(glm::translate(vec3(1, 0.5, 0)));
					(*projectile[i]).setHit(player[j]->getPlayerID());
					damagePlayer(player[j]->getPlayerID(), projectile[i]->getPlayerID());
				}
				//[s][p]
				else if ((pBox.max[2] <= sBox.max[2]) && (pBox.max[2] >= sBox.max[2] - 1) && (pBox.max[0] <= sBox.max[0]) && (pBox.max[0] >= sBox.max[0] - 1) && !(*projectile[i]).checkHit(player[j]->getPlayerID()))
				{
					player[j]->postTrans(glm::translate(vec3(-1, 0.5, 0)));
					(*projectile[i]).setHit(player[j]->getPlayerID());
					damagePlayer(player[j]->getPlayerID(), projectile[i]->getPlayerID());
				}
				//[p]
				//[s]
				else if ((pBox.max[0] <= sBox.max[0] + 1) && (pBox.max[0] >= sBox.max[0]) && (pBox.max[2] <= sBox.max[2]) && (pBox.max[2] >= sBox.max[2]) && !(*projectile[i]).checkHit(player[j]->getPlayerID()))
				{
					player[j]->postTrans(glm::translate(vec3(0, 0.5, -1)));
					(*projectile[i]).setHit(player[j]->getPlayerID());
					damagePlayer(player[j]->getPlayerID(), projectile[i]->getPlayerID());
				}
				//[s]
				//[p]
				else if ((pBox.max[0] <= sBox.max[0] + 1) && (pBox.max[0] >= sBox.max[0]) && (pBox.max[2] <= sBox.max[2] + 1) && (pBox.max[2] >= sBox.max[2]) && !(*projectile[i]).checkHit(player[j]->getPlayerID()))
				{
					player[j]->postTrans(glm::translate(vec3(0, 0.5, 1)));
					(*projectile[i]).setHit(player[j]->getPlayerID());
					damagePlayer(player[j]->getPlayerID(), projectile[i]->getPlayerID());
				}
			}
		}
	}
	void damagePlayer(int targetId, int playerId)
	{
		Object * playerHolder = getPlayerObj(playerId);
		Object * targetHolder = getPlayerObj(targetId);
		targetHolder->setHealth(((RangeWeapon *)playerHolder->getWeapon())->getDamage());
		if (targetHolder->getHealth() < 1)
		{
			int dist, spd, dmg;
			dist = ((RangeWeapon *)playerHolder->getWeapon())->getDistance() * 2;
			spd = ((RangeWeapon *)playerHolder->getWeapon())->getSpeed() * 2;
			dmg = ((RangeWeapon *)playerHolder->getWeapon())->getDamage() * 2;
			//restricting speed and distance
			if (dist > MAX_DISTANCE)
				dist = MAX_DISTANCE;
			if (spd > MAX_SPEED)
				spd = MAX_SPEED;
			if (dmg < MAX_DAMAGE)
				dmg = MAX_DAMAGE;
			 
			targetHolder->setRespawn(RESPAWN_COUNTER);
			//Window::removeDrawList((*targetHolder).getName());
			//Window::removePlayerList((*targetHolder).getName());
			//respawn.push_back(targetHolder);
			for (uint i = 0; i < player.size(); i++)
			{
				if (player[i]->getPlayerID() == targetId)
				{
					player[i]->setAliveModelM(player[i]->getModelM());
					player[i]->setModelM(player[i]->getModelM()*glm::translate(vec3(0, 50, 0)));
				}
			}
			cout << playerId << " " << dmg << endl;
			RangeWeapon * newItem = new RangeWeapon(dist,
													spd,
													dmg);
			playerHolder->setWeapon(newItem);
			playerHolder->setKills(1);
			//RangeWeapon * newItem = new RangeWeapon(((RangeWeapon *)player[0]->getItem())->getDistance() * 3
			//										, ((RangeWeapon *)player[0]->getItem())->getSpeed() * 3);
		}
	}
	void basicAttack(int playerID)
	{
	  for (uint j = 0; j < player.size(); j++)
	    {
	    if (j == playerID)
	      continue;
	    AABB pBox = player[playerID]->getAABB();
	    AABB sBox = player[j]->getAABB();
	    vector<int> holder;	
	    //[p][s]
	    if ((pBox.max[2] <= sBox.max[2] + 5) && (pBox.max[2] >= sBox.max[2] - 5)  && pBox.max[0] == sBox.min[0])
	      {
		player[j]->postTrans(glm::translate(vec3(1, 0.5, 0)));
		damagePlayer(j, playerID);
	      }
	    //[s][p]
	    else if ((pBox.max[2] <= sBox.max[2] + 5) && (pBox.max[2] >= sBox.max[2] - 5) && pBox.min[0] == sBox.max[0])
	      {
		player[j]->postTrans(glm::translate(vec3(-1, 0.5, 0)));
		damagePlayer(j, playerID);
	      }
	    //[p]
	    //[s]
	    else if ((pBox.max[0] <= sBox.max[0] + 5) && (pBox.max[0] >= sBox.max[0] - 5) && pBox.min[2] == sBox.max[2])
	      {
		player[j]->postTrans(glm::translate(vec3(0, 0.5, -1)));
		damagePlayer(j, playerID);
	      }
	    //[s]
	    //[p]
	    else if ((pBox.max[0] <= sBox.max[0] + 5) && (pBox.max[0] >= sBox.max[0] - 5) && pBox.max[2] == sBox.min[2])
	      {
		player[j]->postTrans(glm::translate(vec3(0, 0.5, 1)));
		damagePlayer(j, playerID);
	      }
	    }
	  
	}
	void projectileAttack(int playerID, mat4 * cam)
	{
		mat4 test = *cam; //cam->getCamM();
		vec4 holder = test*vec4(0, 0, -1, 0); //orientation of camera in object space
		mat4 player1 = getPlayerObj(playerID)->getModelM();
		vec4 playerHolder = player1*vec4(0, 0, 0, 1);
		Cube * playerObject = ((Cube *)getPlayerObj(playerID));
		Object * playerHold = getPlayerObj(playerID);

		Projectile* cubeT = new Projectile(player.size() + respawn.size());
		cubeT->setKd(vec3(0.8, 0.0, 0.0));
		cubeT->setKa(vec3(0.3, 0.0, 0.0));
		cubeT->setKs(vec3(0.4, 0.0, 0.0));
		cubeT->setShininess(100);
		cubeT->setReflectFactor(vec2(0.2, 0.5));
		cubeT->setEta(0.5);
		cubeT->setCubeMapUnit(3);
		cubeT->setSpeed(5);
		//cubeT->postTrans(glm::translate(vec3(playerHolder[0] -2 + ((holder[0]) / 4), playerHolder[1], playerHolder[2] - (holder[2] / 4))));
		cubeT->setModelM(player1*glm::translate(vec3(0, 1, 0)));//get the new cube matrix by translating the player0 matrix forward in player0 object space. This way the new matrix will inherit player0 oriantation 
		cubeT->setAABB(AABB(vec3(-0.5, -0.5, -0.5), vec3(0.5, 0.5, 0.5)));
		AABB hold = cubeT->getAABB();
		cubeT->setStartX(hold.max[0]);
		cubeT->setStartY(hold.max[2]);
		cubeT->setPlayerID(playerID);

		//Name and type
		cubeT->setType("Cube");
		cubeT->setName("Test Cube" + std::to_string(projectile_counter));
		projectile_counter++;
		cubeT->setDistance(((RangeWeapon *)(playerHold)->getWeapon())->getDistance());
		//cout << cubeT->getPlayerID() << " " << cubeT->getDistance() << " " << cubeT->getSpeed() << " " << ((RangeWeapon *)playerHold->getWeapon())->getDamage() << " " << endl;
		//Add Cube to the draw list
		////////////////////////////////////////////////////////Window::addDrawList(cubeT);
		projectile.push_back(cubeT);
		cubeT->setSpeed(50);
		//cubeT->setHMove((holder[0] / 4));
		cubeT->setVelocity(vec3(holder)*((RangeWeapon *)playerHold->getWeapon())->getSpeed());// set object space velocity to camera oriantation in object space. Since camera always have the same xz oriantation as the object, xz oriantation wouldnt change when camera rotate.
		//cubeT->setVMove(1);  //do this if you want the cube to not have vertical velocity. uncomment the above setVelocity.
		//cout << holder[0] << ' ' << holder[1] << ' ' << holder[2] << ' ' << playerHolder[0] << ' ' << playerHolder[2] << endl;
	}
	void despawnProjectile()
	{
		for (uint i = 0; i < projectile.size(); i++)
		{
			float startX = projectile[i]->getStartX();
			float startY = projectile[i]->getStartY();
			AABB curr = projectile[i]->getAABB();
			int distance = sqrt(pow(curr.max[0] - startX, 2) + pow(curr.max[2] - startY, 2));//Pythagorean Theorem

			//cout << startX << " " << curr.max[0] << " " << curr.max[0] - startX << " " << distance << endl;
			if (distance >= (*projectile[i]).getDistance())
			{
				////////////////////////////////////////////////Window::removeDrawList((*projectile[i]).getName());
				projectile.erase(projectile.begin() + i);
			}
		}
	}
	void recover(int playerId)
	{
		getPlayerObj(playerId)->jump(20);
	}
	void rechargeJump()
	{
		for (uint i = 0; i < player.size(); i++)
		{
			if (player[i]->getTouchGround() && player[i]->getNumJumps() < player[i]->getTotalJumps())
				player[i]->incNumJumps();
		}
	}
	void takeItem()
	{

	}
	void resetVelocity(int playerId)
	{
		player[playerId]->resetVelocity();
	}
	int numPlayers()
	{
		return player.size();
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////// END OF PLAYER ACTIONS ///////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	boost::array<mat4, 4> getPlayerMats(){
		boost::array<mat4, 4> m;
		assert(player.size() == 4);
		for (uint i = 0; i < 4; i++){
			m[i] = player[i]->getModelM();
		}
		return m;
	}
	Object * getPlayerElement(int i)
	{
		return player[i];
	}
	Object * getPlayer(int i)
	{
		return getPlayerObj(i);
	}
	boost::array<mat4, 2> getTowerMats(){
		boost::array<mat4, 2> m;
		assert(tower.size() == 2);
		for (uint i = 0; i < 2; i++){
			m[i] = tower[i]->getModelM();
		}
		return m;
	}
	void initialize(){


		/*Cube* cube0 = new Cube();
		cube0->setSpeed(5);
		cube0->postTrans(glm::translate(vec3(0, 0.5, 7)));
		cube0->setAABB(AABB(vec3(-0.5, -0.5, -0.5), vec3(0.5, 0.5, 0.5)));
		cube0->setType("Cube");
		cube0->setName("Test cube0");
		addPlayer(cube0);

		Cube* cube1 = new Cube();
		cube1->setSpeed(5);
		cube1->postTrans(glm::translate(vec3(0, 0.5, 7)));
		cube1->setAABB(AABB(vec3(-0.5, -0.5, -0.5), vec3(0.5, 0.5, 0.5)));
		cube1->setType("Cube");
		cube1->setName("Test cube1");
		addPlayer(cube1);

		Cube* cube2 = new Cube();
		cube2->setSpeed(5);
		cube2->postTrans(glm::translate(vec3(0, 0.5, 7)));
		cube2->setAABB(AABB(vec3(-0.5, -0.5, -0.5), vec3(0.5, 0.5, 0.5)));
		cube2->setType("Cube");
		cube2->setName("Test cube2");
		addPlayer(cube2);

		Cube* cube3 = new Cube();
		cube3->setSpeed(5);
		cube3->postTrans(glm::translate(vec3(0, 0.5, 7)));
		cube3->setAABB(AABB(vec3(-0.5, -0.5, -0.5), vec3(0.5, 0.5, 0.5)));
		cube3->setType("Cube");
		cube3->setName("Test cube3");
		addPlayer(cube3);*/

		MD5Model* md50 = new MD5Model();
		md50->setSpeed(PLAYER_SPEED);
		md50->postTrans(glm::translate(vec3(-10, 0.5, 25)));
		md50->setAABB(AABB(vec3(-0.25, 0.0, -0.25), vec3(0.25, 1.5, 0.25)));
		md50->setType("Model");
		md50->setName("Player Model0");
		md50->setPlayerID(0);
		addPlayer(md50);

		MD5Model* md51 = new MD5Model();
		md51->setSpeed(PLAYER_SPEED);
		md51->postTrans(glm::translate(vec3(-10, 0.5, -25)));
		md51->postRotate(glm::rotate(mat4(1.0), 180.0f, vec3(0, 1, 0)));
		md51->setAABB(AABB(vec3(-0.25, 0.0, -0.25), vec3(0.25, 1.5, 0.25)));
		md51->setType("Model");
		md51->setName("Player Model1");
		md51->setPlayerID(1);
		addPlayer(md51);

		MD5Model* md52 = new MD5Model();
		md52->setSpeed(PLAYER_SPEED);
		md52->postTrans(glm::translate(vec3(10, 0.5, 25)));
		md52->setAABB(AABB(vec3(-0.25, 0.0, -0.25), vec3(0.25, 1.5, 0.25)));
		md52->setType("Model");
		md52->setName("Player Model2");
		md52->setPlayerID(2);
		addPlayer(md52);

		MD5Model* md53 = new MD5Model();
		md53->setSpeed(PLAYER_SPEED);
		md53->postTrans(glm::translate(vec3(10, 0.5, -25)));
		md53->postRotate(glm::rotate(mat4(1.0), 180.0f, vec3(0, 1, 0)));
		md53->setAABB(AABB(vec3(-0.25, 0.0, -0.25), vec3(0.25, 1.5, 0.25)));
		md53->setType("Model");
		md53->setName("Player Model3");
		md53->setPlayerID(3);
		addPlayer(md53);

		//triplet tower
		MD5Model* tw0 = new MD5Model();
		tw0->postTrans(glm::translate(vec3(0, 0, -25)));
		tw0->setAABB(AABB(vec3(-0.7, 0.75, -0.7), vec3(0.7, 3.75, 0.7)));
		tw0->setType("Model");
		tw0->setName("Tower Model0");
		tw0->setPlayerID(0);
		addTower(tw0);

		//pctopus tower
		MD5Model* tw1 = new MD5Model();
		tw1->postTrans(glm::translate(vec3(0, 0, 25)));
		tw1->setAABB(AABB(vec3(-0.7, 0.6, -0.7), vec3(0.7, 4.79, 0.7)));
		tw1->setType("Model");
		tw1->setName("Tower Model1");
		tw1->setPlayerID(1);
		addTower(tw1);

		Ground* ground = new Ground();
		ground->loadColorTex("img/moon_tex/moon_COLOR.png", "PNG");
		ground->loadDispTex("img/moon_tex/moon_DISP.png", "PNG");
		ground->loadNormalTex("img/moon_tex/moon_NRM.png", "PNG");
		ground->loadOccTex("img/moon_tex/moon_OCC.png", "PNG");
		ground->loadSpecTex("img/moon_tex/moon_SPEC.png", "PNG");
		ground->setDimensionS(200);
		ground->setDimensionT(200);
		ground->setRow(100);
		ground->setColumn(100);
		ground->setHeight(6);
		ground->generate();
		ground->setType("Ground");
		ground->setName("Ground");
		addStationary(ground);

		//Bottom Mid Platform
		Cube* platform_01 = new Cube(-10.0, 10.0, -0.5, 0.5, -10.0, 10.0);
		//platform_01->setSpeed(5);
		platform_01->postTrans(glm::translate(vec3(0, 13.0, 0)));
		platform_01->setAABB(AABB(vec3(-10.0, -0.5, -10.0), vec3(10.0, 0.5, 10.0)));
		platform_01->setType("Cube");
		platform_01->setName("Test Platform");
		addStationary(platform_01);

		//1st Bottom Side Step Platform
		Cube* platform_02 = new Cube(-1.5, 1.5, -0.5, 0.5, -5.0, 5.0);
		//platform_01->setSpeed(5);
		platform_02->postTrans(glm::translate(vec3(20.0, 8.0, 0)));
		platform_02->setAABB(AABB(vec3(-1.5, -0.5, -5.0), vec3(1.5, 0.5, 5.0)));
		platform_02->setType("Cube");
		platform_02->setName("Test Platform");
		addStationary(platform_02);

		//2nd Bottom Side Step Platform
		Cube* platform_03 = new Cube(-1.5, 1.5, -0.5, 0.5, -5.0, 5.0);
		//platform_01->setSpeed(5);
		platform_03->postTrans(glm::translate(vec3(-20.0, 8.0, 0)));
		platform_03->setAABB(AABB(vec3(-1.5, -0.5, -5.0), vec3(1.5, 0.5, 5.0)));
		platform_03->setType("Cube");
		platform_03->setName("Test Platform");
		addStationary(platform_03);

		//Platform Steps 1-1
		Cube* platform_04 = new Cube(-5.0, 5.0, -0.5, 0.5, -1.5, 1.5);
		//platform_01->setSpeed(5);
		platform_04->postTrans(glm::translate(vec3(0.0, 18.0, 20.0)));
		platform_04->setAABB(AABB(vec3(-5.0, -0.5, -1.5), vec3(5.0, 0.5, 1.5)));
		platform_04->setType("Cube");
		platform_04->setName("Test Platform");
		addStationary(platform_04);

		//Platform Steps 1-2
		Cube* platform_05 = new Cube(-5.0, 5.0, -0.5, 0.5, -5.0, 5.0);
		//platform_01->setSpeed(5);
		platform_05->postTrans(glm::translate(vec3(0.0, 23.0, 40.0)));
		platform_05->setAABB(AABB(vec3(-5.0, -0.5, -5.0), vec3(5.0, 0.5, 5.0)));
		platform_05->setType("Cube");
		platform_05->setName("Test Platform");
		addStationary(platform_05);

		//Platform Steps 1-3
		Cube* platform_06 = new Cube(-5.0, 5.0, -0.5, 0.5, -1.5, 1.5);
		//platform_01->setSpeed(5);
		platform_06->postTrans(glm::translate(vec3(0.0, 28.0, 60.0)));
		platform_06->setAABB(AABB(vec3(-5.0, -0.5, -1.5), vec3(5.0, 0.5, 1.5)));
		platform_06->setType("Cube");
		platform_06->setName("Test Platform");
		addStationary(platform_06);

		//Platform Steps 2-1
		Cube* platform_07 = new Cube(-5.0, 5.0, -0.5, 0.5, -1.5, 1.5);
		//platform_01->setSpeed(5);
		platform_07->postTrans(glm::translate(vec3(0.0, 18.0, -20.0)));
		platform_07->setAABB(AABB(vec3(-5.0, -0.5, -1.5), vec3(5.0, 0.5, 1.5)));
		platform_07->setType("Cube");
		platform_07->setName("Test Platform");
		addStationary(platform_07);

		//Platform Steps 2-2
		Cube* platform_08 = new Cube(-5.0, 5.0, -0.5, 0.5, -5.0, 5.0);
		//platform_01->setSpeed(5);
		platform_08->postTrans(glm::translate(vec3(0.0, 23.0, -40.0)));
		platform_08->setAABB(AABB(vec3(-5.0, -0.5, -5.0), vec3(5.0, 0.5, 5.0)));
		platform_08->setType("Cube");
		platform_08->setName("Test Platform");
		addStationary(platform_08);

		//Platform Steps 2-3
		Cube* platform_09 = new Cube(-5.0, 5.0, -0.5, 0.5, -1.5, 1.5);
		//platform_01->setSpeed(5);
		platform_09->postTrans(glm::translate(vec3(0.0, 28.0, -60.0)));
		platform_09->setAABB(AABB(vec3(-5.0, -0.5, -1.5), vec3(5.0, 0.5, 1.5)));
		platform_09->setType("Cube");
		platform_09->setName("Test Platform");
		addStationary(platform_09);

		//m_pMesh2 = new Mesh();
		//m_pMesh2->LoadMesh("Model/monky_04_27_smooth.dae");
		//m_pMesh2->setShader(sdrCtl.getShader("basic_model"));
		//m_pMesh2->setAdjustM(glm::translate(vec3(0.0, 4.1, 0.0))*glm::rotate(mat4(1.0), 90.0f, vec3(-1.0, 0, 0))*glm::scale(vec3(0.2, 0.2, 0.2)));
		//m_pMesh2->setAABB();

		/*MD5Model* md5 = new MD5Model();
		md5->setSpeed(5);
		md5->setAABB(AABB(vec3(-25.25, 0.0, -10.25), vec3(0.25, 1.5, 0.25)));
		md5->setType("Model");
		md5->setName("Player Model");
		//addPlayer(md5);*/

		//md6 = new MD5Model();
		//md6->LoadModel("Model/fleurOptonl.md5mesh");
		//md6->LoadAnim("Model/fleurOptonl.md5anim");
		//md6->setShader(sdrCtl.getShader("basic_texture"));
		//md6->setShininess(30);
		//md6->setAdjustM(glm::translate(vec3(0.0, 1.7, 0.0))*glm::rotate(mat4(1.0), 180.0f, vec3(0.0, 1, 0))*glm::rotate(mat4(1.0), 90.0f, vec3(-1.0, 0, 0))*glm::scale(vec3(0.05, 0.05, 0.05)));
		//md6->setModelM(glm::translate(vec3(1.0, 0.0, 0.0)));
		//md6->setType("Model");
		//md6->setName("Player Model");




		counter = 0;
		projectile_counter = 0;
	}

protected:
	vector<Object*> stationary;
	vector<Object*> player;
	vector<Object*> respawn;
	vector<Object*> tower;
	vector<Object*> skillShot;
	vector<Object*> virtualTower;
	vector<Projectile*> projectile;
	vector<Item *> items;
	vector<mat4> camM;
	vec3 gravity;
	vector<vector<int>> prevAttacked;//first element is playerID, second is axis
	int counter;
	int projectile_counter;
};

