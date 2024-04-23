//Copyright (C) 2023, 2024 Carsten Paproth - Licensed under MIT License

#include <vector>
#include <set>
#include <thread>
#include <atomic>
#include "linalg.h"

using ve2 = linalg::vec<int, 2>;
using vec2 = linalg::vec<float, 2>;
using vec3 = linalg::vec<float, 3>;
using mat2 = linalg::mat<float, 2, 2>;
using mat3 = linalg::mat<float, 3, 3>;

vec2 intersect_lines(const vec2&, const vec2&);

class Fields {
public:
	static const int hd = 15;
	static const int wd = hd / 5 * 4;
	int X = 0;
	int Y = 0;
	int D = 0;
	void resize(int w, int h) {
		width = std::max(0, w);
		height = std::max(0, h);
		fields.clear();
		fields.resize(width * height);
	}
	bool check(int p) {
		if (p < width * (height + 1))
			return p == Y * width + X || p == (Y + 1) * width + X;
		p -= width * (height + 1);
		return p == Y * (width + 1) + X || p == Y * (width + 1) + X + 1;
	}
	bool select(int p) {
		if (p < 0 || width <= 0)
			return false;
		int w = p < width * (height + 1)? width: width + 1;
		p = p < width * (height + 1)? p: p - width * (height + 1);
		return select(0.f, p % w, p / w, 0);
	}
	bool select(float = 0.f, int = -1, int = -1, int = -1);
	bool select(int, int);
	bool ignore_row() {rows.insert(Y); return next();}
	bool ignore_col() {cols.insert(X); return next();}
	bool first();
	bool next();
	int separate(int);
	int W() {return D > 0? wd: fields[cur].all.size() / hd;}
	int H() {return hd;}
	uint8_t* data() {return D > 0? fields[cur].chars[D - 1].data(): fields[cur].all.data();}
	uint8_t& operator()(int x, int y) {return data()[y * W() + x];}
	std::string& str() {return fields[cur].text;}
private:
	struct Field {
		std::vector<uint8_t> all;
		std::vector<std::vector<uint8_t>> chars;
		std::string text;
	};
	int cur = 0;
	int width = 0;
	int height = 0;
	std::vector<Field> fields;
	std::set<int> rows, cols;
	void sep_mode0(int, int, uint8_t*, int);
	void sep_mode1(int, int, uint8_t*);
	void sep_mode2(int, int, uint8_t*);
};

class ListProc {
public:
	using Lines = std::vector<mat2>;

	int edge_th = 10;
	int line_th = 40;
	bool big_chars = false;
	bool faint_chars = false;

	ListProc();
	~ListProc();

	void scan(const std::vector<uint8_t>&, int32_t, int32_t);
	void rescan() {scan(input, w, h);}
	bool result(Lines&, Fields&);
	void get_input(std::function<void(void*, int, int)> f) {f(input.data(), w, h);}

private:
	int32_t w = 0;
	int32_t h = 0;
	static const int angs = 21;
	static const int range = 5;
	std::vector<float> cosa;
	std::vector<float> sina;

	std::vector<uint8_t> input;
	std::thread worker;
	std::atomic<bool> finished;
	Lines lines;
	Fields fields;

	std::vector<vec2> filter(std::vector<int>&);
	vec2 find_corner(const vec2&, const std::vector<int>&);
	void process();
};