#include "Actor.h"
#include "GameConstants.h"

//===============================================================================================
// Actor
//===============================================================================================

Actor::Actor(StudentWorld* studentWorld, int imageID, int startX, int startY, bool isVisible, GraphObject::Direction dir)
: GraphObject(imageID, startX, startY, dir), m_studentWorld(studentWorld)
{
	setVisible(isVisible);
}

StudentWorld* Actor::getStudentWorld() const
{
	return m_studentWorld;
}

//===============================================================================================
// DestructableActor
//===============================================================================================

int DestructableActor::getHp() const
{
	return m_hp;
}

void DestructableActor::setHp(int hp)
{
	 m_hp = hp;
}

bool DestructableActor::isAlive() const
{
	return m_isAlive;
}

void DestructableActor::setDead()
{
	m_isAlive = false;
	setVisible(false);
}

void DestructableActor::isAttacked()
{
	m_hp -= 2;
	if (m_hp <= 0)
		setDead();
}

bool DestructableActor::offsetCoordinatesInDirection(int &x, int &y, GraphObject::Direction dir) const
{
	bool hasSucceeded = false;
	//offset the direction (by incrementing the appropritate value x or y) into the direction given
	switch (dir)
	{
	case GraphObject::up:
		if (y + 1 < VIEW_WIDTH)
		{
			y++;
			hasSucceeded = true;
		}
		break;
	case GraphObject::right:
		if (x + 1 < VIEW_HEIGHT)
		{
			x++;
			hasSucceeded = true;
		}
		break;
	case GraphObject::down:
		if (y - 1 >= 0)
		{
			y--;
			hasSucceeded = true;
		}
		break;
	case GraphObject::left:
		if (x - 1 >= 0)
		{
			x--;
			hasSucceeded = true;
		}
		break;
	}

	//if the attempted offset is within the range of the entire game field, this will return true
	return hasSucceeded;
}

//===============================================================================================
// Player
//===============================================================================================

void Player::doSomething()
{
	//if the player is still alive
	if (!isAlive())
		return;

	//Check if the user pressed a key
	int keyPressed = 0;
	if (getStudentWorld()->getKey(keyPressed))
	{
		bool isMove = false;
		int x = getX(), y = getY();

		switch (keyPressed)
		{
			//if the user pressed escape, make the player suicide
		case KEY_PRESS_ESCAPE:
			setDead();
			break;
			//if the user pressed space, make the player shoot a bullet if there is enough ammo left
		case KEY_PRESS_SPACE:
			if (m_ammunition > 0)
			{
				m_ammunition--;
				if (offsetCoordinatesInDirection(x, y, getDirection()))
					getStudentWorld()->insertActor(new Bullet(getStudentWorld(), x, y, getDirection()));
				getStudentWorld()->playSound(SOUND_PLAYER_FIRE);
			}
			break;
			//If the player pressed up, right, down, or left, set the direction accordingly and record that the player wants to move
		case KEY_PRESS_UP:
			setDirection(GraphObject::up);
			isMove = true;
			break;
		case KEY_PRESS_RIGHT:
			setDirection(GraphObject::right);
			isMove = true;
			break;
		case KEY_PRESS_DOWN:
			setDirection(GraphObject::down);
			isMove = true;
			break;
		case KEY_PRESS_LEFT:
			setDirection(GraphObject::left);
			isMove = true;
			break;
		}

		//If the player wants to move, try to execute the move
		if (isMove)
		{
			x = getX(), y = getY();
			if (offsetCoordinatesInDirection(x, y, getDirection()))
			{
				list<Actor*> actorsFound = getStudentWorld()->getActorsAt(x, y);
				//If there are no actors on the field that the user wants to move to, move the player there
				if (actorsFound.empty())
					moveTo(x, y);
				else
				{
					bool shallMove = true;
					for (list<Actor*>::iterator i = actorsFound.begin(); i != actorsFound.end(); i++)
					{
						//if there is a boulder, try to push the boulder and move the player if it worked
						Boulder* b = dynamic_cast<Boulder*>(*i);
						if (b != nullptr)
						{
							if (b->isPushed(getDirection()))
								moveTo(x, y);
							shallMove = false;
						}
						//if there is either a wall, kleptobot factory, hole or any robot, the player cannot move there
						else if (dynamic_cast<Wall*>(*i) != nullptr ||
							dynamic_cast<KleptoBotFactory*>(*i) != nullptr ||
							dynamic_cast<Robot*>(*i) != nullptr ||
							dynamic_cast<Hole*>(*i) != nullptr)
						{
							shallMove = false;
						}
					}
					//if there is only another actor that is not relevant, move the player
					if (shallMove)
						moveTo(x, y);
				}
			}
		}
	}
}

