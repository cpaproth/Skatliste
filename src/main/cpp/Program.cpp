//Copyright (C) 2023, 2024 Carsten Paproth - Licensed under MIT License

#include "Program.h"
#include "imgui.h"
#include <map>
#include <fstream>
#include <iostream>

using namespace std;

const vector<const char*> Program::chars{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "+", "-"};

Program::Program(const string& p) : path(p), cam(480, 640, 0), clss(p, Fields::wd, Fields::hd, chars.size()), converting(false) {
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

	ifstream file(path + "/settings.txt");
	file >> playersfile;
	load_players();
	players.push_back({"Hans Franz", true, 100, {200, 300}});
	players.push_back({"Klaus Glück", true, 100, {200, 300}});
	players.push_back({"Sabine Gutherz", true, 100, {200, 300}});
	cout << "players: " << players.size() << endl;

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
	ofstream file(path + "/settings.txt");
	file << playersfile;
	save_players();
}

void Program::load_players() {

}

void Program::save_players() {

}

void Program::draw() {
	if (convert || list) {
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
		ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
	}

	ImGui::Begin(convert || list? "Skat List##1": "Skat List##2", list? &list: learn? &learn: convert? &convert: 0);
	if (list) {
		show_players();
	} else if (convert) {
		show_results();
	} else if (!learn) {
		converting = false;
		if (worker.joinable())
			worker.join();

		if (ImGui::Button(cam.cap()? "Stop": "Scan"))
			cam.cap()? cam.stop(): cam.start();
		ImGui::SameLine();
		if (ImGui::Button("List"))
			list = true;
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

void Program::show_players() {
	static char csv[30] = "";
	static char name[20] = "";
	static char date[11] = "";
	static bool show = true;
	static int mode = 0;
	auto sortname = [](const Player& a, const Player& b) {mode = 0; return a.plays && !b.plays || a.plays == b.plays && a.name < b.name;};
	auto sortscore = [](const Player& a, const Player& b) {mode = 1; return a.plays && !b.plays || a.plays == b.plays && a.score > b.score;};

	ImGui::PushItemWidth(ImGui::CalcTextSize("longfilename.csv  ").x);
	csv[playersfile.copy(csv, sizeof(csv) - 1)] = 0;
	ImGui::InputText("##0", csv, sizeof(csv));
	playersfile = csv;
	ImGui::PopItemWidth();
	ImGui::SameLine();
	if (ImGui::Button("Load"))
		load_players();
	ImGui::SameLine();
	if (ImGui::Button("Save"))
		save_players();


	if (ImGui::BeginTable("##1", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY)) {
		ImGui::TableSetupScrollFreeze(1, 1);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, show? 0.f: ImGui::CalcTextSize("99. Www").x);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("-999   ---+++").x);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("00.00.0000  ").x);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##1", name, sizeof(name));
		if (ImGui::ArrowButton("", show? ImGuiDir_Left: ImGuiDir_Right))
			show = !show;
		ImGui::SameLine();
		auto it = find_if(players.begin(), players.end(), [](const Player& p) {return p.name == name;});
		if (ImGui::Button(name[0] == 0? "Sort##1": it == players.end()? "Add##1": "Del##1")) {
			if (it == players.end() && name[0] != 0)
				players.push_back({name, true});
			else if (name[0] != 0)
				players.erase(it);
			sort(players.begin(), players.end(), sortname);
			name[0] = 0;
		}

		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		ImGui::InputInt("##2", &result);
		if (ImGui::Button("Sort##2"))
			sort(players.begin(), players.end(), sortscore);
		it = find_if(players.begin(), players.end(), [](const Player& p) {return p.plays && p.score == 0;});
		if (it == players.end()) {
			ImGui::SameLine();
			if (ImGui::Button("Add##2"))
				for_each(players.begin(), players.end(), [](Player& p) {p.score = 0;});
		}

		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##3", date, sizeof(date));
		if (ImGui::Button(date[0] == 0? "Sort##3": date[0] == 'X'? "Del##3": "Add##3")) {
			if (date[0] == 0) {
				sort(players.begin(), players.end(), sortscore);
			} else if (date[0] == 'X') {
				for_each(players.begin(), players.end(), [](Player& p) {p = {p.name, false, 0, {}};});
				sort(players.begin(), players.end(), sortname);
			} else {
				for_each(players.begin(), players.end(), [](Player& p) {p = {p.name, false, 0, {}};});
				sort(players.begin(), players.end(), sortname);
			}
			date[0] = 0;
		}


		for (int r = 0; r < players.size(); r++) {
			ImGui::PushID(r);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			if (ImGui::Selectable((to_string(r + 1) + ". " + players[r].name).c_str(), &players[r].plays))
				sort(players.begin(), players.end(), sortname);

			ImGui::TableNextColumn();
			if (players[r].plays && ImGui::Button(to_string(players[r].score).c_str()) && (players[r].score == 0 || result == 0)) {
				players[r].score = result;
				list = !convert;
			} else if (!players[r].plays && players[r].score != 0) {
				ImGui::Text("%d", players[r].score);
			}
			if (players[r].plays && mode == 1) {
				ImGui::SameLine();
				ImGui::Text("%s%d/%d", string(max(0, 4 - (int)to_string(players[r].score).length()), ' ').c_str(), 4, 3);
			}

			for (int c = 0; c < 3; c++) {
				ImGui::TableNextColumn();
				ImGui::Text("200");
			}
			ImGui::PopID();
		}

		ImGui::EndTable();
	}
}

