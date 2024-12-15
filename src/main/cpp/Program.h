//Copyright (C) 2023, 2024 Carsten Paproth - Licensed under MIT License

#include <GLES3/gl3.h>
#include "NdkCam.h"
#include "ListProc.h"
#include "Classifier.h"
#include <deque>
#include <map>

class Players {
public:
	bool three;
	float bet;
	int bpprize;
	int prizes;
	float prize;
	int remove;

	void load(const std::string&);
	void save(const std::string&);
	int count() {return ps.size();}
	int rounds() {return dates.size();}
	int num() {return count_if(ps.begin(), ps.end(), [](const Player& p) {return p.plays;});}
	bool noround() {return !num() && find_if(ps.begin(), ps.end(), [](const Player& p) {return p.sum() != 0;}) == ps.end();}
	bool find(const std::string& n) {return find_if(ps.begin(), ps.end(), [&](const Player& p) {return p.name == n;}) != ps.end();}
	void add_name(const std::string& n) {sel = n; ps.push_back({n, true, 0, 0, {}});}
	void del(const std::string& n) {ps.erase(find_if(ps.begin(), ps.end(), [&](const Player& p) {return p.name == n;}));}
	bool filled() {return num() && find_if(ps.begin(), ps.end(), [](const Player& p) {return p.plays && p.score == 0;}) == ps.end();}
	void add_score() {for_each(ps.begin(), ps.end(), [](Player& p) {p.result = p.sum(); p.score = 0;});}
	void add_sum(const std::string& d) {dates.push_front(d); for_each(ps.begin(), ps.end(), [](Player& p) {p.scores.push_front(p.sum()); p.result = p.score = 0; p.plays = false;});}
	void clear() {prize = 0.f; remove = order = 0; dates.clear(); sel.clear(); for_each(ps.begin(), ps.end(), [](Player& p) {p = {p.name, false, 0, 0, {}};});}
	const std::string& name(int i) {return ps[i].name;}
	const std::string& date(int i) {return dates[i];}
	bool& plays(int i) {return ps[i].plays;}
	int& score(int i) {return ps[i].score;}
	int sum(int i) {return ps[i].sum();}
	int total(int i) {return ps[i].best(remove + noround(), rounds());}
	int removed(int i) {int t = 0; for(int& s: ps[i].scores) t += s; return t + sum(i) - total(i);}
	int score(int i, int d) {return d < ps[i].scores.size()? ps[i].scores[d]: 0;}
	void sort_name() {order = 0; sort(ps.begin(), ps.end(), [&](const Player& a, const Player& b) {return a.plays && !b.plays || a.plays == b.plays && a.name < b.name;});}
	void sort_score() {order = 1; sel.clear(); stable_sort(ps.begin(), ps.end(), [&](const Player& a, const Player& b) {return a.plays && !b.plays || a.plays == b.plays && a.score > b.score;});}
	void sort_sum() {order = 2; sel.clear(); stable_sort(ps.begin(), ps.end(), [&](const Player& a, const Player& b) {return a.plays && !b.plays || a.plays == b.plays && a.sum() > b.sum();});}
	void sort_total() {order = 3; sel.clear(); int r = remove + noround(); stable_sort(ps.begin(), ps.end(), [&](const Player& a, const Player& b) {return a.best(r, rounds()) > b.best(r, rounds());});}
	int sorted() {return order;}
	void select(int i) {sel = name(i);}
	bool selected(int i) {return sel == name(i);}
	std::pair<int, int> tables() {int n = num(), t3 = three? (n - n % 3 * 4) / 3: (4 - n % 4) % 4, t4 = (n - 3 * t3) / 4; return {t4, t3};}
	int table(int i) {int t4 = tables().first; return i < 4 * t4? i / 4 + 1: (i - 4 * t4) / 3 + 1 + t4;}
	int seat(int i) {int t4 = tables().first; return i < 4 * t4? i % 4 + 1: (i - 4 * t4) % 3 + 1;}
	float prize_round(int);
	float prize_season(int);

private:
	struct Player {
		std::string name;
		bool plays;
		int score;
		int result;
		std::deque<int> scores;
		int sum() const {return result + score;}
		int best(int r, int n) const {auto s = scores; s.resize(n); s.push_back(sum()); sort(s.begin(), s.end()); int b = 0; while (n >= r && n >= 0 && s[n] > 0) b += s[n--]; return b;}
	};
	std::deque<std::string> dates;
	std::vector<Player> ps;
	std::string sel;
	int order;
};

class Program {
public:
	Program(const std::string&);
	~Program();

	void draw();

private:
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
	static const std::map<std::string, std::string> def_cfg;
	std::map<std::string, std::string> cfg = def_cfg;
	Players players;
	std::vector<Game> games;

	std::thread worker;
	std::atomic<bool> converting;
	std::mutex mut;
	List toplist;
	std::array<std::vector<int>, 4> topscores;
	std::vector<float> quality;

	void show_config(float);
	void show_players();
	void show_results();
	bool check_lines();
	void read_field();
	int dist(const std::string&, const std::string&);
	int dist(const Game&, bool, const std::string&, const std::string&, int, const std::string&);
	void process();
};
