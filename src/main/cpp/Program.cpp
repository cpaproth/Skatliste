//Copyright (C) 2023, 2024 Carsten Paproth - Licensed under MIT License

#include "Program.h"
#include "imgui.h"
#include <map>

#include <iostream>

using namespace std;

const vector<const char*> Program::chars{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "+", "-"};

Program::Program() : cam(480, 640, 0), clss(Fields::wd, Fields::hd, chars.size()), converting(false) {
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
	converting = false;
	if (worker.joinable())
		worker.join();
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
		converting = false;
		if (worker.joinable())
			worker.join();

		if (ImGui::Button(cam.cap()? "Stop": "Scan"))
			cam.cap()? cam.stop(): cam.start();
		if (!cam.cap() && fields.select()) {
			ImGui::SameLine();
			if (ImGui::Button("Learn"))
				learn = fields.first();
			ImGui::SameLine();
			if (ImGui::Button("Rescan"))
				proc.rescan();
		}

		ImGui::Checkbox("Big", &proc.big_chars);
		ImGui::SameLine();
		ImGui::Checkbox("Faint", &proc.faint_chars);
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

	if (proc.result(lines, fields) && check_lines()) {
		cam.stop();
		convert = true;
	}

	int pos = 0;
	for (auto& l : lines)
		ImGui::GetBackgroundDrawList()->AddLine({f * l.x.x, f * l.x.y}, {f * l.y.x, f * l.y.y}, fields.check(pos++)? 0xff00ff00: 0xff0000ff);

	if (!ImGui::GetIO().WantCaptureMouse && ImGui::GetIO().MouseClicked[0] && ImGui::GetIO().MousePos.y / f < h && !convert) {
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

	if (!worker.joinable()) {
		toplist.clear();
		worker = thread(&Program::process, this);
	} else if (!converting) {
		worker.join();
		worker = thread(&Program::process, this);
	}

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {1.f, 1.f});
	if (ImGui::BeginTable("", 2 + players, ImGuiTableFlags_Borders)) {
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("99  ").x);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("-999   ").x);

		lock_guard<std::mutex> lg(mut);
		auto& l = toplist;
		vector<int> won(players, 0), lost(players, 0), score(players, 0);
		int sumw = 0, suml = 0;

		for (int r = 1; r < l.size(); r++) {
			if (l[r].points > 0 && l[r].player >= 0) {
				won[l[r].player]++;
				score[l[r].player] += l[r].points;
				sumw++;
			} else if (l[r].points < 0 && l[r].player >= 0) {
				lost[l[r].player]++;
				score[l[r].player] += l[r].points;
				suml++;
			}
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		ImGui::Text("%d", sumw + suml);
		ImGui::TableNextColumn();
		for (int i = 0; i < players && l.size() > 0; i++) {
			ImGui::PushID(i);
			ImGui::PushStyleColor(ImGuiCol_Button, {0.f, 0.5f, 0.5f, 1.f});
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			ImGui::Button((to_string(score[i]) + "##1").c_str(), {ImGui::GetContentRegionAvail().x, 0});
			ImGui::Text("%d/%d", won[i], lost[i]);
			ImGui::Text("%d", (won[i] - lost[i]) * 50);
			ImGui::Text("%d", (suml - lost[i]) * (players == 3? 40: 30));
			ImGui::Button((to_string(score[i] + (won[i] - lost[i]) * 50 + (suml - lost[i]) * (players == 3? 40: 30)) + "##2").c_str(), {ImGui::GetContentRegionAvail().x, 0});
			ImGui::Text("");
			ImGui::PopStyleColor();
			ImGui::PopID();
		}

		int n = 1;
		for (int r = 1; r < l.size(); r++) {
			int res = 0;

			ImGui::PushID(r);
			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			if (l[r].points != 0 &&	ImGui::Button(to_string(n++).c_str(), {ImGui::GetContentRegionAvail().x, 0}))
				res += (l[r].points = 0, 1);

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			res += ImGui::InputInt("", &l[r].points, 0);

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
			if (res > 0) {
				l.resize(r + 1);
				converting = false;
			}

			ImGui::PopID();
		}
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();
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
		if (*mm1.second - *mm1.first < th && mm2.second - mm2.first < th && mm3.second - mm3.first < th && abs(ws[fcol + 9] - ws[fcol + 10]) < th && mm2.first > 2.f * max(*mm1.second, mm3.second))
			break;
	}

	players = fcol + 20 < cols && abs(ws[fcol + 17] - ws[fcol + 20]) < th? 4: 3;

	for (frow = 0; frow < rows; frow++) {
		int count = 1;
		while (frow + count < rows && abs(hs[frow] - hs[frow + count]) < th)
			count++;
		if (count > rows / 2)
			break;
	}

	float sum = 0.f;
	for (int c = fcol; c < cols && c < fcol + 9 + players * 3; c++)
		sum += ws[c];

	return frow > 0 && frow < min(5, rows) && fcol + 9 + players * 3 <= cols && sum > cam.w() * 0.75f;
}