void Program::show_results() {
	if (!worker.joinable()) {
		toplist.clear();
		topscores.fill({0});
		worker = thread(&Program::process, this);
	} else if (!converting) {
		worker.join();
		topscores.fill({0});
		worker = thread(&Program::process, this);
	}

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {1.f, 1.f});
	if (ImGui::BeginTable("##2", 2 + nplayer, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY)) {
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("99  ").x);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("-999   ").x);

		lock_guard<std::mutex> lg(mut);
		auto& l = toplist;

		vector<int> won(nplayer, 0), lost(nplayer, 0);
		int sumw = 0, suml = 0;
		for (int r = 1; r < l.size(); r++) {
			if (l[r].points > 0 && l[r].player >= 0) {
				won[l[r].player]++;
				sumw++;
			} else if (l[r].points < 0 && l[r].player >= 0) {
				lost[l[r].player]++;
				suml++;
			}
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		ImGui::SetWindowFontScale(2.f);
		ImGui::Text("%d", sumw + suml);
		ImGui::SetWindowFontScale(1.f);
		ImGui::TableNextColumn();
		for (int i = 0; i < nplayer && l.size() > 0; i++) {
			auto& s = topscores[i];

			ImGui::PushID(i);
			ImGui::PushStyleColor(ImGuiCol_Button, {0.f, 0.5f, 0.5f, 1.f});
			ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.f, 0.5f, 0.5f, 1.f});
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);

			if (ImGui::BeginCombo("##1", to_string(s[0]).c_str(), ImGuiComboFlags_NoArrowButton)) {
				for (int  j = 1; j < s.size(); j++) {
					if (ImGui::Selectable(to_string(s[j]).c_str()))
						swap(s[0], s[j]);
				}
				ImGui::EndCombo();
			}

			ImGui::Text("%d/%d", won[i], lost[i]);
			ImGui::Text("%d", (won[i] - lost[i]) * 50);
			ImGui::Text("%d", (suml - lost[i]) * (nplayer == 3? 40: 30));
			int res = s[0] + (won[i] - lost[i]) * 50 + (suml - lost[i]) * (nplayer == 3? 40: 30);
			if (ImGui::Button((to_string(res) + "##2").c_str(), {ImGui::GetContentRegionAvail().x, 0})) {
				result = res;
				list = true;
			}
			ImGui::Text("");
			ImGui::PopStyleColor(2);
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

			for (int i = 0; i < nplayer; i++) {
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

	nplayer = fcol + 20 < cols && abs(ws[fcol + 17] - ws[fcol + 20]) < th? 4: 3;

	for (frow = 0; frow < rows; frow++) {
		int count = 1;
		while (frow + count < rows && abs(hs[frow] - hs[frow + count]) < th)
			count++;
		if (count > rows / 2)
			break;
	}

	float sum = 0.f;
	for (int c = fcol; c < cols && c < fcol + 9 + nplayer * 3; c++)
		sum += ws[c];

	return frow > 0 && frow < min(5, rows) && fcol + 9 + nplayer * 3 <= cols && sum > cam.w() * 0.75f;
}

