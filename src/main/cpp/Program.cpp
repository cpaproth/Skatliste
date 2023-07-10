//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include "Program.h"
#include "imgui.h"

using namespace std;

Program::Program() : cam(480, 640, 0) {
	glEnable(GL_TEXTURE_2D);
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

	if (ImGui::Begin("Skatliste")) {
		if (ImGui::Button(cam.cap()? "Stop": "Start"))
			cam.cap()? cam.stop(): cam.start();
		ImGui::SameLine();
		if (ImGui::Button("Lerne")) {

		}
		ImGui::SliderInt("Schwellwert", &proc.thfeat, 1, 100);
	}
	ImGui::End();


	if (!cam.cap())
		return;

	using namespace placeholders;

	glBindTexture(GL_TEXTURE_2D, cap_tex);
	cam.get_rgba(bind(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA, _2, _3, 0, GL_RGBA, GL_UNSIGNED_BYTE, _1));

	auto s = ImGui::GetMainViewport()->Size;
	float f = s.x * cam.h() < s.y * cam.w()? s.x / cam.w(): s.y / cam.h();
	ImGui::GetBackgroundDrawList()->AddImage((void*)(intptr_t)cap_tex, {0.f, 0.f}, {f * cam.w(), f * cam.h()});

	cam.swap_lum(bind(&ListProc::scan, &proc, _1, _2, _3));

	proc.result(lines);
	for (auto l : lines)
		ImGui::GetBackgroundDrawList()->AddLine({f * l[0], f * l[1]}, {f * l[2], f * l[3]}, 0xff0000ff);

}
