//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include "ListProc.h"
#include "utils.h"
#include "linalg.h"
#include <iostream>
#include <fstream>

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

	ifstream file("/storage/emulated/0/Download/img.480.ubyte");
	file.read((char*)input.data(), input.size());

	worker = thread(&ListProc::process, this);
}

bool ListProc::result(Lines& l, Fields& f) {
	if (!finished)
		return false;

	worker.join();
	finished = false;

	Lines old;
	swap(old, l);
	swap(lines, l);
	swap(fields, f);

	if (old.size() < 20 || l.size() < 20 || old.size() != l.size())
		return false;

	for (int i = 0; i < l.size(); i++) {
		if (length(old[i].x - l[i].x) > 5.f || length(old[i].y - l[i].y) > 5.f)
			return false;
	}

//	ofstream file("/storage/emulated/0/Download/img.480.ubyte");
//	file.write((char*)input.data(), input.size());
//	ofstream ofile("/storage/emulated/0/Download/img.txt");
//	ofile << "huch\n";
//	ofile.close();
//	ifstream ifile("/storage/emulated/0/Download/img.txt");
//	string str;
//	ifile >> str;
//	cout << str << endl;

	return true;
}

vector<vec2> ListProc::filter(vector<int>& l) {
	int s = (int)l.size() / angs;
	int t = w * h / s;
	vector<int> as(s, 0), avgl(l.size(), 0), avgd(l.size(), 0), avga(l.size(), 0);
	vector<vec2> ls;

	for (int d = 1; d < s - 1; d++) {
		for (int a = 1; a < angs - 1; a++) {
			int minl = INT_MAX;
			for (int od = -1; od <= 1; od++) {
				for (int oa = -1; oa <= 1; oa++) {
					minl = min(minl, l[(a + oa) * s + d + od]);
				}
			}
			for (int od = -1; od <= 1; od++) {
				for (int oa = -1; oa <= 1; oa++) {
					avgl[a * s + d] += l[(a + oa) * s + d + od] - minl;
					avgd[a * s + d] += (l[(a + oa) * s + d + od] - minl) * (d + od);
					avga[a * s + d] += (l[(a + oa) * s + d + od] - minl) * (a + oa);
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
			float del = float(avgd[p]) / float(l[p]) - float(s) / 2.f;
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

vec2 ListProc::find_corner(const vec2& p, const vector<int>& c) {
	int m = INT_MIN, xm = 0, ym = 0;
	for (int x = max(2, (int)p.x - 3); x < min(w - 2, (int)p.x + 4); x++) {
		for (int y = max(2, (int)p.y - 3); y < min(h - 2, (int)p.y + 4); y++) {
			if (c[y * w + x] > m) {
				m = c[y * w + x];
				xm = x;
				ym = y;
			}
		}
	}
	int minc = INT_MAX, sumc = 0, xc = 0, yc = 0;
	for (int x = xm - 1; x <= xm + 1; x++) {
		for (int y = ym - 1; y <= ym + 1; y++) {
			minc = min(minc, c[y * w + x]);
		}
	}
	for (int x = xm - 1; x <= xm + 1; x++) {
		for (int y = ym - 1; y <= ym + 1; y++) {
			sumc += c[y * w + x] - minc;
			xc += (c[y * w + x] - minc) * x;
			yc += (c[y * w + x] - minc) * y;
		}
	}
	vec2 n = vec2((float)xc, (float)yc) / (float)sumc;
	return length(p - n) < 2.0f? n: p;
}

void ListProc::process() {
	auto finish = guard([&] {finished = true;});
	vector<int> vlines(w * angs, 0), hlines(h * angs, 0), corners(input.size(), 0);
	ve2 o(w / 2, h / 2);

	//timespec res1 = {}, res2 = {};
	//clock_gettime(CLOCK_MONOTONIC, &res1);

	for (int x = 1; x < w - 1; x++) {
		for (int y = 1; y < h - 1; y++) {
			int v = input[y * w + x];
			int ver = minelem(ve2(input[y * w + x + 1], input[y * w + x - 1]) - v);
			int hor = minelem(ve2(input[(y + 1) * w + x], input[(y - 1) * w + x]) - v);

			corners[y * w + x] = input[(y - 1) * w + x - 1] - input[(y - 1) * w + x] + input[(y - 1) * w + x + 1];
			corners[y * w + x] -= input[y * w + x - 1] + input[y * w + x] + input[y * w + x + 1];
			corners[y * w + x] += input[(y + 1) * w + x - 1] - input[(y + 1) * w + x] + input[(y + 1) * w + x + 1];

			if (ver < edge_th && hor < edge_th)
				continue;

			for (int a = 0; a < angs; a++) {
				ve2 d = ve2(floor(mul(mat2({cosa[a], -sina[a]}, {sina[a], cosa[a]}), vec2(ve2(x, y) - o) + 0.5f))) + o;

				if (ver >= edge_th && d.x >= 0 && d.x < w)
					vlines[a * w + d.x]++;
				if (hor >= edge_th && d.y >= 0 && d.y < h)
					hlines[a * h + d.y]++;
			}
		}
	}

	//clock_gettime(CLOCK_MONOTONIC, &res2);
	//cout << (res2.tv_sec - res1.tv_sec) * 1000. + (res2.tv_nsec - res1.tv_nsec) * 1.e-6 << endl;

	auto vl = filter(vlines);
	auto hl = filter(hlines);

	size_t wp = vl.size(), hp = hl.size();
	lines.clear();
	fields.resize((int)wp - 1, (int)hp - 1);
	if (wp * hp == 0)
		return;
	vector<vec2> points(wp * hp), offsets(wp * hp);
	for (int y = 0; y < hp; y++) {
		for (int x = 0; x < wp; x++) {
			points[y * wp + x] = intersect_lines(vl[x], hl[y] + vec2(M_PI / 2.f, 0.f)) + o;
			offsets[y * wp + x] = find_corner(points[y * wp + x], corners) - points[y * wp + x];
		}
	}
	for (int x = wp / 2 + 1; x < wp; x++) {
		offsets[hp / 2 * wp + x] = find_corner(points[hp / 2 * wp + x] + offsets[hp / 2 * wp + x - 1], corners) - points[hp / 2 * wp + x];
		for (int y = hp / 2 + 1; y < hp; y++)
			offsets[y * wp + x] = find_corner(points[y * wp + x] + offsets[(y - 1) * wp + x], corners) - points[y * wp + x];
		for (int y = hp / 2 - 1; y >= 0; y--)
			offsets[y * wp + x] = find_corner(points[y * wp + x] + offsets[(y + 1) * wp + x], corners) - points[y * wp + x];
	}
	for (int x = wp / 2 - 1; x >= 0; x--) {
		offsets[hp / 2 * wp + x] = find_corner(points[hp / 2 * wp + x] + offsets[hp / 2 * wp + x + 1], corners) - points[hp / 2 * wp + x];
		for (int y = hp / 2 + 1; y < hp; y++)
			offsets[y * wp + x] = find_corner(points[y * wp + x] + offsets[(y - 1) * wp + x], corners) - points[y * wp + x];
		for (int y = hp / 2 - 1; y >= 0; y--)
			offsets[y * wp + x] = find_corner(points[y * wp + x] + offsets[(y + 1) * wp + x], corners) - points[y * wp + x];
	}

	for (int y = 0; y < hp; y++)
		for (int x = 0; x + 1 < wp; x++)
			lines.emplace_back(points[y * wp + x] + offsets[y * wp + x], points[y * wp + x + 1] + offsets[y * wp + x + 1]);
	for (int y = 0; y + 1 < hp; y++)
		for (int x = 0; x < wp; x++)
			lines.emplace_back(points[y * wp + x] + offsets[y * wp + x], points[(y + 1) * wp + x] + offsets[(y + 1) * wp + x]);

	for (int y = 0; y + 1 < hp; y++) {
		for (int x = 0; x + 1 < wp; x++) {
			vec3 u1(points[y * wp + x] + offsets[y * wp + x], 1.f);
			vec3 u2(points[y * wp + x + 1] + offsets[y * wp + x + 1], 1.f);
			vec3 u3(points[(y + 1) * wp + x + 1] + offsets[(y + 1) * wp + x + 1], 1.f);
			vec3 u4(points[(y + 1) * wp + x] + offsets[(y + 1) * wp + x], 1.f);

			if (length(u4 - u1) + length(u3 - u2) == 0.f)
				continue;
			fields.select((length(u2 - u1) + length(u3 - u4)) / (length(u4 - u1) + length(u3 - u2)), x, y);

			vec3 v1(0.f, 0.f, 1.f);
			vec3 v2(fields.w() - 1.f, 0.f, 1.f);
			vec3 v3(fields.w() - 1.f, fields.h() - 1.f, 1.f);
			vec3 v4(0.f, fields.h() - 1.f, 1.f);

			vec3 u = mul(inverse(mat3(u1, u2, u3)), u4);
			vec3 v = mul(inverse(mat3(v1, v2, v3)), v4);
			mat3 m = mul(mat3(u1 * u.x, u2 * u.y, u3 * u.z), inverse(mat3(v1 * v.x, v2 * v.y, v3 * v.z)));

			for (int yf = 0; yf < fields.h(); yf++) {
				for (int xf = 0; xf < fields.w(); xf++) {
					vec3 r = mul(m, vec3(xf, yf, 1.f));
					r.x = clamp(r.x / r.z, 0.f, float(w - 1));
					r.y = clamp(r.y / r.z, 0.f, float(h - 1));

					int xi = (int)r.x;
					int yi = (int)r.y;
					float wx = r.x - float(xi);
					float wy = r.y - float(yi);

					uint8_t& m0 = input[yi * w + xi];
					uint8_t& m1 = xi + 1 < w? input[yi * w + xi + 1]: m0;
					uint8_t& m2 = yi + 1 < h? input[(yi + 1) * w + xi]: m0;
					uint8_t& m3 = xi + 1 < w && yi + 1 < h? input[(yi + 1) * w + xi + 1]: xi + 1 < w? m1: m2;
					float val = (1.f - wx) * (1.f - wy) * m0 + wx * (1.f - wy) * m1 + (1.f - wx) * wy * m2 + wx * wy * m3;

					fields(xf, yf) = (uint8_t)clamp(val, 0.f, 255.f);
				}
			}
		}
	}
}

