//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include "Program.h"
#include "imgui.h"

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

	if (ImGui::Begin("Skatliste")) {
		if (ImGui::Button(cam.cap()? "Stop": "Scan"))
			cam.cap()? cam.stop(): cam.start();
		ImGui::SameLine();
		if (ImGui::Button("Lerne")) {

		}
		ImGui::SliderInt("Edge", &proc.edge_th, 1, 100);
		ImGui::SliderInt("Line", &proc.line_th, 1, 100);

		if (!fields.empty()) {
			static int f = 0;
			f = max(0, min(f, (int)fields.size() - 1));
			int width = int(fields[f].size()) / ListProc::dig_h, height = ListProc::dig_h;
			glBindTexture(GL_TEXTURE_2D, dig_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, fields[f].data());
			ImGui::Image((void*)(intptr_t)dig_tex, {width * 4.f, height * 4.f});
			ImGui::SliderInt("Field1", &f, 0, (int)fields.size() - 1);
			ImGui::InputInt("Field2", &f);
		}
	}
	ImGui::End();


	using namespace placeholders;

	glBindTexture(GL_TEXTURE_2D, cap_tex);
	cam.get_rgba(bind(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA, _2, _3, 0, GL_RGBA, GL_UNSIGNED_BYTE, _1));

	auto s = ImGui::GetMainViewport()->Size;
	float w = float(cam.w()), h = float(cam.h()), f = s.x * h < s.y * w? s.x / w: s.y / h;
	ImGui::GetBackgroundDrawList()->AddImage((void*)(intptr_t)cap_tex, {0.f, 0.f}, {f * w, f * h});

	if (cam.cap())
		cam.swap_lum(bind(&ListProc::scan, &proc, _1, _2, _3));

	if (proc.result(lines, fields))
		cam.stop();
	for (auto& l : lines)
		ImGui::GetBackgroundDrawList()->AddLine({f * l.x.x, f * l.x.y}, {f * l.y.x, f * l.y.y}, 0xff0000ff);

}
