#ifndef STUDENTWORLD_H_
#define STUDENTWORLD_H_

#include "GameWorld.h"
#include "GameConstants.h"
#include <string>
#include <list>
using namespace std;

class Actor;
class Player;

class StudentWorld : public GameWorld
{
public:
	StudentWorld(string assetDir)
		: GameWorld(assetDir), m_player(nullptr), m_actors(), m_bonus(1000), m_isLevelCompleted(false) { }
	~StudentWorld();

	virtual int init();
	virtual int move();
	virtual void cleanUp();
	
	Player* getPlayer() const;
	list<Actor*> getActorsAt(int x, int y);
	
	void insertActor(Actor* actor);
	void setLevelCompleted();

private:
	void setDisplayText();

private:
	Player* m_player;
	list<Actor*> m_actors;
	int m_bonus;
	bool m_isLevelCompleted;
};

#endif // STUDENTWORLD_H_