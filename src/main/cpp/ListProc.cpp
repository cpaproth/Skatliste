//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include "ListProc.h"
#include "utils.h"
#include "linalg.h"
#include <iostream>

using namespace std;
using namespace linalg;

ListProc::ListProc() : finished(false) {

}

ListProc::~ListProc() {
	if (worker.joinable())
		worker.join();
}

void ListProc::scan(vector<uint8_t>& l, int32_t width, int32_t height) {
	if (worker.joinable())
		return;

	w = width;
	h = height;
	swap(input, l);
	input.resize(w * h);

	worker = thread(&ListProc::process, this);
}

void ListProc::result(Lines& l) {
	if (!finished)
		return;

	worker.join();
	finished = false;

	swap(lines, l);
}

void ListProc::process() {
	auto finish = guard([&] {finished = true;});

	using ve = vec<float, 2>;
	using ma = mat<float, 2, 2>;

	int angs = 21;
	vector<int> hlines(h * angs, 0), vlines(w * angs, 0);
	vector<float> cosa(angs), sina(angs);

	for (int a = 0; a < angs; a++) {
		float ang = (a - (angs - 1) / 2.f) * 0.005f;
		cosa[a] = cosf(ang);
		sina[a] = sinf(ang);
	}

	ve o(w / 2, h / 2);

	timespec res1, res2;
	clock_gettime(CLOCK_MONOTONIC, &res1);

	for (int x = 1; x < w - 1; x++) {
		for (int y = 1; y < h - 1; y++) {
			ve tr(input[y * w + x + 1], input[(y + 1) * w + x]);
			ve bl(input[y * w + x - 1], input[(y - 1) * w + x]);

			if (length2(tr - bl) < thfeat)
				continue;

			for (int a = 0; a < angs; a++) {
				vec<int, 2> d(floor(mul(ma({cosa[a], sina[a]}, {sina[a], cosa[a]}), ve(x, y) - o)) + o);

				if (d.x >= 0 && d.x < w)
					vlines[a * w + d.x]++;
				if (d.y >= 0 && d.y < h)
					hlines[a * h + d.y]++;
			}
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &res2);
	//cout << (res2.tv_sec - res1.tv_sec) * 1000. + (res2.tv_nsec - res1.tv_nsec) * 1.e-6 << endl;

	lines.clear();

	for (int a = 0; a < angs; a++) {
		for (int d = 0; d < h; d++) {
			if (a == 20 && d == 10)
				lines.push_back({0.f, (d - o.y + o.x * sina[a]) / cosa[a] + o.y, float(w), (d - o.y - o.x * sina[a]) / cosa[a] + o.y});
			if (a == 19 && d == 11)
				lines.push_back({0.f, (d - o.y + o.x * sina[a]) / cosa[a] + o.y, float(w), (d - o.y - o.x * sina[a]) / cosa[a] + o.y});
			if (hlines[a * h + d] > w / 2) {
				lines.push_back({0.f, (d - o.y + o.x * sina[a]) / cosa[a] + o.y, float(w), (d - o.y - o.x * sina[a]) / cosa[a] + o.y});


			}
		}
	}

}
