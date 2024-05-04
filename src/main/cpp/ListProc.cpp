//Copyright (C) 2023, 2024 Carsten Paproth - Licensed under MIT License

#include "ListProc.h"
#include "utils.h"
#include "linalg.h"
#include <iostream>
#include <fstream>
#include <random>

using namespace std;
using namespace linalg;

vec2 intersect_lines(const vec2& l1, const vec2& l2) {
	return mul(inverse(mat2({cosf(l1.x), cosf(l2.x)}, {sinf(l1.x), sinf(l2.x)})), vec2(l1.y, l2.y));
}

bool Fields::select(float s, int x, int y, int d) {
	X = clamp(x < 0? X: x, 0, width - 1);
	Y = clamp(y < 0? Y: y, 0, height - 1);
	cur = Y * width + X;
	if (cur < 0 || cur >= fields.size())
		return false;
	D = clamp(d < 0? D: d, 0, (int)fields[cur].chars.size());
	if (s > 0.f)
		fields[cur].all.resize(hd * ceil(s * hd));
	return true;
}

bool Fields::select(int x, int y) {
	if (width <= 0)
		return false;
	if (y < 0) {
		y = x / width;
		x = x % width;
	}
	if (x >= width || y >= height)
		return false;
	return select(0.f, x, y, 0);
}

bool Fields::first() {
	rows.clear();
	cols.clear();
	select(0.f, 0, 0, 0);
	return next();
}

bool Fields::next() {
	while (cols.count(X) > 0 || rows.count(Y) > 0) {
		if (X == width - 1 && Y == height - 1)
			return false;
		if (X < width - 1)
			select(0.f, X + 1, Y, 0);
		else
			select(0.f, 0, Y + 1, 0);
	}
	if (D < fields[cur].chars.size()) {
		D++;
		return true;
	} else if (X < width - 1) {
		select(0.f, X + 1, Y, 0);
		return next();
	} else if (Y < height - 1) {
		select(0.f, 0, Y + 1, 0);
		return next();
	}
	return false;
}

int Fields::separate(int mode) {
	D = 0;
	fields[cur].chars.clear();

	if (mode == 0)
		sep_mode0(W(), H(), data(), 3);
	else if (mode == 1)
		sep_mode1(W(), H(), data());
	else if (mode == 2)
		sep_mode2(W(), H(), data());
	else
		sep_mode0(W(), H(), data(), 1);

	for (D = fields[cur].chars.size(); D > 0; D--) {
		uint8_t mi = *min_element(data(), data() + W() * H()), ma = *max_element(data(), data() + W() * H());
		uint8_t cmi = 255, cma = 0;
		for (int y = 0; y < H(); y++) {
			for (int x = 0; x < W(); x++) {
				if (x > 2 && x < W() - 3 && y > 2 && y < H() - 3) {
					cmi = min(cmi, operator()(x, y));
					cma = max(cma, operator()(x, y));
				}
				operator()(x, y) = (operator()(x, y) - mi) * 255 / max(1, ma - mi);
			}
		}
		if (cma - cmi < 64)
			fields[cur].chars.erase(fields[cur].chars.begin() + D - 1);
	}
	return fields[cur].chars.size();
}

void Fields::sep_mode0(int w, int h, uint8_t* field, int s) {
	for (int i = 0; i + wd <= w; i += s) {
		D = fields[cur].chars.size() + 1;
		fields[cur].chars.emplace_back(wd * h);
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < wd; x++) {
				float val = field[y * w + x + i] / 255.f;
				float weight = x < 3? 0.5f + x / 6.f: x + 3 >= wd? 0.5f + (wd - 1.f - x) / 6.f: 1.f;
				operator()(x, y) = clamp(weight * val + 1.f - weight, 0.f, 1.f) * 255.f;
			}
		}
	}
}