int Player::getAmmunition() const
{
	return m_ammunition;
}

void Player::increaseAmmunition(int amount)
{
	m_ammunition += amount;
}

//===============================================================================================
// Boulder
//===============================================================================================

bool Boulder::isPushed(GraphObject::Direction dir)
{

	//if there is no obstruction, move boulder one field to the front
	int x = getX(), y = getY();
	offsetCoordinatesInDirection(x, y, dir);
	list<Actor*> actorsFound = getStudentWorld()->getActorsAt(x, y);
	if (actorsFound.empty())
	{
		moveTo(x, y);
		return true;
	}
	else
	{
		//if there is an obstruction
		for (list<Actor*>::iterator i = actorsFound.begin(); i != actorsFound.end(); i++)
		{
			//if the obstruction is a hole, attack and kill it
			Hole* h = dynamic_cast<Hole*>(*i);
			if (h != nullptr)
			{
				h->isAttacked();
				moveTo(x, y);
				setDead();
				return true;
			}
		}
		//otherwise, do not move or do anything
	}
	
	//if this is reached the boulder could not be moved in that way
	return false;
}

//===============================================================================================
// Bullet
//===============================================================================================

void Bullet::doSomething()
{
	//If bullet is alive
	if (!isAlive())
		return;

	int x = getX(), y = getY();

	//Check if it hits an actor at its current position
	if (hitTest(x, y))
		return;

	//If not, move it one field to the front
	if (offsetCoordinatesInDirection(x, y, getDirection()))
		moveTo(x, y);
	else
		isAttacked();

	//Check again if the bullet hits something at that new field
	hitTest(x, y);
}

bool Bullet::hitTest(const int& x, const int& y)
{
	//get all actors at this spot and for each
	list<Actor*> actorsFound = getStudentWorld()->getActorsAt(x, y);

	if (!actorsFound.empty())
	{
		for (list<Actor*>::iterator i = actorsFound.begin(); i != actorsFound.end(); i++)
		{
			DestructableActor* da = dynamic_cast<DestructableActor*>(*i);

			//If it is a player, boulder, or robot, attack it and kill the bullet itself
			if (da != nullptr)
			{
				if (dynamic_cast<Player*>(*i) != nullptr ||
					dynamic_cast<Robot*>(*i) != nullptr ||
					dynamic_cast<Boulder*>(*i) != nullptr)
				{
					da->isAttacked();
					isAttacked();
					return true;
				}
			}
			//else if there is a wall or a factory, only kill the bullet itself and deal no damage to the other actor
			//Note: KleptoBots always come before their factories in my list, since I push them to the front
			//      that is why this still also attack a kleptobot on top of a factory
			else if (dynamic_cast<Wall*>(*i) != nullptr ||
					 dynamic_cast<KleptoBotFactory*>(*i) != nullptr)
			{
				isAttacked();
				return true;
			}
		}
	}
	//otherwise nothing was hit at that field
	return false;
}

//===============================================================================================
// Exit
//===============================================================================================

void Exit::doSomething()
{
	//If exit is visible and player stands on top of it
	StudentWorld* studentWorld = getStudentWorld();
	if (isVisible() && studentWorld->getPlayer()->getX() == getX() && studentWorld->getPlayer()->getY() == getY())
	{
		//User won the game
		studentWorld->playSound(SOUND_FINISHED_LEVEL);
		studentWorld->setLevelCompleted();
	}
}

