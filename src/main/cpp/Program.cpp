//Copyright (C) 2023, 2024 Carsten Paproth - Licensed under MIT License

#include "Program.h"
#include "imgui.h"
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

void Players::load(const string& filename) {
	ps.clear();
	clear();

	ifstream file(filename);
	string line;
	bool round = false;
	while (getline(file, line)) {
		vector<string> vals;
		stringstream str(line);
		while (getline(str >> ws, line, ';'))
			vals.push_back(line);

		if (ps.size() == 0 && vals.size() > 1 && (vals[1] == "Runde" || vals[1].substr(0, 6) == "Gesamt")) {
			round = vals[1] == "Runde";
			if (vals.size() > (round? 3: 1))
				prize = atof(vals[round? 3: 1].substr(6).c_str());
			if (vals.size() > (round? 4: 2))
				remove = atoi(vals[round? 4: 2].substr(5).c_str());
			for (int d = round? 5: 3; d < vals.size(); d++)
				dates.push_back(vals[d]);
			continue;
		}

		if (vals.size() > 0)
			ps.push_back({vals[0], false, 0, 0, {}});
		if (round && vals.size() > 1 && (ps.back().plays = !vals[1].empty()))
			ps.back().score = atoi(vals[1].c_str());
		if (round && vals.size() > 2)
			ps.back().result = atoi(vals[2].c_str());
		for (int s = round? 5: 3; s < vals.size(); s++)
			ps.back().scores.push_back(atoi(vals[s].c_str()));
	}
}

void Players::save(const string& filename) {
	ofstream file(filename);

	file << "Name";
	if (!noround())
		file << ";Runde;Summe";
	file << ";Gesamt " << prize << ";Abzug " << remove;
	for (const auto& d : dates)
		file << ";" << d;
	file << "\n";

	for (int p = 0; p < ps.size(); p++) {
		file << name(p);
		if (!noround())
			file << ";" << (plays(p) || score(p)? to_string(score(p)): "") << ";" << (ps[p].result? to_string(ps[p].result): "");
		file << ";" << (total(p)? to_string(total(p)): "") << ";" << (removed(p)? to_string(removed(p)): "");
		for (const auto& s : ps[p].scores)
			file << ";" << (s? to_string(s): "");
		file << "\n";
	}
}

float Players::prize_round(int i) {
	int n = num(), przs = (n - 1) / max(1, bpprize) + 1;
	float e = 0.5f, m = 0.5f, o = 0.5f, eps = 1.f + FLT_EPSILON;
	auto f = [&](int p, int t, float (*r)(float)) {float s = 0.f; while (p <= t) s += r(0.5f + o + (m * n - 1.f) * pow((float)p++ / przs, 1.f / e)); return s;};
	for (float mi = 0.f, ma = 1.f, s; (s = f(1, przs, fabs)) != n && ma / mi > eps; m = (mi + ma) / 2.f)
		(s > n? ma: mi) = m;
	for (float mi = 0.f, ma = 1.f, s; (s = f(1, przs, round)) != n && ma / mi > eps; o = (mi + ma) / 2.f)
		(s > n? ma: mi) = o;
	return i < przs? f(przs - i, przs - i, round) * bet: 0.f;
}

float Players::prize_season(int i) {
	float s = 0.f, c = 0.f;
	for (int p = min(prizes, count()) - 1; p >= i; p--)
		s += c = floor((prize - s) / (prizes - p / 2.f) / (p + 1.f) * (prizes - p) * 2.f) / 2.f;
	return i == 0? c + prize - s: c;
}

const vector<const char*> Program::chars{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "+", "-"};
const map<string, string> Program::def_cfg{{"csv", "players.csv"}, {"three", "0"}, {"scale", "1.0"}, {"bet", "10.0"}, {"prizes", "3"}, {"bpprize", "4"}};

