//Copyright (C) 2023, 2024 Carsten Paproth - Licensed under MIT License

#include <GLES3/gl3.h>

#include "NdkCam.h"
#include "ListProc.h"
#include "Classifier.h"

class Program {
public:
	Program();
	~Program();

	void draw();

private:
	struct Player {
		std::string name;
		bool plays;
		int score;
		std::vector<int> scores;
	};
	struct Game {
		std::string name;
		std::string tips;
		int extra;
		int points;
	};
	struct Line {
		int points;
		int player;
		std::array<int, 4> scores;
	};
	typedef std::vector<Line> List;

	bool learn = false;
	bool convert = false;
	bool list = false;
	int cols = 0;
	int rows = 0;
	int fcol = 0;
	int frow = 0;
	int nplayer = 3;

	GLuint cap_tex = 0;
	GLuint dig_tex = 0;
	NdkCam cam;
	ListProc proc;
	ListProc::Lines lines;
	Fields fields;
	Classifier clss;
	static const std::vector<const char*> chars;
	std::vector<Player> players;
	std::vector<Game> games;

	std::thread worker;
	std::atomic<bool> converting;
	std::mutex mut;
	List toplist;

	void show_list();
	void show_results();
	bool check_lines();
	void read_field();
	int dist(const std::string&, const std::string&);
	int dist(const Game&, bool, int, const std::string&, const std::string&, int, const std::string&, const std::string&);

	//void refine_list();

	void process();
};
