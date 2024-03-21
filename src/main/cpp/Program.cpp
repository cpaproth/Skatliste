//Copyright (C) 2023, 2024 Carsten Paproth - Licensed under MIT License

#include "Program.h"
#include "imgui.h"
#include <map>

#include <iostream>

using namespace std;

const vector<const char*> Program::chars{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "+", "-"};

Program::Program() : cam(480, 640, 0), clss(Fields::wd, Fields::hd, chars.size()) {
	glEnable(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (auto tex : {&cap_tex, &dig_tex}) {
		glGenTextures(1, tex);
		glBindTexture(GL_TEXTURE_2D, *tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	for (int n = 9; n <= 12; n++) {
		for (int g = 2; g <= 18; g++) {
			for (int t = max(1, g - 7); t <= min(11, g - 1); t++)
				games.push_back(Game{to_string(n), to_string(t), g - t - 1, g * n});
		}
	}
	for (int g = 2; g <= 11; g++) {
		for (int t = max(1, g - 7); t <= min(4, g - 1); t++)
			games.push_back(Game{"24", to_string(t), g - t - 1, g * 24});
	}
	games.push_back(Game{"23", "", 0, 23});
	games.push_back(Game{"23", "", 1, 35});
	games.push_back(Game{"23", "", 1, 46});
	games.push_back(Game{"23", "", 2, 59});
	games.push_back(Game{"35", "", 0, 35});
	games.push_back(Game{"46", "", 0, 46});
	games.push_back(Game{"59", "", 0, 59});
}

Program::~Program() {
	for (auto tex : {&cap_tex, &dig_tex})
		glDeleteTextures(1, tex);
}

void Program::draw() {

	static bool learn = false;
	static bool convert = false;
	if (convert) {
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
		ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
	}
	ImGui::Begin(convert? "Skat List##1": "Skat List##2", learn? &learn: convert? &convert: 0);
	if (!learn && !convert) {
		if (ImGui::Button(cam.cap()? "Stop": "Scan"))
			cam.cap()? cam.stop(): cam.start();
		ImGui::SameLine();
		if (!cam.cap() && fields.select()) {
			if (ImGui::Button("Learn"))
				learn = fields.first();
			ImGui::SameLine();
			if (ImGui::Button("Convert"))
				convert = read_list(0);
		} else {
			ImGui::Checkbox("Big", &proc.big_chars);
		}

		ImGui::SliderInt("Edge", &proc.edge_th, 1, 100);
		ImGui::SliderInt("Line", &proc.line_th, 1, 100);

		if (fields.select()) {
			glBindTexture(GL_TEXTURE_2D, dig_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, fields.W(), fields.H(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, fields.data());
			ImGui::Image((void*)(intptr_t)dig_tex, {fields.W() * 4.f, fields.H() * 4.f});
			ImGui::Text("%s", fields.str().c_str());
			ImGui::InputInt("FieldX", &fields.X);
			ImGui::InputInt("FieldY", &fields.Y);
			ImGui::InputInt("FieldD", &fields.D);
		}
	} else if (convert) {
		show_results();
	} else if (fields.select()) {
		ImGui::BeginGroup();
		int val = ImGui::Button("S0") + ImGui::Button("S1") * 2 + ImGui::Button("S2") * 3 - 1;
		if (val >= 0) {
			fields.separate(val);
			learn = fields.next();
		}
		ImGui::EndGroup();

		ImGui::SameLine();
		glBindTexture(GL_TEXTURE_2D, dig_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, fields.W(), fields.H(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, fields.data());
		ImGui::Image((void*)(intptr_t)dig_tex, {fields.W() * 10.f, fields.H() * 10.f});

		ImGui::SameLine();
		ImGui::BeginGroup();
		if (ImGui::Button("Ignore Row"))
			learn = fields.ignore_row();
		if (ImGui::Button("Ignore Column"))
			learn = fields.ignore_col();
		if (ImGui::Button("Ignore"))
			learn = fields.next();
		ImGui::EndGroup();

		uint8_t cl1, prob1, cl2, prob2, cln, diff;
		tie(cl1, prob1, cl2, prob2, cln, diff) = clss.classify(fields.data());
		ImGui::Text("%s %d%% %s %d%% %s %d", chars[cl1], prob1, chars[cl2], prob2, chars[cln], diff);
		val = -1;
		for (int i = 0; i < chars.size(); i++) {
			if (i % 4 != 0)
				ImGui::SameLine();
			if (ImGui::Button(chars[i], {70, 70}))
				val = i;
		}
		if (val >= 0) {
			if (val != cln || diff > 20)
				clss.learn(fields.data(), val);
			learn = fields.next();
		}
	}
	ImGui::End();

	using namespace placeholders;

	glBindTexture(GL_TEXTURE_2D, cap_tex);
	if (cam.cap())
		cam.get_rgba(bind(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA, _2, _3, 0, GL_RGBA, GL_UNSIGNED_BYTE, _1));
	else
		proc.get_input(bind(glTexImage2D, GL_TEXTURE_2D, 0, GL_LUMINANCE, _2, _3, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, _1));

	auto s = ImGui::GetMainViewport()->Size;
	float w = float(cam.w()), h = float(cam.h()), f = s.x * h < s.y * w? s.x / w: s.y / h;
	ImGui::GetBackgroundDrawList()->AddImage((void*)(intptr_t)cap_tex, {0.f, 0.f}, {f * w, f * h});

	if (cam.cap())
		cam.swap_lum(bind(&ListProc::scan, &proc, _1, _2, _3));

	static int curpos = INT_MAX;
	if (proc.result(lines, fields)) {
		cam.stop();
		curpos = 0;
	}
	for (int i = 0; !cam.cap() && i < 40 && fields.select(curpos, -1); i++, curpos++, learn = convert = false)
		read_field();

	int pos = 0;
	for (auto& l : lines)
		ImGui::GetBackgroundDrawList()->AddLine({f * l.x.x, f * l.x.y}, {f * l.y.x, f * l.y.y}, fields.check(pos++)? 0xff00ff00: 0xff0000ff);

	if (!ImGui::GetIO().WantCaptureMouse && ImGui::GetIO().MouseClicked[0] && ImGui::GetIO().MousePos.y / f < h) {
		float l = FLT_MAX;
		pos = 0;
		vec2 mouse(ImGui::GetIO().MousePos.x / f, ImGui::GetIO().MousePos.y / f);
		for (int i = 0; i + 1 < lines.size(); i++) {
			vec2 d = (lines[i].x + lines[i].y + lines[i + 1].x + lines[i + 1].y) / 4.f - mouse;
			if (length(d) < l) {
				l = length(d);
				pos = i;
			}
		}
		fields.select(pos);
		learn = learn? fields.next(): learn;
	}

	if (!fields.select(curpos, -1))
		this_thread::sleep_for(chrono::milliseconds(30));
}

void Program::show_results() {
	int start = 0;

	if (ImGui::BeginTable("", 7)) {
		int n = 1;
		auto& l = lists[0];
		for (int r = 1; r < l.size(); r++) {
			ImGui::PushID(r);
			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			if (l[r].points != 0)
				ImGui::Text("%d", n++);

			ImGui::TableNextColumn();
			int res = ImGui::InputInt("##1", &l[r].points, 0);

			ImGui::TableNextColumn();
			if (l[r].points != 0)
				res += ImGui::SliderInt("##2", &l[r].player, 0, 3);

			for (int i = 0; i < 4; i++) {
				ImGui::TableNextColumn();
				ImGui::PushID(i);
				if (l[r].scores[i] != l[r - 1].scores[i])
					res += 2 * ImGui::InputInt("##3", &l[r].scores[i], 0);
				else
					ImGui::Text("-");
				ImGui::PopID();
			}

			if (res == 1 && l[r].player >= 0) {
				l[r].scores = l[r - 1].scores;
				l[r].scores[l[r].player] = l[r - 1].scores[l[r].player] + l[r].points;
			}
			if (res > 0)
				start = r;

			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	if (start > 0)
		read_list(start);
}

void Program::read_field() {
	fields.str().clear();
	int n = fields.separate(3), o = Fields::wd / 2;
	NeuralNet::Values mem(n * chars.size()), in(Fields::wd * Fields::hd), out;

	for (int i = 0; i < n; i++) {
		fields.select(0.f, -1, -1, i + 1);

		for (int j = 0; j < in.size(); j++)
			in[j] = fields.data()[j] / 255.f;
		out = clss.classify(in);

		for (int j = 0; j < out.size(); j++)
			mem[j * n + i] = out[j];
	}

	multimap<float, int, greater<float>> best;
	for (int j = 0; j < chars.size(); j++) {
		for (int i = 0; i < n; i++) {
			float sum = 0.f, ma = 0.f;
			for (int k = i >= o? i - o: 0; k < n && k <= i + o; k++) {
				sum += mem[j * n + k];
				ma = max(ma, mem[j * n + k] + mem[j * n + (k > 0? k - 1: 0)] + mem[j * n + (k + 1 < n? k + 1: k)]);
			}
			if (ma == mem[j * n + i] + mem[j * n + (i > 0? i - 1: 0)] + mem[j * n + (i + 1 < n? i + 1: i)])
				best.insert({sum, (i << 4) + j});
		}
	}

	set<int> digs;
	float sum = 0.f;
	for (auto it = best.begin(); it != best.end() && it->first > o * sum / n; sum += it->first, it++)
		digs.insert(it->second);

	for (auto it = digs.begin(); it != digs.end(); it++)
		if ((*it & 15) < 11)
			fields.str().append(chars[*it & 15]);

	fields.separate(1);
}

int Program::dist(const string& s, const string& t) {
	vector<int> v0(t.length() + 1), v1(t.length() + 1);

	for (int i = 0; i < v0.size(); i++)
		v0[i] = i;

	for (int i = 0; i < s.length(); i++) {
		v1[0] = i + 1;
		for (int j = 0; j < t.length(); j++)
			v1[j + 1] = min({v0[j + 1] + 1, v1[j] + 1, v0[j] + (s[i] == t[j]? 0: 1)});
		swap(v0, v1);
	}

	return v0[t.length()];
}

int Program::dist(const Game& game, bool win, int last, const string& name, const string& tips, int extra, const string& points, const string& score) {
	int res = dist(game.name, name) + dist(game.tips, tips) + abs(game.extra - extra) - points.length() - score.length();
	if (game.points == 0) {
		res += points.length() + score.length();
	} else if (win) {
		res += dist(to_string(game.points), points);
		res += dist(to_string(abs(last + game.points)), score);
	} else {
		res += dist(to_string(2 * game.points), points);
		res += dist(to_string(abs(last - 2 * game.points)), score);
	}
	return res;
}

bool Program::read_list(int start) {
	vector<int> scol, srow;

	for (int y = 0; fields.select(0, y); y++) {
		srow.resize(max((int)srow.size(), y + 1), 0);
		for (int x = 0; fields.select(x, y); x++) {
			scol.resize(max((int)scol.size(), x + 1), 0);
			srow[y] += fields.str().length();
			scol[x] += fields.str().length();
		}
	}

	int n = 3, c = 0;
	while (c + 17 < scol.size() && !(scol[c] > scol[c + 1] && scol[c + 8] < scol[c + 9] && scol[c + 11] > scol[c + 12] && scol[c + 13] < scol[c + 14] && scol[c + 14] > scol[c + 15] && scol[c + 16] < scol[c + 17]))
		c++;
	if (c + 20 < scol.size() && scol[c + 17] > scol[c + 18] && scol[c + 19] < scol[c + 20])
		n++;
	if (c + 9 + n * 3 > scol.size())
		return false;

	if (start == 0) {
		lists.clear();
		lists.push_back(List{Line{0, 0, {0, 0, 0, 0}}});
	} else {
		lists.resize(1);
		lists[0].resize(start + 1);
	}

	for (int i = start; i < srow.size(); i++) {
		auto f = [&](int x) {fields.select(c + x, i); return fields.str();};
		string name = f(0), tips = f(1) + f(2), extra = f(3) + f(4) + f(5) + f(6) + f(7) + f(8);
		int ext = count(extra.begin(), extra.end(), '+');

		multimap<int, tuple<int, int, int>> best;
		for (int l = 0; l < lists.size(); l++) {
			for (int g = 0; g < games.size(); g++) {
				for (int p = 0; p < n; p++) {
					best.insert({dist(games[g], true, lists[l][i].scores[p], name, tips, ext, f(9), f(11 + 3 * p)), {games[g].points, p, l}});
					best.insert({dist(games[g], false, lists[l][i].scores[p], name, tips, ext, f(10), f(11 + 3 * p)), {-2 * games[g].points, p, l}});
				}
			}
			best.insert({dist(Game{"", "", 0, 0}, true, 0, name, tips, ext, "", ""), {0, -1, l}});
			int sum = 0;
			for (int p = 0; p < n; p++)
				sum += dist(to_string(abs(lists[l][i].scores[p])), f(11 + 3 * p)) - f(11 + 3 * p).length();
			best.insert({dist(Game{"", "", 0, 0}, true, 0, name, tips, ext, "", "") + sum, {0, -2, l}});
		}

		vector<List> nlists;
		set<tuple<int, int, int>> rep;
		for (auto it = best.begin(); it != best.end() && it->first == best.begin()->first; it++) {
			if (rep.count(it->second) != 0)
				continue;
			rep.insert(it->second);

			int v, p, l;
			tie(v, p, l) = it->second;
			nlists.push_back(lists[l]);
			nlists.back().push_back(Line{v, p, lists[l][i].scores});
			if (p >= 0)
				nlists.back().back().scores[p] += v;
		}
		swap(nlists, lists);

		if (start > 0 && (lists.size() > 1 || best.begin()->first > 0))
			break;

		if (lists.back().back().player == -2)
			break;
	}

	return true;
}

