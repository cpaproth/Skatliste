//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include "ListProc.h"
#include "utils.h"
#include "linalg.h"
#include <iostream>

using namespace std;
using namespace linalg;

vec2 intersect_lines(const vec2& l1, const vec2& l2) {
	return mul(inverse(mat2({cosf(l1.x), cosf(l2.x)}, {sinf(l1.x), sinf(l2.x)})), vec2(l1.y, l2.y));
}

ListProc::ListProc() : finished(false), cosa(angs), sina(angs) {
	for (int a = 0; a < angs; a++) {
		float ang = (float(a) - (angs - 1) / 2.f) * 0.01f;
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

vector<vec2> ListProc::filter(vector<int>& l) {
	int s = (int)l.size() / angs;
	int t = w * h / s;
	vector<int> as(s, 0), avgl(l.size(), 0), avgd(l.size(), 0), avga(l.size(), 0);
	vector<vec2> ls;

	for (int d = 1; d < s - 1; d++) {
		for (int a = 1; a < angs - 1; a++) {
			for (int od = -1; od <= 1; od++) {
				for (int oa = -1; oa <= 1; oa++) {
					avgl[a * s + d] += l[(a + oa) * s + d + od];
					avgd[a * s + d] += l[(a + oa) * s + d + od] * (d + od);
					avga[a * s + d] += l[(a + oa) * s + d + od] * (a + oa);
				}
			}
		}
	}
	swap(avgl, l);

	for (int d = 0; d < s; d++) {
		for (int a = 0; a < angs; a++) {
			if (l[a * s + d] > l[as[d] * s + d])
				as[d] = a;
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
		int p = as[d] * s + d;
		if (l[p] > t * line_th / 50) {
			float ang = (float(avga[p]) / float(l[p]) - (angs - 1) / 2.f) * 0.01f;
			float del = float(avgd[p]) / float(l[p]) + 0.5f - float(s) / 2.f;
			if (abs(intersect_lines(vec2(ang, del), vec2(0.f, float(s) / 2.f)).y) < float(t) / 2.f)
				continue;
			if (abs(intersect_lines(vec2(ang, del), vec2(0.f, float(-s) / 2.f)).y) < float(t) / 2.f)
				continue;
			if (!ls.empty() && abs(intersect_lines(vec2(ang, del), ls.back()).y) < float(t) / 2.f)
				continue;
			ls.emplace_back(ang, del);
		}
	}

	return ls;
}

void ListProc::process() {
	auto finish = guard([&] {finished = true;});
	vector<int> vlines(w * angs, 0), hlines(h * angs, 0);
	ve2 o(w / 2, h / 2);

	timespec res1 = {}, res2 = {};
	clock_gettime(CLOCK_MONOTONIC, &res1);

	for (int x = 1; x < w - 1; x++) {
		for (int y = 1; y < h - 1; y++) {
			int v = input[y * w + x];
			int ver = minelem(ve2(input[y * w + x + 1], input[y * w + x - 1]) - v);
			int hor = minelem(ve2(input[(y + 1) * w + x], input[(y - 1) * w + x]) - v);

			if (ver < edge_th && hor < edge_th)
				continue;

			for (int a = 0; a < angs; a++) {
				ve2 d = ve2(floor(mul(mat2({cosa[a], -sina[a]}, {sina[a], cosa[a]}), vec2(ve2(x, y) - o)))) + o;

				if (ver >= edge_th && d.x >= 0 && d.x < w)
					vlines[a * w + d.x]++;
				if (hor >= edge_th && d.y >= 0 && d.y < h)
					hlines[a * h + d.y]++;
			}
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &res2);
	//cout << (res2.tv_sec - res1.tv_sec) * 1000. + (res2.tv_nsec - res1.tv_nsec) * 1.e-6 << endl;

	auto vl = filter(vlines);
	auto hl = filter(hlines);

	lines.clear();
	if (vl.empty() || hl.empty())
		return;
	for (auto& i : vl) {
		vec2 p1 = intersect_lines(i, hl.front() + vec2(M_PI / 2.f, 0.f)) + o;
		vec2 p2 = intersect_lines(i, hl.back() + vec2(M_PI / 2.f, 0.f)) + o;
		lines.emplace_back(p1, p2);
	}
	for (auto& i : hl) {
		vec2 p1 = intersect_lines(i + vec2(M_PI / 2.f, 0.f), vl.front()) + o;
		vec2 p2 = intersect_lines(i + vec2(M_PI / 2.f, 0.f), vl.back()) + o;
		lines.emplace_back(p1, p2);
	}
}