//===============================================================================================
// Goodie
//===============================================================================================

void Goodie::doSomething()
{
	//If player stands on top of it
	StudentWorld* studentWorld = getStudentWorld();
	if (studentWorld->getPlayer()->getX() == getX() && studentWorld->getPlayer()->getY() == getY())
	{
		//"Pick it up" by killing it and playing the appropriate sound
		isAttacked();
		studentWorld->playSound(SOUND_GOT_GOODIE);
	}
}

//===============================================================================================
// Jewel
//===============================================================================================

void Jewel::doSomething()
{
	//If jewel is still alive
	if (!isAlive())
		return;

	//try to "pick it up"
	Goodie::doSomething();

	//And increase user score by 50 it was picked up
	if (!isAlive())
	{
		getStudentWorld()->increaseScore(50);
	}
}

//===============================================================================================
// ExtraLifeGoodie
//===============================================================================================

void ExtraLifeGoodie::doSomething()
{
	//If ExtraLifeGoodie is still alive
	if (!isAlive())
		return;

	//Try to "pick it up"
	Goodie::doSomething();

	//and increase user score by 1000 and its lives by 1, if it was picked up
	if (!isAlive())
	{
		getStudentWorld()->increaseScore(1000);
		getStudentWorld()->incLives();
	}
}

//===============================================================================================
// RestoreHealthGoodie
//===============================================================================================

void RestoreHealthGoodie::doSomething()
{
	//if RestoreHealthGoodie is still alive
	if (!isAlive())
		return;

	//try to "pick it up"
	Goodie::doSomething();

	//if it was picked up, increase user score by 500 and restore player hp back to 20
	if (!isAlive())
	{
		getStudentWorld()->increaseScore(500);
		getStudentWorld()->getPlayer()->setHp(20);
	}
}

//===============================================================================================
// AmmoGoodie
//===============================================================================================

void AmmoGoodie::doSomething()
{
	//if AmmoGoodie is still alive
	if (!isAlive())
		return;

	//try to "pick it up"
	Goodie::doSomething();

	//and increase user score by 100 and the player's ammunition by 20 if it was picked up
	if (!isAlive())
	{
		getStudentWorld()->increaseScore(100);
		getStudentWorld()->getPlayer()->increaseAmmunition(20);
	}
}

//===============================================================================================
// Robot
//===============================================================================================

Robot::Robot(StudentWorld* studentWorld, int imageID, int startX, int startY, int hp, GraphObject::Direction dir)
: DestructableActor(studentWorld, imageID, startX, startY, hp, dir), m_currentTick(1)
{
	//Calculate the amount of ticks, depending on the current level number
	//that this robot always has to wait before it does anything
	m_maxTicks = (28 - getStudentWorld()->getLevel()) / 4;
	if (m_maxTicks < 3)
		m_maxTicks = 3;
}

int Robot::getCurrentTick() const
{
	return m_currentTick;
}

bool Robot::incCurrentTick()
{
	//If currentTick is one, robot should do something and currentTicks should be incremented
	if (m_currentTick == 1)
	{
		m_currentTick++;
		return true;
	}
	//if currentTick is at maxTicks, set it back to 1
	else if (m_currentTick == m_maxTicks)
		m_currentTick = 1;
	//otherwise just increment m_currentTicks
	else
		m_currentTick++;

	//if it was not 1, the robot should not do anything
	return false;
}

