//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#define CNFG_IMPLEMENTATION
#include "CNFG.h"

#include "NdkCam.h"
#include "SkatListProc.h"

#include <string>
#include <iostream>

using namespace std;

class coutbuf : streambuf {
	streambuf* oldbuf;
	string buffer;
	int overflow(int c) {
		buffer.append(1, (char)c);
		return c;
	}
public:
	coutbuf() {
		oldbuf = cout.rdbuf(this);
	}
	~coutbuf() {
		cout.rdbuf(oldbuf);
	}
	const char* c_str() {
		return buffer.c_str();
	}
};

NdkCam* cam = 0;
void HandleKey(int, int) {}
void HandleButton(int, int, int, int) {}
void HandleMotion(int, int, int) {}
void HandleDestroy() {}
void HandleResume() {
	if (!cam)
		cam = new NdkCam(480, 640, 0);
}
void HandleSuspend() {
	delete cam;
	cam = 0;
}

int main(int, char**) {

	if (!AndroidHasPermissions("CAMERA"))
		AndroidRequestAppPermissions("CAMERA");

	coutbuf buf;
	SkatListProc proc;

	CNFGSetupFullscreen("Skatliste", 0);

	char text[50] = "öäüß";

	while (CNFGHandleInput()) {

		short w, h;
		CNFGGetDimensions(&w, &h);

		CNFGBGColor = 0x000080ff;
		CNFGClearFrame();

		if (cam && cam->cap()) {
			unsigned tex = cam->get_rgba(CNFGTexImage);
			if (w * cam->h() < h * cam->w())
				CNFGBlitTex(tex, 0, 0, w, cam->h() * w / cam->w());
			else
				CNFGBlitTex(tex, 0, 0, cam->w() * h / cam->h(), h);
			CNFGDeleteTex(tex);
		}

		ImGui::Text("Hello, world!");
		ImGui::InputText("Hallo", text, sizeof(text));
		if (cam && ImGui::Button(cam->cap()? "Stop": "Start")) {
			if (cam->cap())
				cam->stop();
			else
				cam->start();
		}
		ImGui::TextUnformatted(buf.c_str());
		ImGui::ShowDemoWindow();

		CNFGSwapBuffers();
	}
}

