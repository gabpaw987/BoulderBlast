#include "StudentWorld.h"
#include "level.h"
#include "Actor.h"
#include <string>
#include <sstream>
#include <iomanip>

GameWorld* createStudentWorld(string assetDir)
{
	return new StudentWorld(assetDir);
}

StudentWorld::~StudentWorld()
{
	cleanUp();
}

int StudentWorld::init()
{
	//If the player has completed 99 level, he/ahe won
	if (getLevel() > 99)
		return GWSTATUS_PLAYER_WON;

	//Set data members back to inital values in case this is not the first time a level is loaded
	m_bonus = 1000;
	m_isLevelCompleted = false;

	//load the current level
	ostringstream stream;
	stream << std::setw(2) << std::setfill('0') << getLevel();
	string currentLevel = "level" + stream.str() + ".dat";

	Level level(assetDirectory());
	Level::LoadResult loadResult = level.loadLevel(currentLevel);
	
	//if there is no level left, player won, if there was an error, return that
	if (loadResult == Level::load_fail_file_not_found)
		return GWSTATUS_PLAYER_WON;
	else if (loadResult == Level::load_fail_bad_format)
		return GWSTATUS_LEVEL_ERROR;

	//For each field in the level
	for (int i = 0; i < VIEW_WIDTH; i++)
	{
		for (int j = 0; j < VIEW_HEIGHT; j++)
		{
			//Create the appropriate actor at this field, if any
			switch (level.getContentsOf(i, j))
			{
			case Level::wall:
				insertActor(new Wall(this, i, j));
				break;
			case Level::player:
				m_player = new Player(this, i, j);
				break;
			case Level::boulder:
				insertActor(new Boulder(this, i, j));
				break;
			case Level::hole:
				insertActor(new Hole(this, i, j));
				break;
			case Level::jewel:
				insertActor(new Jewel(this, i, j));
				break;
			case Level::exit:
				insertActor(new Exit(this, i, j));
				break;
			case Level::extra_life:
				insertActor(new ExtraLifeGoodie(this, i, j));
				break;
			case Level::restore_health:
				insertActor(new RestoreHealthGoodie(this, i, j));
				break;
			case Level::ammo:
				insertActor(new AmmoGoodie(this, i, j));
				break; 
			case Level::horiz_snarlbot:
				insertActor(new SnarlBot(this, i, j, GraphObject::right));
				break;
			case Level::vert_snarlbot:
				insertActor(new SnarlBot(this, i, j, GraphObject::down));
				break;
			case Level::kleptobot_factory:
				insertActor(new KleptoBotFactory(this, i, j, false));
				break;
			case Level::angry_kleptobot_factory:
				insertActor(new KleptoBotFactory(this, i, j, true));
				break;
			} 
		}
	}

	return GWSTATUS_CONTINUE_GAME;
}

int StudentWorld::move()
{
	//Ask all actors to do something
	for (list<Actor*>::iterator i = m_actors.begin(); i != m_actors.end(); i++)
	{
		(*i)->doSomething();
		//If this one made the player die, handle that
		if (!m_player->isAlive())
		{
			decLives();
			return GWSTATUS_PLAYER_DIED;
		}
		//If this one made the user complete the level, handle that
		if (m_isLevelCompleted)
		{
			increaseScore(2000 + m_bonus);
			return GWSTATUS_FINISHED_LEVEL;
		}
	}

	//make the player do something
	m_player->doSomething();

	//Delete all the actors that were killed during this tick
	for (list<Actor*>::iterator j = m_actors.begin(); j != m_actors.end();)
	{
		DestructableActor* da = dynamic_cast<DestructableActor*>(*j);
		if (da != nullptr && !da->isAlive())
		{
			m_actors.erase(j);
			delete da;
			j = m_actors.begin();
		}
		else
		{
			j++;
		}
	}

	//If bonus is above 0, decrement it by one to reflect that the user took long to finish the level
	if (m_bonus > 0)
		m_bonus--;

	bool allJewelsCollected = true;

	//Check if any jewel was not collected yet
	for (list<Actor*>::iterator j = m_actors.begin(); j != m_actors.end(); j++)
	{
		Jewel* jewel = dynamic_cast<Jewel*>(*j);
		if (jewel != nullptr)
		{
			allJewelsCollected = false;
			break;
		}
	}
	//if all jewels have been collected, reveal the exit
	if (allJewelsCollected)
		for (list<Actor*>::iterator j = m_actors.begin(); j != m_actors.end(); j++)
		{
			Exit* exit = dynamic_cast<Exit*>(*j);
			if (exit != nullptr && !exit->isVisible())
			{
				exit->setVisible(true);
				playSound(SOUND_REVEAL_EXIT);
			}
		}

	//Update the top display text
	setDisplayText();

	//If this tick made the player die, handle that
	if (!m_player->isAlive())
	{
		decLives();
		return GWSTATUS_PLAYER_DIED;
	}
	//If this tick made the user complete the level, handle that
	if (m_isLevelCompleted)
	{
		increaseScore(2000 + m_bonus);
		return GWSTATUS_FINISHED_LEVEL;
	}

	//Otherwise continue the game
	return GWSTATUS_CONTINUE_GAME;
}

void StudentWorld::cleanUp()
{
	//Delete the player and the dangling pointer
	if (m_player != nullptr)
		delete m_player;

	m_player = nullptr;
	
	//Delete all the other dynamically allocated actors and their dangling pointers
	for (list<Actor*>::iterator i = m_actors.begin(); i != m_actors.end(); i++)
	{
		delete *i;
	}

	m_actors.clear();
}

list<Actor*> StudentWorld::getActorsAt(int x, int y)
{
	//if the player is at that field, add it to the list
	list<Actor*> actorsFound;
	if (m_player->getX() == x && m_player->getY() == y)
		actorsFound.push_back(m_player);

	//for all the other actors in the game
	for (list<Actor*>::iterator i = m_actors.begin(); i != m_actors.end(); i++)
	{
		//if this actors is on the specified field, add it to the list
		if ((*i)->getX() == x && (*i)->getY() == y)
		{
			actorsFound.push_back(*i);
		}
	}

	//return the list with all the actors found
	return actorsFound;
}

Player* StudentWorld::getPlayer() const
{
	return m_player;
}

void StudentWorld::insertActor(Actor* actor)
{
	m_actors.push_front(actor);
}

void StudentWorld::setLevelCompleted()
{
	m_isLevelCompleted = true;
}

void StudentWorld::setDisplayText()
{
	//Format all the numbers for the display properly with an ostringstream
	ostringstream scoreStream;
	scoreStream << setw(7) << setfill('0') << getScore();

	ostringstream lvlStream;
	lvlStream << setw(2) << setfill('0') << getLevel();

	ostringstream livesStream;
	livesStream << setw(2) << setfill(' ') << getLives();

	ostringstream hpStream;
	hpStream << setw(3) << setfill(' ') << (m_player->getHp() * (100 / 20));

	ostringstream ammoStream;
	ammoStream << setw(3) << setfill(' ') << m_player->getAmmunition();

	ostringstream bonusStream;
	bonusStream << setw(4) << setfill(' ') << m_bonus;

	//Put all the formatted numbers together with the appropriate explanatory text into the string
	string statusText = "Score: " + scoreStream.str() + "  Level: " + lvlStream.str() +
		"  Lives: " + livesStream.str() + "  Health: " + hpStream.str() + "%  Ammo: " + 
		ammoStream.str() + "  Bonus: " + bonusStream.str();

	//Update the top text above the game world
	setGameStatText(statusText);
}