bool Robot::fieldContainsObstruction(int x, int y, bool forBullet) const
{
	//get all actors at this field
	list<Actor*> actorsFound = getStudentWorld()->getActorsAt(x, y);

	if (!actorsFound.empty())
	{
		//for each
		for (list<Actor*>::iterator i = actorsFound.begin(); i != actorsFound.end(); i++)
		{
			//if it is a wall, robot, factory or boulder, there is an obstruction for anything
			Actor* actorFound = *i;
			if (dynamic_cast<Wall*>(actorFound) != nullptr ||
				dynamic_cast<Robot*>(actorFound) != nullptr ||
				dynamic_cast<KleptoBotFactory*>(actorFound) != nullptr ||
				dynamic_cast<Boulder*>(actorFound) != nullptr)
			{
				return true;
			}
			//if the function is asked to check for obstructions for a robot itself and not just
			//for bullets, the player and a hole also count as obstructions the robot cannot move onto
			if (!forBullet)
			{
				if (dynamic_cast<Player*>(actorFound) != nullptr ||
					dynamic_cast<Hole*>(actorFound) != nullptr)
				{
					return true;
				}
			}
		}
	}

	//else there is no obstruction
	return false;
}

bool Robot::isCurrentlyFacingPlayer() const
{
	Player* player = getStudentWorld()->getPlayer();

	int distance = 0;
	GraphObject::Direction faceDirection = getDirection();

	int x = getX(), y = getY();

	//If the robot is facing right and is to the left of the player
	//or if the robot is facing left and is to the right of the player
	if ((faceDirection == GraphObject::right && player->getY() == y && player->getX() > x) ||
		(faceDirection == GraphObject::left && player->getY() == y && player->getX() < x))
	{
		//Take x difference as distance
		distance = player->getX() - x;
	}
	//If the robot is facing up and is below of the player
	//or if the robot is facing down and is above of the player
	else if ((faceDirection == GraphObject::up && player->getX() == x && player->getY() > y) ||
		(faceDirection == GraphObject::down && player->getX() == x && player->getY() < y))
	{
		//Take y difference as distance
		distance = player->getY() - y;
	}

	//Make distance positive to loop through
	bool isNegativeDistance = false;

	if (distance < 0)
	{
		distance *= (-1);
		isNegativeDistance = true;
	}

	//For each field between the robot and the player
	for (int i = 0; i < distance; i++)
	{
		bool containsObstruction = false;

		//Make the distance negative again to get the correct delta
		int realI = i + 1;
		if (isNegativeDistance)
			realI *= (-1);

		//if that field contains an obstruction
		if (faceDirection == GraphObject::right || faceDirection == GraphObject::left)
			containsObstruction = fieldContainsObstruction(x + realI, y, true);
		else
			containsObstruction = fieldContainsObstruction(x, y + realI, true);

		//reutrn false
		if (containsObstruction)
			return false;
	}

	//otherwise, return true
	if (distance != 0)
		return true;
	else
		return false;
}

bool Robot::attack() const
{
	//If the robot is in the same row/column as the player and is facing it without any obstructions
	Player* player = getStudentWorld()->getPlayer();
	if (((player->getY() == getY() && (getDirection() == GraphObject::right || getDirection() == GraphObject::left)) ||
		(player->getX() == getY() && (getDirection() == GraphObject::down || getDirection() == GraphObject::up))) &&
		isCurrentlyFacingPlayer())
	{
		//fire bullet to attack the player
		int x = getX(), y = getY();
		if (offsetCoordinatesInDirection(x, y, getDirection()))
			getStudentWorld()->insertActor(new Bullet(getStudentWorld(), x, y, getDirection()));
		getStudentWorld()->playSound(SOUND_ENEMY_FIRE);
		return true;
	}
	return false;
}

void Robot::isAttacked()
{
	//"Attack" the robot
	DestructableActor::isAttacked();

	//Play the appropriate sound if the robot was killed or just wounded
	if (isAlive())
		getStudentWorld()->playSound(SOUND_ROBOT_IMPACT);
	else
		getStudentWorld()->playSound(SOUND_ROBOT_DIE);
}

//===============================================================================================
// SnarlBot
//===============================================================================================

