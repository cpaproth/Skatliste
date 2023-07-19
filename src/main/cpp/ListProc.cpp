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

bool ListProc::result(Lines& l) {
	if (!finished)
		return false;

	worker.join();
	finished = false;

	Lines old;
	swap(old, l);
	swap(lines, l);

	if (old.size() < 20 || l.size() < 20 || old.size() != l.size())
		return false;

	for (int i = 0; i < l.size(); i++) {
		if (length(old[i].x - l[i].x) > 2.f || length(old[i].y - l[i].y) > 2.f)
			return false;
	}
	return true;
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

float ListProc::bilinear(float, float) {
	/*template<class T> double bilinear(const valarray<T>& mem, unsigned w, unsigned h, unsigned x, unsigned y, double wx, double wy) {
		const T& m0 = mem[y * w + x];
		const T& m1 = x + 1 < w? mem[y * w + x + 1]: m0;
		const T& m2 = y + 1 < h? mem[(y + 1) * w + x]: m0;
		const T& m3 = x + 1 < w && y + 1 < h? mem[(y + 1) * w + x + 1]: x + 1 < w? m1: m2;
		return (1. - wx) * (1. - wy) * m0 + wx * (1. - wy) * m1 + (1. - wx) * wy * m2 + wx * wy * m3;
	}*/
	return 0;
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

	for (int x = 0; x + 1 < vl.size(); x++) {
		for (int y = 0; y + 1 < hl.size(); y++) {
			vec3 u1(intersect_lines(vl[x], hl[y] + vec2(M_PI / 2.f, 0.f)) + o, 1.f);
			vec3 u2(intersect_lines(vl[x + 1], hl[y] + vec2(M_PI / 2.f, 0.f)) + o, 1.f);
			vec3 u3(intersect_lines(vl[x + 1], hl[y + 1] + vec2(M_PI / 2.f, 0.f)) + o, 1.f);
			vec3 u4(intersect_lines(vl[x], hl[y + 1] + vec2(M_PI / 2.f, 0.f)) + o, 1.f);

			float height = 16., width = ceil((length(u2 - u1) + length(u3 - u4)) / (length(u4 - u1) + length(u3 - u2)) * height);

			vec3 v1(0.f, 0.f, 1.f);
			vec3 v2(width - 1.f, 0.f, 1.f);
			vec3 v3(width - 1.f, height - 1.f, 1.f);
			vec3 v4(0.f, height - 1.f, 1.f);

			vec3 u = mul(inverse(mat3(u1, u2, u3)), u4);
			vec3 v = mul(inverse(mat3(v1, v2, v3)), v4);
			mat3 m = mul(mat3(u1 * u.x, u2 * u.y, u3 * u.z), inverse(mat3(v1 * v.x, v2 * v.y, v3 * v.z)));

			for (float xi = 0.f; xi < width; xi++) {
				for (float yi = 0.f; yi < height; yi++) {
					vec3 r = mul(m, vec3(xi, yi, 1.f));
					float val = bilinear(r.x, r.y);
					using namespace ostream_overloads;
					//if (x==0 && y==0&&xi==0&&yi==0)
					//	cout <<m<<endl;
				}
			}
		}
	}

}