Program::Program(const string& p) : path(p), cam(720, 960, 0), clss(p, Fields::wd, Fields::hd, chars.size()), converting(false) {
	glEnable(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (const auto& tex : {&cap_tex, &dig_tex}) {
		glGenTextures(1, tex);
		glBindTexture(GL_TEXTURE_2D, *tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	ifstream file(path + "/settings.txt");
	string key;
	while (getline(file >> ws, key, '='))
		getline(file >> ws, cfg[key]);
	players.load(path + "/" + cfg["csv"]);
	cout << "players: " << players.count() << endl;

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
	for (const auto& tex : {&cap_tex, &dig_tex})
		glDeleteTextures(1, tex);
	ofstream file(path + "/settings.txt");
	for (const auto& c : def_cfg)
		file << c.first << "=" << cfg[c.first] << "\n";
	players.save(path + "/" + cfg["csv"]);
}

void Program::draw() {
	auto s = ImGui::GetMainViewport()->Size, p = ImGui::GetMainViewport()->Pos;
	float w = float(cam.w()), h = float(cam.h()), f = s.x * h < s.y * w? s.x / w: s.y / h, sc = ImGui::GetFontSize() / 30.f;

	if (convert || list) {
		ImGui::SetNextWindowPos(p);
		ImGui::SetNextWindowSize(s);
	} else {
		ImGui::SetNextWindowPos({p.x, p.y + min(f * h, 0.7f * s.y)});
		ImGui::SetNextWindowSize({s.x, s.y - min(f * h, 0.7f * s.y)});
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
		show_config(sc);
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
		ImGui::Image((void*)(intptr_t)dig_tex, {fields.W() * 10.f * sc, fields.H() * 10.f * sc});

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
			if (ImGui::Button(chars[i], {70.f * sc, 70.f * sc}))
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

	ImGui::GetBackgroundDrawList()->AddImage((void*)(intptr_t)cap_tex, {0.f, 0.f}, {f * w, f * h});

	if (cam.cap())
		cam.get_lum(bind(&ListProc::scan, &proc, _1, _2, _3));

	if (proc.result(lines, fields) && check_lines()) {
		cam.stop();
		convert = true;
	}

	int pos = 0;
	for (const auto& l : lines)
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

void Program::show_config(float sc) {
	ImGui::GetIO().FontGlobalScale = clamp(atof(cfg["scale"].c_str()), 0.5, 2.);
	players.bet = max(atof(cfg["bet"].c_str()), 1.);
	players.bpprize = max(atoi(cfg["bpprize"].c_str()), 2);
	players.prizes = max(atoi(cfg["prizes"].c_str()), 1);
	players.three = atoi(cfg["three"].c_str());

	static float width = ImGui::GetContentRegionAvail().x / 2.f;
	ImGui::BeginChild("##1", {width, 0.f}, true);
	ImGui::Checkbox("Big", &proc.big_chars);
	ImGui::SameLine();
	ImGui::Checkbox("Faint", &proc.faint_chars);
	ImGui::SameLine();
	bool test = !proc.test_img.empty();
	if (ImGui::Checkbox("Test", &test))
		proc.test_img = test? path + "/test.720.ubyte": "";
	ImGui::SliderInt("Edge", &proc.edge_th, 1, 100);
	ImGui::SliderInt("Line", &proc.line_th, 1, 100);

	if (fields.select()) {
		glBindTexture(GL_TEXTURE_2D, dig_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, fields.W(), fields.H(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, fields.data());
		ImGui::Image((void*)(intptr_t)dig_tex, {fields.W() * 4.f * sc, fields.H() * 4.f * sc});
		ImGui::Text("%s", fields.str().c_str());
		ImGui::InputInt("X", &fields.X);
		ImGui::InputInt("Y", &fields.Y);
		ImGui::InputInt("D", &fields.D);
	}
	ImGui::EndChild();

	ImGui::SameLine(0.f, 0.f);
	ImGui::InvisibleButton("##2", {40.f * sc, ImGui::GetContentRegionAvail().y});
	if (ImGui::IsItemActive())
		width = clamp(width + ImGui::GetIO().MouseDelta.x, 1.f, ImGui::GetContentRegionAvail().x - 1.f - 40.f * sc);
	ImGui::SameLine(0.f, 0.f);

	ImGui::BeginChild("##3", {0.f, 0.f}, true);
	ImGui::DragFloat("Scale", &ImGui::GetIO().FontGlobalScale, 0.01f, 0.5f, 2.f, "%.2f");
	if (sc != ImGui::GetIO().FontGlobalScale)
		(ImGui::GetStyle() = ImGuiStyle()).ScaleAllSizes(3.f * ImGui::GetIO().FontGlobalScale);
	ImGui::InputFloat("EUR/Bet", &players.bet, 0.f, 0.f, "%.2f");
	ImGui::InputInt("Bets/Prize", &players.bpprize);
	ImGui::InputInt("Prizes/Season", &players.prizes);
	ImGui::Checkbox("Prefer 3-Tables", &players.three);
	ImGui::Text("Players: %d / %.2f EUR", players.num(), players.num() * players.bet);
	ImGui::Text("4-Tables: %d", players.tables().first);
	ImGui::Text("3-Tables: %d", players.tables().second);
	ImGui::EndChild();

	cfg["scale"] = to_string(ImGui::GetIO().FontGlobalScale);
	cfg["bet"] = to_string(players.bet);
	cfg["bpprize"] = to_string(players.bpprize);
	cfg["prizes"] = to_string(players.prizes);
	cfg["three"] = to_string(players.three);
}

void Program::show_players() {
	static char csv[30] = "";
	static char name[20] = "";
	static char date[11] = "";
	static float money = 0.f;
	static bool show = true;

	ImGui::PushItemWidth(ImGui::CalcTextSize("longfilename.csv").x);
	csv[cfg["csv"].copy(csv, sizeof(csv) - 1)] = 0;
	ImGui::InputText("##0", csv, sizeof(csv));
	cfg["csv"] = csv;
	ImGui::PopItemWidth();
	ImGui::SameLine();
	if (ImGui::Button("Load"))
		players.load(path + "/" + cfg["csv"]);
	ImGui::SameLine();
	if (ImGui::Button("Save"))
		players.save(path + "/" + cfg["csv"]);
	ImGui::SameLine();
	if (ImGui::Button("Download"))
		players.save("/storage/emulated/0/Download/" + cfg["csv"]);

	if (ImGui::BeginTable("##1", 5 + players.rounds(), ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY)) {
		ImGui::TableSetupScrollFreeze(1, 1);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, show? 0.f: ImGui::CalcTextSize("99. Www").x);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("-999   ---+++").x);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("00.00.0000  ").x);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("99999 999.99").x);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("99999  ").x);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##1", name, sizeof(name));
		if (ImGui::ArrowButton("", show? ImGuiDir_Left: ImGuiDir_Right))
			show = !show;
		ImGui::SameLine();
		if (ImGui::Button(name[0] == 0? "Sort##1": !players.find(name)? "Add##1": "Del##1")) {
			if (!players.find(name) && name[0] != 0)
				players.add_name(name);
			else if (name[0] != 0)
				players.del(name);
			players.sort_name();
			name[0] = 0;
		}

		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		ImGui::InputInt("##2", &result);
		if (ImGui::Button("Sort##2"))
			players.sort_score();
		if (players.filled()) {
			ImGui::SameLine();
			if (ImGui::Button("Add##2"))
				players.add_score();
		}

		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##3", date, sizeof(date));
		if (ImGui::Button(date[0] == 0? "Sort##3": date[0] == 'X'? "Del##3": "Add##3")) {
			if (date[0] == 0) {
				players.sort_sum();
			} else if (date[0] == 'X') {
				players.clear();
				players.sort_name();
			} else {
				players.add_sum(date);
				players.sort_total();
			}
			date[0] = 0;
		}

		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		ImGui::InputFloat("##4", &money, 0.f, 0.f, "%.2f");
		if (ImGui::Button(money == 0.f? "Sort##4": "Add##4")) {
			if (money == 0.f)
				players.sort_total();
			else
				players.prize += money;
			money = 0.f;
		}
		ImGui::Text("%.2f", players.prize);

		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		if (ImGui::SliderInt("##5", &players.remove, 0, players.rounds()))
			players.sort_total();

		for (int d = 0; d < players.rounds(); d++) {
			ImGui::TableNextColumn();
			ImGui::Text("%s", players.date(d).c_str());
		}

		for (int r = 0; r < players.count(); r++) {
			ImGui::PushID(r);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			if (ImGui::Selectable((to_string(r + 1) + ". " + players.name(r)).c_str(), &players.plays(r)))
				players.sort_name();

			ImGui::TableNextColumn();
			if (players.plays(r) && ImGui::Button(to_string(players.score(r)).c_str(), {ImGui::CalcTextSize(" 9999 ").x, 0}) && (players.score(r) == 0 || result == 0)) {
				players.score(r) = result;
				list = !convert;
			} else if (!players.plays(r) && players.score(r) != 0) {
				ImGui::Text("%4d", players.score(r));
			}
			if (players.plays(r) && players.sorted() == 1) {
				ImGui::SameLine();
				ImGui::Text("%d/%d", players.table(r), players.seat(r));
			}

			ImGui::TableNextColumn();
			if (players.sum(r) != 0)
				ImGui::Text("%4d", players.sum(r));
			if (players.sorted() == 2 && players.prize_round(r) != 0) {
				if (players.sum(r) != 0)
					ImGui::SameLine();
				ImGui::Text("%6.2f", players.prize_round(r));
			}

			ImGui::TableNextColumn();
			if (players.total(r) != 0)
				ImGui::Text("%5d", players.total(r));
			if (players.sorted() == 3 && players.prize_season(r) != 0) {
				if (players.total(r) != 0)
					ImGui::SameLine();
				ImGui::Text("%6.2f", players.prize_season(r));
			}

			ImGui::TableNextColumn();
			if (players.removed(r) != 0)
				ImGui::Text("%4d", players.removed(r));

			for (int d = 0; d < players.rounds(); d++) {
				ImGui::TableNextColumn();
				if (players.score(r, d) != 0)
					ImGui::Text("%4d", players.score(r, d));
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
				if (l[r].scores[i] != l[r - 1].scores[i] || l[r].points == 0 && l[r].player == i)
					res += 2 * ImGui::InputInt("", &l[r].scores[i], 0);
				else if (ImGui::Button("-", {ImGui::GetContentRegionAvail().x, 0}))
					res += (l[r].player = i, l[r].points != 0? 1: 2);
				ImGui::PopID();
			}

			if (res == 1 && l[r].points == 0)
				l[r].player = -1;
			if (res == 1) {
				l[r].scores = l[r - 1].scores;
				if (l[r].player >= 0)
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
	float th = 5.f;

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
			in[j] = fields.data()[j] / 255.f;
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
		for (int r = 1; r < toplist.size(); r++) {
			if (toplist[r].player >= 0)
				topscores[toplist[r].player][0] += toplist[r].points;
			for (int p = 0; p < nplayer && toplist[r].points == 0; p++)
				topscores[p][0] += toplist[r].scores[p] - toplist[r - 1].scores[p];
		}

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
