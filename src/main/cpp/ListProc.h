//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include <vector>
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
	int x = 0;
	int y = 0;
	int d = 0;
	void resize(int w, int h) {
		width = std::max(0, w);
		height = std::max(0, h);
		fields.resize(width * height);
	}
	bool select(float s = 0.f, int x_ = -1, int y_ = -1, int d_ = -1) {
		x = linalg::clamp(x_ < 0? x: x_, 0, width - 1);
		y = linalg::clamp(y_ < 0? y: y_, 0, height - 1);
		cur = y * width + x;
		if (cur < 0 || cur >= fields.size())
			return false;
		d = linalg::clamp(d_ < 0? d: d_, 0, fields[cur].chars.size());
		if (s > 0.f)
			fields[cur].all.resize(size * ceil(s * size));
		return true;
	}
	int w() {return d > 0? size: fields[cur].all.size() / size;}
	int h() {return size;}
	uint8_t* data() {return d > 0? fields[cur].chars[d - 1].data(): fields[cur].all.data();}
	uint8_t& operator()(int x_, int y_) {return data()[y_ * w() + x_];}
private:
	struct Field {
		std::vector<uint8_t> all;
		std::vector<std::vector<uint8_t>> chars;
		std::string text;
		int value;
	};
	static const int size = 16;
	int cur = 0;
	int width = 0;
	int height = 0;
	std::vector<Field> fields;
};

class ListProc {
public:
	using Lines = std::vector<mat2>;

	int edge_th = 10;
	int line_th = 50;

	ListProc();
	~ListProc();

	void scan(std::vector<uint8_t>&, int32_t, int32_t);
	bool result(Lines&, Fields&);

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
	vec2 fit_line(const vec2&, const vec2&, const std::vector<int>&, int);
	void process();
};