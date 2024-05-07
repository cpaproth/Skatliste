//Copyright (C) 2023, 2024 Carsten Paproth - Licensed under MIT License

#include <GLES3/gl3.h>
#include "NdkCam.h"
#include "ListProc.h"
#include "Classifier.h"

class Players {
public:
	bool three;
	float prize;
	int remove;

	void load(const std::string&);
	void save(const std::string&);
	int count();
	int rounds();
	int find(const std::string&);
	void add_name(const std::string&);
	void del(int);
	void clear();
	void add_score();
	void add_round(const std::string&);
	const std::string& name(int);
	bool& plays(int);
	int& score(int);
	int sum(int);
	int total(int);
	int removed(int);
	int score(int, int);
	void sort_name();
	void sort_score();
	void sort_sum();
	void sort_total();
	int sorted();
	int table(int);
	int seat(int);
	int prize_day(int);
	float prize_year(int);

private:
	struct Player {
		std::string name;
		bool plays;
		int score;
		int sum;
		std::vector<int> scores;
	};
	std::vector<std::string> dates;
	std::vector<Player> players;
	int order;
};

class Program {
public:
	Program(const std::string&);
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
	int result = 0;

	GLuint cap_tex = 0;
	GLuint dig_tex = 0;
	std::string path;
	NdkCam cam;
	ListProc proc;
	ListProc::Lines lines;
	Fields fields;
	Classifier clss;
	static const std::vector<const char*> chars;
	std::string playersfile = "players.csv";
	std::vector<Player> players;
	std::vector<Game> games;

	std::thread worker;
	std::atomic<bool> converting;
	std::mutex mut;
	List toplist;
	std::array<std::vector<int>, 4> topscores;

	void load_players();
	void save_players();
	void show_players();
	void show_results();
	bool check_lines();
	void read_field();
	int dist(const std::string&, const std::string&);
	int dist(const Game&, bool, int, const std::string&, const std::string&, int, const std::string&, const std::string&);
	void process();
};
