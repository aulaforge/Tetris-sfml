#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <time.h>
#include <iostream>
#include <vector>
#include <map>
#include <utility>


using namespace sf;
const int FIELD_SIZE_X = 10;
const int FIELD_SIZE_Y = 20;
const int TILESIZE = 15;
const int MAX_OBJ_SIZE = 4;
const float DELAY = 2;

typedef int object_id_type;
#define CURRENTFIGURE 0
#define OLDFIGURES 1


typedef int16_t figure_bitmask_type;
#define TOTALFIGURES 7
figure_bitmask_type figures[TOTALFIGURES] =
{
		0b100010001000100, // I
		0b100011000100000, // Z
		0b010011001000000, // S
		0b010011100000000, // T
		0b100010001100000, // L
		0b010001001100000, // J
		0b110011000000000, // O
};


class Coordinate{
public:
	int x;
	int y;
	Coordinate(int x, int y) : x(x), y(y) {};
	Coordinate() {x=0; y=0;};
};

class Point : public Coordinate{
public:
	Point(int x, int y, int tile_id) : Coordinate(x, y), tile_id(tile_id) {};
	Point(): Coordinate() {tile_id=0;};
public:
	int tile_id;
};

typedef std::vector<Point> t_vectorOfPoints;


class FieldObject {
public:
	t_vectorOfPoints points;
	FieldObject(Coordinate position, t_vectorOfPoints points) : position(position), points(points) {};
	FieldObject() : position(Point(0,0,0)) {};


	FieldObject(figure_bitmask_type binarypoints, int tile_id) {
		for (size_t p =0 ; p< sizeof(binarypoints)*8; ++p)
			if ( binarypoints & (1<<p) ) {
				Point newpoint((sizeof(binarypoints)*8-1-p) % MAX_OBJ_SIZE, (sizeof(binarypoints)*8-1-p) / MAX_OBJ_SIZE, tile_id);
				points.push_back(newpoint);
				//std::cout << "N: " << p << " " <<  p % MAX_OBJ_SIZE<< ":" << p / MAX_OBJ_SIZE<< std::endl;
			}
		position.x = (FIELD_SIZE_X-MAX_OBJ_SIZE)/2;
		position.y = 0;
	}

	const t_vectorOfPoints getAbsolutePoints() {
		t_vectorOfPoints abspoints;
		abspoints.reserve(points.size());
		for (size_t i = 0 ; i<points.size(); ++i ) {
			//std::cout << points[i].x << "+" << position.x << "=" << points[i].x + position.x << " & " << points[i].y << "+" << position.y << "=" << points[i].y + position.y  << " ;" ;
			abspoints.push_back(Point(points[i].x + position.x, points[i].y + position.y, points[i].tile_id ));
		}
		//std::cout << std::endl;
		return abspoints;
	}

public:
	 Coordinate position;
	 friend FieldObject operator+(const FieldObject& absleft, const FieldObject&  absright);

	 FieldObject& operator=(const FieldObject& o) {
		 points = o.points;
		 position = o.position;
		 return *this;
	 }


	 static FieldObject getRandomFigure() {
		 	int r = rand() % TOTALFIGURES;
			FieldObject randomfigure(figures[r], r);
			return randomfigure;

	 }
};

FieldObject operator+(const FieldObject& left, const FieldObject&  right) {
	 FieldObject o;
	 FieldObject l = left;
	 FieldObject r = right;
	 const t_vectorOfPoints absleftpoints  = l.getAbsolutePoints();
	 const t_vectorOfPoints absrightpoints = r.getAbsolutePoints();
     //std::cout << "Size left" << absleftpoints.size()  << ", Size right" << absrightpoints.size()  << std::endl;

	 o.points.insert(o.points.end(), absleftpoints.begin(), absleftpoints.end());
	 o.points.insert(o.points.end(), absrightpoints.begin(), absrightpoints.end());

	 o.position = left.position;
	 //std::cout << "Sum " << o.points.size()  << std::endl;
	 return o;

}

typedef std::map<object_id_type, FieldObject*> t_vectorOfFieldObjects;

