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
	if (cam.cap()) {
		using namespace placeholders;

		glBindTexture(GL_TEXTURE_2D, cap_tex);
		cam.get_rgba(bind(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA, _2, _3, 0, GL_RGBA, GL_UNSIGNED_BYTE, _1));

		auto s = ImGui::GetMainViewport()->Size;
		s = s.x * cam.h() < s.y * cam.w()? ImVec2(s.x, cam.h() * s.x / cam.w()): ImVec2(cam.w() * s.y / cam.h(), s.y);
		ImGui::GetBackgroundDrawList()->AddImage((void*)(intptr_t)cap_tex, {0, 0}, s);

		vector<float> res;
		if (!proc.result(res))
			cam.swap_lum(bind(&SkatListProc::scan, &proc, _1, _2, _3));


		if (ImGui::Begin("Scanne Skatliste")) {
			if (ImGui::Button("Stop"))
				cam.stop();
		}
		ImGui::End();

	} else if (ImGui::Button("Start")) {
		cam.start();
	}


}