void Program::read_field() {
	fields.str().clear();
	int n = fields.separate(3), o = Fields::wd / 2;
	NeuralNet::Values mem(n * chars.size(), 0.f), in(Fields::wd * Fields::hd), out;

	for (int i = 0; i < n; i++) {
		fields.select(0.f, -1, -1, i + 1);

		for (int j = 0; j < in.size(); j++)
			in[j] = pow(fields.data()[j] / 255.f, proc.faint_chars? 2.f: 1.f);
		out = clss.classify(in);

		for (int j = 0; j < out.size(); j++)
			for (int k = max(0, i - o / 2); k < min(n, i + o / 2 + 1); k++)
				mem[j * n + k] += out[j];
	}

	multimap<float, int, greater<float>> best;
	for (int j = 0; j < chars.size(); j++) {
		for (int i = 0; i < n; i++) {
			float ma = *max_element(&mem[j * n + max(0, i - o)], &mem[j * n + min(n, i + o + 1)]);
			if (ma == mem[j * n + i]) {
				best.insert({ma, i * 16 + j});
				i += o;
			}
		}
	}

	set<int> digs;
	float sum = 0.f;
	for (auto it = best.begin(); it != best.end() && it->first > o * sum / n; sum += it->first, it++) {
		auto dit = digs.lower_bound(it->second & -16);
		if (dit == digs.end() || (*dit / 16 - it->second / 16) != 0)
			digs.insert(it->second);
	}

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

/*void Program::refine_list() {
	set<int> points, rows;
	for (int g = 0; g < games.size(); g++)
		points.insert(games[g].points);

	int p = 1, c = fcol;
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

	for (int n = 0; n < 20;n++)
		cout << ls[n].back() << " " << ds[n] << endl;
	cout<<endl;

}*/

void Program::process() {
	converting = true;

	vector<List> lists;
	vector<int> dists(1, 0);

	if (toplist.size() == 0) {
		lists.push_back(List{Line{0, -1, {0, 0, 0, 0}}});
	} else {
		lock_guard<std::mutex> lg(mut);
		lists.push_back(toplist);
	}

	for (int i = lists[0].size() - 1; i < rows; i++) {

		for (int c = 0; c < cols && i >= frow && fields.select(c, i); c++)
			read_field();

		auto f = [&](int x) {fields.select(fcol + x, i); return fields.str();};
		string name = f(0), tips = f(1) + f(2);
		int extra = !f(3).empty() + !f(4).empty() + !f(5).empty() + !f(6).empty() + !f(7).empty() + !f(8).empty();

		multimap<int, tuple<int, int, int>> best;
		for (int l = 0; l < lists.size(); l++) {
			for (int g = 0; g < games.size(); g++) {
				for (int p = 0; p < players; p++) {
					if (players == 4 && (i - frow) % players == p)
						continue;
					best.insert({dists[l] +
								 dist(games[g], true, lists[l][i].scores[p], name, tips, extra,
									  f(9), f(11 + 3 * p)), {games[g].points, p, l}});
					best.insert({dists[l] +
								 dist(games[g], false, lists[l][i].scores[p], name, tips, extra,
									  f(10), f(11 + 3 * p)), {-2 * games[g].points, p, l}});
				}
			}
			best.insert({dists[l] + dist(Game{"", "", 0, 0}, true, 0, name, tips, extra, "", ""), {0, -1, l}});
			int sum = 0;
			for (int p = 0; p < players; p++)
				sum += dist(to_string(abs(lists[l][i].scores[p])), f(11 + 3 * p)) - f(11 + 3 * p).length();
			best.insert({dists[l] + dist(Game{"", "", 0, 0}, true, 0, name, tips, extra, "", "") + sum, {0, -2, l}});

			if (!converting)
				return;
		}

		vector<List> nlists;
		dists.clear();
		set<tuple<int, int, int>> rep;
		int o = lists.size() > 1000? 0: 1;
		for (auto it = best.begin(); it != best.end() && it->first <= best.begin()->first + o; it++) {
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

			if (!converting)
				return;
		}
		swap(nlists, lists);

		lock_guard<std::mutex> lg(mut);
		toplist = lists[0];

		if (lists.front().back().player == -2)
			break;

	}
}