void SnarlBot::doSomething()
{
	//If it is alive and should move in this tick
	if (!isAlive() || !incCurrentTick())
		return;

	int x = getX(), y = getY();

	//if it cannot attack (because the player is out of sight)
	if(!attack() && offsetCoordinatesInDirection(x, y, getDirection()))
	{	
		//If there is no obstruction in front of it, move
		if (!fieldContainsObstruction(x, y, false))
			moveTo(x, y);
		//otherwise reverse direction
		else if (getDirection() == GraphObject::right)
			setDirection(GraphObject::left);
		else if (getDirection() == GraphObject::left)
			setDirection(GraphObject::right);
		else if (getDirection() == GraphObject::up)
			setDirection(GraphObject::down);
		else if (getDirection() == GraphObject::down)
			setDirection(GraphObject::up);
	}
}

void SnarlBot::isAttacked()
{
	//"Attack" the SnarlBot
	Robot::isAttacked();

	//increase player score by 100
	if (!isAlive())
		getStudentWorld()->increaseScore(100);
}

//===============================================================================================
// KleptoBot
//===============================================================================================

void KleptoBot::doSomething()
{
	//if robot is alive and should do something this tick
	if (!isAlive() || !incCurrentTick())
		return;

	//attack, if the player is in sight
	//This will only ever return true for AngryKleptoBots
	if (attack())
		return;

	int x = getX(), y = getY();

	//Otherwise, if it has not picked up a goodie yet
	if (m_goodie.empty())
	{
		//Check if there is a goodie other than a jewel
		list<Actor*> actorsFound = getStudentWorld()->getActorsAt(x, y);

		if (!actorsFound.empty())
		{
			for (list<Actor*>::iterator i = actorsFound.begin(); i != actorsFound.end(); i++)
			{
				Goodie* g = dynamic_cast<Goodie*>(*i);

				//if there is, pick it up with a chance of 1 out of 10
				if (g != nullptr && dynamic_cast<Jewel*>(g) == nullptr)
				{
					int randInt = rand() % 10;
					if (randInt == 0)
					{
						//Store which goodie was picked up, to create it again later
						if (dynamic_cast<ExtraLifeGoodie*>(g) != nullptr)
							m_goodie = "ExtraLifeGoodie";
						else if (dynamic_cast<AmmoGoodie*>(g) != nullptr)
							m_goodie = "AmmoGoodie";
						else if (dynamic_cast<RestoreHealthGoodie*>(g) != nullptr)
							m_goodie = "RestoreHealthGoodie";
						//destroy goodie and play appropriate sound
						g->isAttacked();
						getStudentWorld()->playSound(SOUND_ROBOT_MUNCH);
						return;
					}
				}
			}
		}
	}

	//If the robot has not moved movingDistance yet and there is no obstruction in front
	//of the robot, move there
	if (m_noOfMoves < m_movingDistance)
	{
		if (offsetCoordinatesInDirection(x, y, getDirection()) &&
			!fieldContainsObstruction(x, y, false))
		{
			moveTo(x, y);
			m_noOfMoves++;
			return;
		}
	}

	//Otherwise generate a new movingDistance and a random direction
	m_movingDistance = 1 + (rand() % 6);
	int dirRandInt = rand() % 4;
	GraphObject::Direction dir = getDirectionFromInt(dirRandInt);
	int randInts[4];
	randInts[0] = dirRandInt;
	int nRandInts = 1;

	//Try all directions
	for (int i = 0; i < 4; i++)
	{
		//if robot can move in this direction, move and return
		x = getX(), y = getY();
		if (offsetCoordinatesInDirection(x, y, dir) &&
			!fieldContainsObstruction(x, y, false))
		{
			setDirection(dir);
			moveTo(x, y);
			m_noOfMoves++;
			return;
		}

		//If all 4 directions have already been tried, do not generate a 5th one there is none
		if (i == 3)
			break;

		//Otherwise generate a new random direction that has not yet been tried
		while (true)
		{
			//Create random integer between 0 and 3
			dirRandInt = rand() % 4;
			bool existsAlready = false;
			//check if it has already been used
			for (int j = 0; j < nRandInts; j++)
				if (dirRandInt == randInts[j])
					existsAlready = true;
			//If not, put it into the array and break
			if (!existsAlready)
			{
				randInts[nRandInts] = dirRandInt;
				nRandInts++;
				break;
			}
		}
		//set it to the new random direction
		dir = getDirectionFromInt(randInts[i]);
	}
	//in case no direction works, set the robot's to the first random one
	setDirection(getDirectionFromInt(randInts[0]));
}

