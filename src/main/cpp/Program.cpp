//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include "Program.h"
#include "imgui.h"
#include <iostream>

using namespace std;

Program::Program() : cam(480, 640, 0) {
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
}

Program::~Program() {
	for (auto tex : {&cap_tex, &dig_tex})
		glDeleteTextures(1, tex);
}

void Program::draw() {

	static bool learn = false;
	ImGui::Begin("Skat List", learn? &learn: 0);
	if (!learn) {
		if (ImGui::Button(cam.cap()? "Stop": "Scan"))
			cam.cap()? cam.stop(): cam.start();
		ImGui::SameLine();
		if (!cam.cap() && fields.select()) {
			if (ImGui::Button("Learn"))
				learn = fields.first();
		} else {
			ImGui::Checkbox("Big", &proc.big_chars);
		}

		ImGui::SliderInt("Edge", &proc.edge_th, 1, 100);
		ImGui::SliderInt("Line", &proc.line_th, 1, 100);

		if (fields.select()) {
			glBindTexture(GL_TEXTURE_2D, dig_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, fields.W(), fields.H(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, fields.data());
			ImGui::Image((void*)(intptr_t)dig_tex, {fields.W() * 4.f, fields.H() * 4.f});
			ImGui::InputInt("FieldX", &fields.X);
			ImGui::InputInt("FieldY", &fields.Y);
			ImGui::InputInt("FieldD", &fields.D);
		}
	} else {
		if (fields.select()) {
			glBindTexture(GL_TEXTURE_2D, dig_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, fields.W(), fields.H(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, fields.data());
			ImGui::Image((void*)(intptr_t)dig_tex, {fields.W() * 10.f, fields.H() * 10.f});
		}
		ImGui::SameLine();
		ImGui::BeginGroup();
		if (ImGui::Button("Ignore Row"))
			learn = fields.ignore_row();
		if (ImGui::Button("Ignore Column"))
			learn = fields.ignore_col();
		if (ImGui::Button("Ignore"))
			learn = fields.next();
		ImGui::EndGroup();

		const char* chars[15] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "+", "x", "|", " "};
		int val = -1;
		for (int i = 0; i < 15; i++) {
			if (i % 5 != 0)
				ImGui::SameLine();
			if (ImGui::Button(chars[i], {70, 70}))
				val = i;
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

	if (proc.result(lines, fields))
		cam.stop();
	int pos = 0;
	for (auto& l : lines)
		ImGui::GetBackgroundDrawList()->AddLine({f * l.x.x, f * l.x.y}, {f * l.y.x, f * l.y.y}, fields.check(pos++)? 0xff00ff00: 0xff0000ff);

	if (!ImGui::GetIO().WantCaptureMouse && ImGui::GetIO().MouseReleased[0])
		cout << ImGui::GetIO().MousePos.x << endl;
}
