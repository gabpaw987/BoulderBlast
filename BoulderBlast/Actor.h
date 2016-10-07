#ifndef ACTOR_H_
#define ACTOR_H_

#include "GraphObject.h"
#include "StudentWorld.h"

class Actor :public GraphObject
{
public:
	Actor(StudentWorld* studentWorld, int imageID, int startX, int startY, bool isVisible = true, GraphObject::Direction dir = GraphObject::none);
	virtual ~Actor() {}
	virtual void doSomething() {};
	StudentWorld* getStudentWorld() const;

private:
	StudentWorld* m_studentWorld;
};

class Wall :public Actor
{
public:
	Wall(StudentWorld* studentWorld, int startX, int startY) : Actor(studentWorld, IID_WALL, startX, startY) {}
};

class DestructableActor : public Actor
{
public:
	DestructableActor(StudentWorld* studentWorld, int imageID, int startX, int startY, int hp = 1, GraphObject::Direction dir = GraphObject::none)
		: Actor(studentWorld, imageID, startX, startY, true, dir), m_hp(hp), m_isAlive(true) {}
	int getHp() const;
	void setHp(int hp);
	bool isAlive() const;
	void setDead();
	bool offsetCoordinatesInDirection(int &x, int &y, GraphObject::Direction dir) const;
	virtual void isAttacked();

private:
	int m_hp;
	bool m_isAlive;
};

class Player : public DestructableActor
{
public:
	Player(StudentWorld* studentWorld, int startX, int startY)
		: DestructableActor(studentWorld, IID_PLAYER, startX, startY, 20, GraphObject::right), m_ammunition(20) {}
	virtual void doSomething();
	int getAmmunition() const;
	void increaseAmmunition(int amount);

private:
	int m_ammunition;
};

class Boulder : public DestructableActor
{
public:
	Boulder(StudentWorld* studentWorld, int startX, int startY)
		: DestructableActor(studentWorld, IID_BOULDER, startX, startY, 10) {}
	bool isPushed(GraphObject::Direction dir);
};

class Hole : public DestructableActor
{
public:
	Hole(StudentWorld* studentWorld, int startX, int startY)
		: DestructableActor(studentWorld, IID_HOLE, startX, startY) {}
};

class Bullet : public DestructableActor
{
public:
	Bullet(StudentWorld* studentWorld, int startX, int startY, GraphObject::Direction dir)
		: DestructableActor(studentWorld, IID_BULLET, startX, startY, 1, dir) {}
	virtual void doSomething();
private:
	bool hitTest(const int& x, const int& y);
};

class Exit : public Actor
{
public:
	Exit(StudentWorld* studentWorld, int startX, int startY)
		: Actor(studentWorld, IID_EXIT, startX, startY, false) {}
	virtual void doSomething();
};

class Goodie : public DestructableActor
{
public:
	Goodie(StudentWorld* studentWorld, int imageID, int startX, int startY)
		: DestructableActor(studentWorld, imageID, startX, startY) {}
	virtual void doSomething();
};

class Jewel : public Goodie
{
public:
	Jewel(StudentWorld* studentWorld, int startX, int startY)
		: Goodie(studentWorld, IID_JEWEL, startX, startY) {}
	virtual void doSomething();
};

class ExtraLifeGoodie : public Goodie
{
public:
	ExtraLifeGoodie(StudentWorld* studentWorld, int startX, int startY)
		: Goodie(studentWorld, IID_EXTRA_LIFE, startX, startY) {}
	virtual void doSomething();
};

class RestoreHealthGoodie : public Goodie
{
public:
	RestoreHealthGoodie(StudentWorld* studentWorld, int startX, int startY)
		: Goodie(studentWorld, IID_RESTORE_HEALTH, startX, startY) {}
	virtual void doSomething();
};

class AmmoGoodie : public Goodie
{
public:
	AmmoGoodie(StudentWorld* studentWorld, int startX, int startY)
		: Goodie(studentWorld, IID_AMMO, startX, startY) {}
	virtual void doSomething();
};

class Robot : public DestructableActor
{
public:
	Robot(StudentWorld* studentWorld, int imageID, int startX, int startY, int hp, GraphObject::Direction dir);

	virtual void doSomething() = 0;
	virtual bool attack() const;
	
	int getCurrentTick() const;
	bool incCurrentTick();

	virtual bool fieldContainsObstruction(int x, int y, bool forBullet) const;
	bool isCurrentlyFacingPlayer() const;

	virtual void isAttacked();

private:
	int m_currentTick;
	int m_maxTicks;
};

class SnarlBot : public Robot
{
public:
	SnarlBot(StudentWorld* studentWorld, int startX, int startY, GraphObject::Direction dir)
		: Robot(studentWorld, IID_SNARLBOT, startX, startY, 10, dir) {}
	virtual void doSomething();
	virtual void isAttacked();
};

class KleptoBot : public Robot
{
public:
	KleptoBot(StudentWorld* studentWorld, int startX, int startY, bool isForAngryKleptoBot = false)
		: Robot(studentWorld, isForAngryKleptoBot ? IID_ANGRY_KLEPTOBOT : IID_KLEPTOBOT, startX, startY, isForAngryKleptoBot ? 8 : 5, GraphObject::right),
		m_movingDistance(1 + (rand() % 6)), m_noOfMoves(0), m_goodie("") {}
	virtual void doSomething();
	virtual bool attack() { return false; }
	virtual void isAttacked();

private:
	GraphObject::Direction getDirectionFromInt(int dirInt);

private:
	int m_movingDistance;
	int m_noOfMoves;
	string m_goodie;
};

class AngryKleptoBot : public KleptoBot
{
public:
	AngryKleptoBot(StudentWorld* studentWorld, int startX, int startY)
		: KleptoBot(studentWorld, startX, startY, true) {}
	virtual bool attack() { return Robot::attack(); };
	virtual void isAttacked();
};


class KleptoBotFactory : public Actor
{
public:
	KleptoBotFactory(StudentWorld* studentWorld, int startX, int startY, bool producesAngryKleptoBots)
		: Actor(studentWorld, IID_ROBOT_FACTORY, startX, startY), m_producesAngryKleptoBots(producesAngryKleptoBots) {}
	virtual void doSomething();

private:
	bool m_producesAngryKleptoBots;
};

#endif // ACTOR_H_
