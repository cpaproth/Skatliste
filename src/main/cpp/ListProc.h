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
	int X = 0;
	int Y = 0;
	int D = 0;
	void resize(int w, int h) {
		width = std::max(0, w);
		height = std::max(0, h);
		fields.clear();
		fields.resize(width * height);
	}
	bool select(float = 0.f, int = -1, int = -1, int = -1);
	void separate();
	int W() {return D > 0? size / 5 * 4: fields[cur].all.size() / size;}
	int H() {return size;}
	uint8_t* data() {return D > 0? fields[cur].chars[D - 1].data(): fields[cur].all.data();}
	uint8_t& operator()(int x, int y) {return data()[y * W() + x];}
private:
	struct Field {
		std::vector<uint8_t> all;
		std::vector<std::vector<uint8_t>> chars;
		std::string text;
		int value;
	};
	static const int size = 15;
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
	void process();
};