void Fields::sep_mode1(int w, int h, uint8_t* field) {
	int count = 0;
	vector<float> avg(w, 0.f), var(w, 0.f);
	vector<int> splits;

	for (int x = 0; x < w; x++) {
		for (int y = 0; y < h; y++)
			avg[x] += field[y * w + x];
		for (int y = 1; y < h; y++)
			var[x] += pow(field[y * w + x] / 255.f - field[(y - 1) * w + x] / 255.f, 2.f);
	}

	for (int x = 1; x + 1 < w; x++)
		if (avg[x] >= avg[x - 1] && avg[x] > avg[x + 1])
			splits.push_back(x);

	while (splits.size() > 0) {
		float diff = -1.f;
		int pos = 0;
		for (int i = 0; i + 1 < splits.size(); i++) {
			float d = abs(var[splits[i + 1]] - var[splits[i]]) / (splits[i + 1] - splits[i]);
			if (d > diff && (splits[i + 1] - splits[i] <= wd / 2 || d > 0.05f || count > 0)) {
				diff = d;
				pos = i;
			}
		}
		if (diff < 0.f)
			break;
		if (count > 0 && diff <= 0.05f && splits[pos + 1] - splits[pos] > wd / 2)
			count--;
		if (var[splits[pos + 1]] > var[splits[pos]])
			pos++;
		splits.erase(splits.begin() + pos);
	}

	for (int i = 0; i + 1 < splits.size(); i++) {
		D = fields[cur].chars.size() + 1;
		fields[cur].chars.emplace_back(wd * h);

		float ma = *max_element(&avg[splits[i]] + 1, &avg[splits[i + 1]]), sum = 0.f, cent = 0.f;
		for (int x = splits[i] + 1; x < splits[i + 1]; x++) {
			cent += x * (ma - avg[x]);
			sum += ma - avg[x];
		}

		int o = (int)(cent / sum - (wd - 1.f) / 2.f + 0.5f);
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < wd; x++) {
				float val = x + o < 0 || x + o >= w? 1.f: field[y * w + x + o] / 255.f;
				float weight = min(1.f, -0.2f * min({0.f, x + o - 1.f - splits[i], splits[i + 1] - 1.f - x - o}));
				operator()(x, y) = clamp((1.f - weight) * val + weight, 0.f, 1.f) * 255.f;
			}
		}
	}
}

