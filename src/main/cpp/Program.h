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
	int count() {return ps.size();}
	int rounds() {return dates.size();}
	bool find(const std::string& n) {return find_if(ps.begin(), ps.end(), [&](const Player& p) {return p.name == n;}) != ps.end();}
	void add_name(const std::string& n) {ps.push_back({n, true, 0, 0, {}});}
	void del(const std::string& n) {ps.erase(find_if(ps.begin(), ps.end(), [&](const Player& p) {return p.name == n;}));}
	void add_score() {for_each(ps.begin(), ps.end(), [](Player& p) {p.sum = p.sum + p.score; p.score = 0;});}
	void add_round(const std::string& d) {dates.push_back(d); for_each(ps.begin(), ps.end(), [](Player& p) {p.scores.push_back(p.sum + p.score); p.sum = p.score = 0; p.plays = false;});}
	void clear() {for_each(ps.begin(), ps.end(), [](Player& p) {p = {p.name, false, 0, 0, {}};});}
	const std::string& name(int i) {return ps[i].name;}
	const std::string& date(int i) {return dates[i];}
	bool& plays(int i) {return ps[i].plays;}
	int& score(int i) {return ps[i].score;}
	int sum(int i) {return ps[i].sum + ps[i].score;}
	int total(int i) {int t = 0; for(int& s: ps[i].scores) t += s; return t + sum(i) - removed(i);}
	int removed(int);
	int score(int i, int d) {return ps[i].scores[d];}
	void sort_name();
	void sort_score();
	void sort_sum();
	void sort_total();
	int sorted() {return order;}
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
	std::vector<Player> ps;
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
