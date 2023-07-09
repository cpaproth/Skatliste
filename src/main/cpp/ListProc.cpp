//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include "ListProc.h"
#include "utils.h"
#include "linalg.h"
#include <iostream>

using namespace std;
using namespace linalg;
using vi = vec<int, 2>;
using vf = vec<float, 2>;
using ma = mat<float, 2, 2>;

ListProc::ListProc() : finished(false), cosa(angs), sina(angs) {
	for (int a = 0; a < angs; a++) {
		float ang = (a - (angs - 1) / 2.f) * 0.01f;
		cosa[a] = cosf(ang);
		sina[a] = sinf(ang);
	}
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

vector<array<int, 2>> ListProc::filter(vector<int>& l) {
	int s = (int)l.size() / angs;
	int t = 2 * w * h / 3 / s;
	vector<int> as(s, 0);
	vector<array<int,2>> ls;

	for (int d = 0; d < s; d++) {
		int b = -1;
		for (int a = 0; a < angs; a++) {
			if (l[a * s + d] > b) {
				b = l[a * s + d];
				as[d] = a;
			}
		}
	}
	for (int d = range; d < s - range; d++) {
		int a = as[d];
		for (int i = -range; i <= range; i++) {
			if (i != 0 && l[as[d + i] * s + d + i] >= l[a * s + d])
				l[a * s + d] = 0;
		}
	}

	for (int d = range; d < s - range; d++) {
		int a = as[d];
		if (l[a * s + d] > t)
			ls.push_back({d, a});
	}
	for (int i = 1; i + 1 < ls.size(); i++) {
		if (abs(ls[i - 1][1] - ls[i][1]) > 2 && abs(ls[i + 1][1] - ls[i][1]) > 2)
			ls.erase(ls.begin() + i--);
	}

	return ls;
}

void ListProc::process() {
	auto finish = guard([&] {finished = true;});
	vector<int> vlines(w * angs, 0), hlines(h * angs, 0);
	vi o(w / 2, h / 2);

	timespec res1 = {}, res2 = {};
	clock_gettime(CLOCK_MONOTONIC, &res1);

	for (int x = 1; x < w - 1; x++) {
		for (int y = 1; y < h - 1; y++) {
			vi tr(input[y * w + x + 1], input[(y + 1) * w + x]);
			vi bl(input[y * w + x - 1], input[(y - 1) * w + x]);

			if (length2(tr - bl) < thfeat)
				continue;

			for (int a = 0; a < angs; a++) {
				vi d = vi(floor(mul(ma({cosa[a], sina[a]}, {sina[a], cosa[a]}), vf(vi(x, y) - o)))) + o;

				if (d.x >= 0 && d.x < w)
					vlines[a * w + d.x]++;
				if (d.y >= 0 && d.y < h)
					hlines[a * h + d.y]++;
			}
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &res2);
	//cout << (res2.tv_sec - res1.tv_sec) * 1000. + (res2.tv_nsec - res1.tv_nsec) * 1.e-6 << endl;

	auto vl = filter(vlines);
	auto hl = filter(hlines);

	lines.clear();
	for (int i = 0; i < vl.size(); i++) {
		int d = vl[i][0];
		int a = vl[i][1];
		lines.push_back({(d - o.x + o.y * sina[a]) / cosa[a] + o.x, 0.f, (d - o.x - o.y * sina[a]) / cosa[a] + o.x, float(h)});
	}
	for (int i = 0; i < hl.size(); i++) {
		int d = hl[i][0];
		int a = hl[i][1];
		lines.push_back({0.f, (d - o.y + o.x * sina[a]) / cosa[a] + o.y, float(w), (d - o.y - o.x * sina[a]) / cosa[a] + o.y});
	}
}