void KleptoBot::isAttacked()
{
	//"Attack KleptoBot"
	Robot::isAttacked();
	//If it was killed and had picked up a goodie
	if (!isAlive())
	{
		if (!m_goodie.empty())
		{
			//Spawn that goodie at the place of death
			Goodie* g = nullptr;
			if (m_goodie == "ExtraLifeGoodie")
				g = new ExtraLifeGoodie(getStudentWorld(), getX(), getY());
			else if (m_goodie == "AmmoGoodie")
				g = new AmmoGoodie(getStudentWorld(), getX(), getY());
			else if (m_goodie == "RestoreHealthGoodie")
				g = new RestoreHealthGoodie(getStudentWorld(), getX(), getY());
			if (g != nullptr)
				getStudentWorld()->insertActor(g);
		}
		//increase user score by 10
		getStudentWorld()->increaseScore(10);
	}
}

GraphObject::Direction KleptoBot::getDirectionFromInt(int dirInt)
{
	//Create an arbitrary direction from integers 0 through 3
	switch (dirInt)
	{
	case 0:
		return GraphObject::up;
		break;
	case 1:
		return GraphObject::right;
		break;
	case 2:
		return GraphObject::down;
		break;
	case 3:
		return GraphObject::left;
		break;
	}

	return GraphObject::none;
}

//===============================================================================================
// AngryKleptoBot
//===============================================================================================

void AngryKleptoBot::isAttacked()
{
	//"Attack" it like a KleptoBot
	KleptoBot::isAttacked();
	//If it died, increase score by another 10
	if (!isAlive())
		getStudentWorld()->increaseScore(10);
}

//===============================================================================================
// KleptoBotFactory
//===============================================================================================

void KleptoBotFactory::doSomething()
{
	int botCount = 0;

	//Count bots in a radius of 3 around this factory
	for (int x = getX() - 3; x <= getX() + 3; x++)
		if (x >= 0 && x < VIEW_WIDTH)
			for (int y = getY() - 3; y <= getY() + 3; y++)
				if (y >= 0 && y < VIEW_HEIGHT)
				{
					list<Actor*> actorsFound = getStudentWorld()->getActorsAt(x, y);
					if (!actorsFound.empty())
						for (list<Actor*>::iterator i = actorsFound.begin(); i != actorsFound.end(); i++)
							if (dynamic_cast<KleptoBot*>(*i) != nullptr)
								botCount++;
				}

	bool botOnTheSameField = false;

	//Check if there is no bot on the same field as the factory yet
	list<Actor*> actorsFound = getStudentWorld()->getActorsAt(getX(), getY());
	if (!actorsFound.empty())
		for (list<Actor*>::iterator i = actorsFound.begin(); i != actorsFound.end(); i++)
			if (dynamic_cast<KleptoBot*>(*i) != nullptr)
				botOnTheSameField = true;

	//If less than 3 bots were counted and there is no one on the same field as the factory
	if (botCount < 3 && !botOnTheSameField)
	{
		//Create a random number from 0 to 49
		int randInt = rand() % 50;
		//If it happens to be 0 (approx. 2% chance)
		if (randInt == 0)
		{
			//Produce the new type of kleptoBot that should be produced by this factory
			Actor* newKleptoBot;
			if (m_producesAngryKleptoBots)
			{
				newKleptoBot = new AngryKleptoBot(getStudentWorld(), getX(), getY());
			}
			else
			{
				newKleptoBot = new KleptoBot(getStudentWorld(), getX(), getY());
			}
			getStudentWorld()->insertActor(newKleptoBot);
			getStudentWorld()->playSound(SOUND_ROBOT_BORN);
		}
	}
}