class GameField {
public:
	//char _field[FIELD_X][FIELD_Y] ={0};
	t_vectorOfFieldObjects fieldobjects;
public:

	GameField() {
	}

	void addObject(object_id_type object_id, FieldObject *obj) {
		fieldobjects.insert(std::make_pair(object_id, obj));
	}

	auto objectByID(object_id_type object_id) {
		for (auto field_iter = fieldobjects.begin(); field_iter != fieldobjects.end(); ++field_iter) {
			if (field_iter->first == object_id)
				return field_iter->second;
		}
		std::cout << "Can't find object by name " << object_id << std::endl;
		//return fake object
		t_vectorOfPoints vp = t_vectorOfPoints();
		FieldObject *o = new FieldObject(Point(0, 0, 0), vp);
		return o;
	}

	void concat_objects(object_id_type first_id, object_id_type second_id) {
		FieldObject *first =  objectByID(first_id);
		FieldObject *second=  objectByID(second_id);
		*first = *first + *second;
		*second = FieldObject();

	}

	void replace_object(object_id_type object_id, FieldObject newobj) {
		FieldObject *obj =  objectByID(object_id);
		*obj = newobj;

	}

	 void move_left(object_id_type object_id)  {
		 if ( !collision_left() )
			 objectByID(object_id)->position.x -= 1;
	 }

	 void move_right(object_id_type object_id) {
		 if ( !collision_right())
			 objectByID(object_id)->position.x += 1;
	 }

	 void move_up_delete(object_id_type object_id)    {
			 objectByID(object_id)->position.y -= 1;
	 }

	 void move_down(object_id_type object_id)  {
		 if ( !collision_bottom())
			 objectByID(object_id)->position.y += 1;
	 }

	 void rotate_right(object_id_type object_id) {
		 FieldObject *obj = objectByID(object_id);

		 int min = MAX_OBJ_SIZE-obj->points[0].y ;
		 //std::cout << "Points to rotate: " << std::endl;
		 //return;

		 // rotate (change x and y)
		 for (size_t pi = 0 ; pi<obj->points.size(); ++pi ) {
			 //std::cout << "N: " << pi << " " << obj->points[pi].x << ":" << obj->points[pi].y << std::endl;
			 int mem;
		 	 mem = obj->points[pi].y;
		 	obj->points[pi].y = obj->points[pi].x;
		 	obj->points[pi].x = MAX_OBJ_SIZE - mem;
			 if (min > obj->points[pi].x)
				 min = obj->points[pi].x;
		 }
		 // substract minimum x
		 if (min>0)
			 for (size_t pi = 0 ; pi<obj->points.size(); ++pi )
				 obj->points[pi].x -= min;
	 }


	 bool collision_bottom() {
		 	const t_vectorOfPoints oldfig_abspoints = objectByID(OLDFIGURES)->getAbsolutePoints();
		 	const t_vectorOfPoints curfig_abspoints = objectByID(CURRENTFIGURE)->getAbsolutePoints();
		 	/*
			for (size_t i = 0 ; i<curfig_abspoints.size(); ++i )
				std::cout << "Y:" << curfig_abspoints[i].y << ", " ;
			std::cout << std::endl;
		 	 */

		 	for (size_t ci = 0 ; ci<curfig_abspoints.size(); ++ci ) {
		 		//std::cout << curfig_abspoints[ci].y << ", ";
				if ( curfig_abspoints[ci].y > FIELD_SIZE_Y-2) {
							//std::cout << " Point is bottom" << std::endl;
							return true;
				}
		 		//std::cout << std::endl;
		 		for (size_t oi = 0 ; oi<oldfig_abspoints.size(); ++oi ) {
					if ((oldfig_abspoints[oi].x == curfig_abspoints[ci].x)
							&& (( (oldfig_abspoints[oi].y - curfig_abspoints[ci].y) < 2) ) ) {
								//std::cout << "Found point in bottom collision" << std::endl;
								return true;
					}
				}
		 	}
			return false;

	 }

