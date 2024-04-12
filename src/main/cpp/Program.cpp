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
	static bool gaps = false;
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
				convert = true;
		} else {
			ImGui::Checkbox("Big", &proc.big_chars);
			ImGui::SameLine();
			ImGui::Checkbox("Faint", &proc.faint_chars);
			ImGui::SameLine();
			ImGui::Checkbox("Gaps", &gaps);
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
		cam.get_lum(bind(&ListProc::scan, &proc, _1, _2, _3));

	static int curpos = INT_MAX;
	if (proc.result(lines, fields) && check_lines()) {
		cam.stop();
		curpos = frow * cols;
		lists.clear();
		convert = true;
	}
	for (int i = 0; !cam.cap() && i < cols && fields.select(curpos, -1); i++, curpos++, learn = false)
		read_field(gaps);

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

	this_thread::sleep_for(chrono::milliseconds(30));
}

void Program::show_results() {
	int line = 0;

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {1.f, 1.f});
	if (lists.size() > 0 && ImGui::BeginTable("", 2 + players, ImGuiTableFlags_Borders)) {
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("99  ").x);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("-999   ").x);

		int n = 1;
		auto& l = lists[0];
		for (int r = 1; r < l.size(); r++) {
			ImGui::PushID(r);
			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			if (l[r].points != 0)
				ImGui::Text("%d", n++);

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			int res = ImGui::InputInt("", &l[r].points, 0);

			for (int i = 0; i < players; i++) {
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-1);
				ImGui::PushID(i);
				if (l[r].scores[i] != l[r - 1].scores[i])
					res += 2 * ImGui::InputInt("", &l[r].scores[i], 0);
				else if (ImGui::Button("-", {ImGui::GetContentRegionAvail().x, 0}))
					res += (l[r].player = i, 1);
				ImGui::PopID();
			}

			if (res == 1 && l[r].player >= 0) {
				l[r].scores = l[r - 1].scores;
				l[r].scores[l[r].player] = l[r - 1].scores[l[r].player] + l[r].points;
			}
			if (res > 0)
				line = r;

			ImGui::PopID();
		}
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();

	read_line(line);

	if (lists.size() > 1 && ImGui::Button("Skip"))
		lists.erase(lists.begin());

	if (lists.size() > 0 && ImGui::Button("Refine"))
		refine_list();
}

bool Program::check_lines() {
	float th = 3.f;

	for (cols = 0; fields.select(cols, 0); cols++);
	for (rows = 0; fields.select(0, rows); rows++);

	vector<float> ws(cols), hs(rows);
	for (int c = 0; c < cols; c++) {
		vector<float> v(rows + 1);
		for (int r = 0; r < v.size(); r++)
			v[r] = length(lines[r * cols + c].x - lines[r * cols + c].y);
		nth_element(v.begin(), v.begin() + rows / 2, v.end());
		ws[c] = v[rows / 2];
	}
	for (int r = 0; r < rows; r++) {
		vector<float> v(cols + 1);
		for (int c = 0; c < v.size(); c++)
			v[c] = length(lines[(rows + 1) * cols + r * (cols + 1) + c].x - lines[(rows + 1) * cols + r * (cols + 1) + c].y);
		nth_element(v.begin(), v.begin() + cols / 2, v.end());
		hs[r] = v[cols / 2];
	}

	for (fcol = 0; fcol + 17 < cols; fcol++) {
		auto mm1 = minmax_element(ws.begin() + fcol + 1, ws.begin() + fcol + 9);
		auto mm2 = minmax({ws[fcol + 11], ws[fcol + 14], ws[fcol + 17]});
		auto mm3 = minmax({ws[fcol + 12], ws[fcol + 13], ws[fcol + 15], ws[fcol + 16]});
		if (abs(*mm1.first - *mm1.second) < th && abs(mm2.first - mm2.second) < th && abs(mm3.first - mm3.second) < th && abs(ws[fcol + 9] - ws[fcol + 10]) < th && mm2.first > 2.f * max(*mm1.second, mm3.second))
			break;
	}

	players = fcol + 20 < cols && abs(ws[fcol + 17] - ws[fcol + 20]) < th? 4: 3;

	for (frow = 0; frow < rows; frow++) {
		int count = 1;
		while (frow + count < rows && abs(hs[frow] - hs[frow + count]) < th)
			count++;
		if (count > rows * 3 / 4)
			break;
	}

	return frow > 0 && frow < rows && fcol + 8 + players * 3 < cols;
}

void Program::read_field(bool gaps) {
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
			if (ma == mem[j * n + i] + mem[j * n + (i > 0? i - 1: 0)] + mem[j * n + (i + 1 < n? i + 1: i)]) {
				best.insert({sum, (i << 4) + j});
				i += o;
			}
		}
	}

	set<int> digs;
	float sum = 0.f;
	for (auto it = best.begin(); it != best.end() && it->first > o * sum / n; sum += it->first, it++)
		digs.insert(it->second);

	for (auto it = digs.begin(); it != digs.end(); it++)
		if ((*it & 15) < 11)
			fields.str().append(chars[*it & 15]);

	n = fields.separate(1);

	if (!gaps)
		return;

	fields.str().clear();
	for (int i = 0; i < n; i++) {
		fields.select(0.f, -1, -1, i + 1);

		for (int j = 0; j < in.size(); j++)
			in[j] = fields.data()[j] / 255.f;
		out = clss.classify(in);

		int c = max_element(out.begin(), out.end()) - out.begin();
		if (c < 11)
			fields.str().append(chars[c]);
	}
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