void Program::read_field() {
	fields.str().clear();
	int n = fields.separate(3), o = Fields::wd / 2;
	NeuralNet::Values mem(n * chars.size(), 0.f), in(Fields::wd * Fields::hd), out;

	for (int i = 0; i < n; i++) {
		fields.select(0.f, -1, -1, i + 1);

		for (int j = 0; j < in.size(); j++)
			in[j] = pow(fields.data()[j] / 255.f, proc.faint_chars? 1.5f: 1.f);
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
				for (int p = 0; p < nplayer; p++) {
					if (nplayer == 4 && (i - frow) % nplayer == p)
						continue;
					best.insert({dists[l] + dist(games[g], true, lists[l][i].scores[p], name, tips, extra, f(9), f(11 + 3 * p)), {games[g].points, p, l}});
					best.insert({dists[l] + dist(games[g], false, lists[l][i].scores[p], name, tips, extra, f(10), f(11 + 3 * p)), {-2 * games[g].points, p, l}});
				}
			}
			best.insert({dists[l] + dist(Game{"", "", 0, 0}, true, 0, name, tips, extra, "", ""), {0, -1, l}});
			int sum = 0;
			for (int p = 0; p < nplayer; p++)
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
		topscores.fill({0});
		for (int r = 1; r < toplist.size(); r++)
			if (toplist[r].player >= 0)
				topscores[toplist[r].player][0] += toplist[r].points;

		if (lists.front().back().player == -2)
			break;
	}

	set<int> points;
	for (int g = 0; g < games.size(); g++)
		points.insert(games[g].points);

	for (int p = 0; p < nplayer; p++) {
		set<int> rs;
		for (int r = 1; r < lists[0].size(); r++) {
			if (lists[0][r].player == p && lists[0][r].points != 0 || lists[0][r].player == -2)
				rs.insert(r);
		}

		vector<int> ls{0}, ds{0};
		for (auto rit = rs.begin(); rit != rs.end(); rit++) {
			auto f = [&](int x) {fields.select(fcol + x, *rit - 1); return fields.str();};

			multimap<int, int> best;
			for (int l = 0; l < ls.size(); l++) {
				for (auto pit = points.begin(); pit != points.end(); pit++) {
					if (lists[0][*rit].points > 0)
						best.insert({ds[l] + dist(f(9), to_string(*pit)) + dist(f(11 + 3 * p), to_string(abs(ls[l] + *pit))), ls[l] + *pit});
					else if (lists[0][*rit].points < 0)
						best.insert({ds[l] + dist(f(10), to_string(*pit * 2)) + dist(f(11 + 3 * p), to_string(abs(ls[l] - *pit * 2))), ls[l] - *pit * 2});
				}
				if (lists[0][*rit].player == -2)
					best.insert({ds[l] + dist(f(11 + 3 * p), to_string(abs(ls[l]))), ls[l]});

				if (!converting)
					return;
			}

			ls.clear();
			ds.clear();
			set<int> rep;
			for (auto it = best.begin(); it != best.end() && ls.size() < 10000; it++) {
				if (rep.count(it->second) != 0)
					continue;
				rep.insert(it->second);

				ls.push_back(it->second);
				ds.push_back(it->first);

				if (!converting)
					return;
			}
		}

		lock_guard<std::mutex> lg(mut);
		for (int n = 0; n < ls.size() && n < 50; n++)
			topscores[p].push_back(ls[n]);
	}
}