	 bool collision_left() {
		 	const t_vectorOfPoints oldfig_abspoints = objectByID(OLDFIGURES)->getAbsolutePoints();
		 	const t_vectorOfPoints curfig_abspoints = objectByID(CURRENTFIGURE)->getAbsolutePoints();

		 	for (size_t ci = 0 ; ci<curfig_abspoints.size(); ++ci ) {
				if ( curfig_abspoints[ci].x == 0) {
							//std::cout << " Point is at left minimum" << std::endl;
							return true;
				}

				for (size_t oi = 0 ; oi<oldfig_abspoints.size(); ++oi ) {
					if ((oldfig_abspoints[oi].y == curfig_abspoints[ci].y)
							&& (( (curfig_abspoints[ci].x - oldfig_abspoints[oi].x) == 1) ) ) {
								//std::cout << "Found point in left collision" << std::endl;
								return true;
					}
				}
		 	}
	 }

	 bool collision_right() {
		 	const t_vectorOfPoints oldfig_abspoints = objectByID(OLDFIGURES)->getAbsolutePoints();
		 	const t_vectorOfPoints curfig_abspoints = objectByID(CURRENTFIGURE)->getAbsolutePoints();

		 	for (size_t ci = 0 ; ci<curfig_abspoints.size(); ++ci ) {
				if ( curfig_abspoints[ci].x == FIELD_SIZE_X - 1) {
							//std::cout << " Point is at right maximum" << std::endl;
							return true;
				}

				for (size_t oi = 0 ; oi<oldfig_abspoints.size(); ++oi ) {
					if ((oldfig_abspoints[oi].y == curfig_abspoints[ci].y)
							&& (( ( oldfig_abspoints[oi].x - curfig_abspoints[ci].x) == 1) ) ) {
								//std::cout << "Found point in right collision" << std::endl;
								return true;
					}
				}
		 	}
	 }

	 std::vector<size_t> check_completelines() {
		 bool array[FIELD_SIZE_X][FIELD_SIZE_Y] = {false};
		 	const t_vectorOfPoints oldfig_abspoints = objectByID(OLDFIGURES)->getAbsolutePoints();
		 	std::vector<size_t> result;

		 	// copy points to array
		 	for (size_t oi = 0 ; oi<oldfig_abspoints.size(); ++oi )
		 		array[oldfig_abspoints[oi].x][oldfig_abspoints[oi].y] = true;

		 	for (int j = 0; j<FIELD_SIZE_Y; j++) {
		 		bool linecomplete = false;
		 		int linecounter = 0;
		 		for (int i = 0; i<FIELD_SIZE_X; i++)
		 			if (array[i][j])
		 				++linecounter;
		 		if (linecounter == FIELD_SIZE_X)
		 			result.push_back(j);
		 		//if (linecounter>0)
		 			//std::cout << "Total complete blocks found " << linecounter << "in position " << j << std::endl;
		 	}
	 		//std::cout << std::endl;

		 	return result;
	 }

	 void removeline(size_t l) {
			auto oldfigures = objectByID(OLDFIGURES);
			t_vectorOfPoints cleanpoints;
			for (size_t pi = 0 ; pi< oldfigures->points.size(); ++pi )
				if (oldfigures->points[pi].y != l) {
					if (oldfigures->points[pi].y<l)
						++oldfigures->points[pi].y;
					cleanpoints.push_back(oldfigures->points[pi]);
				}
			oldfigures->points = cleanpoints;
	 }

	 void removelines(std::vector<size_t> lines) {
		std::cout << "Removing " << lines.size() << " lines" << std::endl;
	 	for (size_t lineindex = 0; lineindex<lines.size(); lineindex++) {
	 		std::cout << "->" << lines[lineindex] << std::endl;
	 		removeline(lines[lineindex]);
	 	}
	 }

};


class GameResources {
public:
	Texture t;
	SoundBuffer musicbuffer;
	Music music;
	Font font;

	static GameResources * Instance() {
		if (!_instance) {
			_instance = new GameResources();
		}
		return _instance;
	}

