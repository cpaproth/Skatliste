//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include <vector>
#include <array>
#include <thread>
#include <atomic>
#include "linalg.h"

using ve2 = linalg::vec<int, 2>;
using vec2 = linalg::vec<float, 2>;
using vec3 = linalg::vec<float, 3>;
using mat2 = linalg::mat<float, 2, 2>;
using mat3 = linalg::mat<float, 3, 3>;

vec2 intersect_lines(const vec2&, const vec2&);

class ListProc {
public:
	using Lines = std::vector<mat2>;

	int edge_th = 10;
	int line_th = 50;

	ListProc();
	~ListProc();

	void scan(std::vector<uint8_t>&, int32_t, int32_t);
	bool result(Lines&);

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

	std::vector<vec2> filter(std::vector<int>&);
	float bilinear(float, float);
	void process();
};