void Program::read_line(int line) {

	if (lists.size() == 0) {
		lists.push_back(List{Line{0, -1, {0, 0, 0, 0}}});
		dists = {0};
	} else if (line > 0) {
		lists.resize(1);
		lists[0].resize(line + 1);
		dists = {0};
	}

	int i = line > 0? line: lists[0].size() - 1;

	if (lists.front().back().player == -2 || i >= rows)
		return;

	auto f = [&](int x) {fields.select(fcol + x, i); return fields.str();};
	string name = f(0), tips = f(1) + f(2), extra = f(3) + f(4) + f(5) + f(6) + f(7) + f(8);
	int ext = count(extra.begin(), extra.end(), '+');

	multimap<int, tuple<int, int, int>> best;
	for (int l = 0; l < lists.size(); l++) {
		for (int g = 0; g < games.size(); g++) {
			for (int p = 0; p < players; p++) {
				if (players == 4 && (i - frow) % players == p)
					continue;
				best.insert({dists[l] + dist(games[g], true, lists[l][i].scores[p], name, tips, ext, f(9), f(11 + 3 * p)), {games[g].points, p, l}});
				best.insert({dists[l] + dist(games[g], false, lists[l][i].scores[p], name, tips, ext, f(10), f(11 + 3 * p)), {-2 * games[g].points, p, l}});
			}
		}
		best.insert({dists[l] + dist(Game{"", "", 0, 0}, true, 0, name, tips, ext, "", ""), {0, -1, l}});
		int sum = 0;
		for (int p = 0; p < players; p++)
			sum += dist(to_string(abs(lists[l][i].scores[p])), f(11 + 3 * p)) - f(11 + 3 * p).length();
		best.insert({dists[l] + dist(Game{"", "", 0, 0}, true, 0, name, tips, ext, "", "") + sum, {0, -2, l}});
	}

	vector<List> nlists;
	dists.clear();
	set<tuple<int, int, int>> rep;
	for (auto it = best.begin(); it != best.end() && it->first <= best.begin()->first + 1 && nlists.size() < 100; it++) {
		if (rep.count(it->second) != 0)
			continue;
		rep.insert(it->second);

		int v, p, l;
		tie(v, p, l) = it->second;
		nlists.push_back(lists[l]);
		nlists.back().push_back(Line{v, p, lists[l][i].scores});
		if (p >= 0)
			nlists.back().back().scores[p] += v;
		dists.push_back(it->first);
	}
	swap(nlists, lists);
}

void Program::refine_list() {
	set<int> points, rows;
	for (int g = 0; g < games.size(); g++)
		points.insert(games[g].points);

	int p = 0, c = fcol;
	for (int i = 1; i < lists[0].size(); i++) {
		if (lists[0][i].player == p || lists[0][i].player == -2)
			rows.insert(i);
	}

	vector<vector<int>> ls{{0}};
	vector<int> ds{0};

	for (auto rit = rows.begin(); rit != rows.end(); rit++) {
		auto f = [&](int x) {fields.select(c + x, *rit - 1); return fields.str();};

		multimap<int, tuple<int, int>> best;
		for (int l = 0; l < ls.size(); l++) {
			for (auto pit = points.begin(); pit != points.end(); pit++) {
				if (lists[0][*rit].points > 0)
					best.insert({ds[l] + dist(f(9), to_string(*pit)) + dist(f(11 + 3 * p), to_string(abs(ls[l].back() + *pit))), {*pit, l}});
				else if (lists[0][*rit].points < 0)
					best.insert({ds[l] + dist(f(10), to_string(*pit * 2)) + dist(f(11 + 3 * p), to_string(abs(ls[l].back() - *pit * 2))), {*pit * -2, l}});
			}
			if (lists[0][*rit].player == -2)
				best.insert({ds[l] + dist(f(11 + 3 * p), to_string(abs(ls[l].back()))), {0, l}});
		}

		vector<vector<int>> nls;
		vector<int> nds;
		set<int> rep;
		for (auto it = best.begin(); it != best.end() && nls.size() < 10000; it++) {
			int p, l;
			tie(p, l) = it->second;

			if (rep.count(ls[l].back() + p) != 0)
				continue;
			rep.insert(ls[l].back() + p);

			nls.push_back(ls[l]);
			nls.back().push_back(ls[l].back() + p);
			nds.push_back(it->first);
		}
		swap(nls, ls);
		swap(nds, ds);

	}

	for (int n = 0; n < 10;n++)
		cout << ls[n].back() << " " << ds[n] << endl;
	cout<<endl;

}