	void init() {
			std::cout << "Loading resources ... " << std::endl;
			if (!t.loadFromFile("resources/tiles.png"))
				std::cout << "Can't load tiles" << std::endl;
			if (!music.openFromFile("resources/Tetris_theme.ogg"))
				std::cout << "Can't load music" << std::endl;
			if (!font.loadFromFile("resources/arial.ttf"))
				std::cout << "Can't load font" << std::endl;
   }

private:
	static GameResources * _instance;


	GameResources() { }  // конструктор недоступен
	GameResources(const GameResources& ) { }
   ~GameResources() { } // и деструктор
   //GameResources(GameResources const&); // реализация не нужна
   GameResources& operator= (GameResources const&);  // и тут



};
GameResources * GameResources::_instance = 0;



//static const Point figures[] = {Point(1, 0, 0) , Point(0, 1, 0) , Point(1, 1, 0) , Point(2, 1, 0) , };

void drawscreen(RenderWindow& window, GameField field) {
	GameResources  *res = GameResources::Instance();
	Text text("Hello Tomas", res->font, 50);
	window.clear(Color::Yellow);
	window.draw(text);

	for (auto field_iter = field.fieldobjects.begin(); field_iter != field.fieldobjects.end(); ++field_iter)
		for (size_t pi = 0 ; pi<field_iter->second->points.size(); ++pi ) {
			Sprite ss(res->t);
			//std::cout << "Drw " << field_iter->second->position.tile_id << std::endl;
			//field_iter->second->position.tile_id=1;
			ss.setTextureRect(IntRect(field_iter->second->points[pi].tile_id*TILESIZE, 0 , TILESIZE, TILESIZE));
			ss.setPosition(
					field_iter->second->points[pi].x * TILESIZE + (field_iter->second->position.x * TILESIZE) ,
					field_iter->second->points[pi].y * TILESIZE + (field_iter->second->position.y * TILESIZE)
					);
			window.draw(ss);
		}

window.display();
}

int main() {
	GameResources  *res = GameResources::Instance();
	res->init();
	GameField field;

	srand(time(0));
	FieldObject newfigure = FieldObject::getRandomFigure();
	field.addObject(CURRENTFIGURE, &newfigure);

	FieldObject oldfigures= FieldObject();
	field.addObject(OLDFIGURES, &oldfigures);

	RenderWindow window(VideoMode(( FIELD_SIZE_X)*TILESIZE, (FIELD_SIZE_Y)*TILESIZE), "Tetris");
	Sprite s(res->t);
	s.setTextureRect(IntRect(0,0 , TILESIZE, TILESIZE));
	s.setPosition(100, 100);


	res->music.setLoop(true);
	res->music.play();

	Clock clock;
	float timer=0;
	bool paused = false;

	while (window.isOpen()) {
		float time = clock.getElapsedTime().asSeconds();
		clock.restart();
		timer += time;

		Event e;
		while (window.pollEvent(e)) {
			if (e.type == Event::Closed)
				window.close();
			if (e.type == Event::KeyPressed){
				if (e.key.code == Keyboard::Space) paused = !paused;
				if (paused) break;
				if (e.key.code == Keyboard::Left) field.move_left(CURRENTFIGURE);
				if (e.key.code == Keyboard::Right) field.move_right(CURRENTFIGURE);
				if (e.key.code == Keyboard::Down) field.move_down(CURRENTFIGURE);
				if (e.key.code == Keyboard::Up) field.rotate_right(CURRENTFIGURE);

			}
		}

		if (paused) continue;

		if (timer>DELAY) {

			if (field.collision_bottom()) {
				field.concat_objects(OLDFIGURES, CURRENTFIGURE);
				FieldObject randomfigure = FieldObject::getRandomFigure();
				field.replace_object(CURRENTFIGURE, randomfigure);
				//std::cout << "bottom " << std::endl;
			}
			else {
				field.move_down(CURRENTFIGURE);
			}
		timer = 0;
		}

		std::vector<size_t> completelines = field.check_completelines();
		if (completelines.size()>0) {
			field.removelines(completelines);
			//std::cout << "Total lines complete " << completelines.size() << " " << completelines[0] << std::endl;
		}
		drawscreen(window, field);
	}


	res->music.stop();

	return 0;
}