void Fields::sep_mode2(int w, int h, uint8_t* field) {
	int md = h / 2;
	vector<float> values(w * h);
	vector<int> ind(values.size());

	for (int i = 0; i < values.size(); i++)
		values[i] = field[i];

	for (int y = 1; y <= md; y++) {
		for (int x = 0; x < w; x++) {
			float* p = max_element(&values[(y - 1) * w + (x > 0? x - 1: x)], &values[(y - 1) * w + (x + 1 < w? x + 1: x)] + 1);
			values[y * w + x] += *p;
			ind[(y - 1) * w + x] = x + (p - &values[(y - 1) * w + x]);
		}
	}
	for (int y = h - 2; y >= md; y--) {
		for (int x = 0; x < w; x++) {
			float* p = max_element(&values[(y + 1) * w + (x > 0? x - 1: x)], &values[(y + 1) * w + (x + 1 < w? x + 1: x)] + 1);
			values[y * w + x] += *p;
			ind[(y + 1) * w + x] = x + (p - &values[(y + 1) * w + x]);
		}
	}

	vector<pair<int, float>> mf;
	mf.emplace_back(0, values[md * w]);
	for (int x = 1; x + 1 < w; x++) {
		float& f = values[md * w + x];
		if ((f < mf.back().second || f > values[md * w + x + 1]) && (f > mf.back().second || f < values[md * w + x + 1]))
			mf.emplace_back(x, f);
	}
	mf.emplace_back(w - 1, values[md * w + w - 1]);

	for (int i = 0; i + 3 < mf.size(); i++) {
		float mii = min(mf[i + 1].second, mf[i + 2].second);
		float mio = min(mf[i].second, mf[i + 3].second);
		float mai = max(mf[i + 1].second, mf[i + 2].second);
		float mao = max(mf[i].second, mf[i + 3].second);
		if (mii >= mio && mai <= mao && mao - mio >= 8.f * (mai - mii)) {
			mf.erase(mf.begin() + i + 2);
			mf.erase(mf.begin() + i + 1);
			i = -1;
		}
	}
	if (mf.size() > 1 && mf[0].second <= mf[1].second)
		mf.erase(mf.begin());
	if (mf.size() > 1 && mf[mf.size() - 1].second <= mf[mf.size() - 2].second)
		mf.pop_back();
	for (int i = 0; i + 4 < mf.size(); i += 2) {
		float dol = mf[i].second - mf[i + 3].second;
		float dor = mf[i + 4].second - mf[i + 1].second;
		float dil = mf[i + 2].second - mf[i + 1].second;
		float dir = mf[i + 2].second - mf[i + 3].second;
		if (mf[i + 3].first - mf[i + 1].first < md && (dol + dor >= 2.f * (dil + dir) || dol >= 4.f * dil || dor >= 4.f * dir)) {
			mf[i + 2].first = lround((dil * mf[i + 1].first + dir * mf[i + 3].first) / (dil + dir));
			mf[i + 2].second = min(mf[i + 1].second, mf[i + 3].second);
			mf.erase(mf.begin() + i + 3);
			mf.erase(mf.begin() + i + 1);
			i = -2;
		}
	}

	for (int i = 1; i + 1 < mf.size(); i += 2) {
		D = fields[cur].chars.size() + 1;
		fields[cur].chars.emplace_back(W() * H());

		vector<int> l(h), r(h);
		l[md] = mf[i - 1].first;
		r[md] = mf[i + 1].first;
		for (int y = md; y > 0; y--) {
			l[y - 1] = ind[(y - 1) * w + l[y]];
			r[y - 1] = ind[(y - 1) * w + r[y]];
		}
		for (int y = md; y + 1 < h; y++) {
			l[y + 1] = ind[(y + 1) * w + l[y]];
			r[y + 1] = ind[(y + 1) * w + r[y]];
		}

		for (int y = 0; y < H(); y++) {
			for (int x = 0; x < W(); x++) {
				int xd = mf[i].first + x < l[y] + W() / 2? l[y]: mf[i].first + x > r[y] + W() / 2? r[y]: mf[i].first + x - W() / 2;
				operator()(x, y) = field[y * w + xd];
			}
		}
	}
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

void ListProc::scan(const vector<uint8_t>& l, int32_t width, int32_t height) {
	if (worker.joinable())
		return;

	w = width;
	h = height;
	input = l;
	input.resize(w * h);

	ifstream file("/storage/emulated/0/Download/img2.480.ubyte");
	//file.read((char*)input.data(), input.size());

	worker = thread(&ListProc::process, this);
}

bool ListProc::result(Lines& l, Fields& f) {
	if (!finished)
		return false;

	worker.join();
	finished = false;

	swap(lines, l);
	swap(fields, f);

//	ofstream file("/storage/emulated/0/Download/img.480.ubyte");
//	file.write((char*)input.data(), input.size());
//	ofstream ofile("/storage/emulated/0/Download/img.txt");
//	ofile << "huch\n";
//	ofile.close();
//	ifstream ifile("/storage/emulated/0/Download/img.txt");
//	string str;
//	ifile >> str;
//	cout << str << endl;

	return l.size() > 20 && l.size() == lines.size();
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
	for (int x = max(2, (int)p.x - 4); x < min(w - 2, (int)p.x + 5); x++) {
		for (int y = max(2, (int)p.y - 4); y < min(h - 2, (int)p.y + 5); y++) {
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
	return length(p - n) < 2.f? n: p;
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

	int wp = vl.size(), hp = hl.size();
	lines.clear();
	fields.resize(wp - 1, hp - 1);
	if (wp * hp == 0)
		return;
	vector<vec2> points(wp * hp);
	vector<int> pos;
	for (int y = 0; y < hp; y++) {
		for (int x = 0; x < wp; x++) {
			points[y * wp + x] = intersect_lines(vl[x], hl[y] + vec2(M_PI / 2.f, 0.f)) + o;
			pos.insert(pos.end(), 3, y * wp + x);
		}
	}

	shuffle(pos.begin(), pos.end(), default_random_engine(time(0)));
	for (int i = 0; i < pos.size(); i++) {
		vec2 point = points[pos[i]];
		vec2 offset = find_corner(point, corners) - point;
		if (length(offset) == 0.f)
			continue;
		for (int j = 0; j < points.size(); j++)
			points[j] += 0.5f * offset * exp(-0.7f * pow(length(points[j] - point) / 50.f, 2.f));
	}

	for (int y = 0; y < hp; y++)
		for (int x = 0; x + 1 < wp; x++)
			lines.emplace_back(points[y * wp + x], points[y * wp + x + 1]);
	for (int y = 0; y + 1 < hp; y++)
		for (int x = 0; x < wp; x++)
			lines.emplace_back(points[y * wp + x], points[(y + 1) * wp + x]);

	for (int y = 0; y + 1 < hp; y++) {
		for (int x = 0; x + 1 < wp; x++) {
			vec3 u1(points[y * wp + x], 1.f);
			vec3 u2(points[y * wp + x + 1], 1.f);
			vec3 u3(points[(y + 1) * wp + x + 1], 1.f);
			vec3 u4(points[(y + 1) * wp + x], 1.f);

			if (length(u4 - u1) + length(u3 - u2) == 0.f)
				continue;
			fields.select((length(u2 - u1) + length(u3 - u4)) / (length(u4 - u1) + length(u3 - u2)) * (big_chars? 0.8f: 1.f), x, y);

			vec3 v1(0.f, 0.f, 1.f);
			vec3 v2(fields.W() - 1.f, 0.f, 1.f);
			vec3 v3(fields.W() - 1.f, fields.H() - 1.f, 1.f);
			vec3 v4(0.f, fields.H() - 1.f, 1.f);

			vec3 u = mul(inverse(mat3(u1, u2, u3)), u4);
			vec3 v = mul(inverse(mat3(v1, v2, v3)), v4);
			mat3 m = mul(mat3(u1 * u.x, u2 * u.y, u3 * u.z), inverse(mat3(v1 * v.x, v2 * v.y, v3 * v.z)));

			uint8_t mi = 255, ma = 0;
			for (int yf = 0; yf < fields.H(); yf++) {
				for (int xf = 0; xf < fields.W(); xf++) {
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

					fields(xf, yf) = clamp(val, 0.f, 255.f);
					mi = min(mi, fields(xf, yf));
					ma = max(ma, fields(xf, yf));
				}
			}
			for (int yf = 0; yf < fields.H(); yf++)
				for (int xf = 0; xf < fields.W(); xf++)
					fields(xf, yf) = clamp((fields(xf, yf) - mi) * 255.f / (ma - mi), 0.f, 255.f);

		}
	}
}

