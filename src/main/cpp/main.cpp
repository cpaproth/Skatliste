//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#define CNFG_IMPLEMENTATION
#include "CNFG.h"

#include "NdkCam.h"

#include <string>
#include <iostream>

using namespace std;

class coutbuf : streambuf {
	streambuf* oldbuf;
	string buffer;
	int overflow(int c) {
		if (c != 8)
			buffer.append(1, (char)c);
		else if (!buffer.empty())
			buffer.pop_back();
		while (count(buffer.begin(), buffer.end(), '\n') > 30)
			buffer.erase(0, buffer.find('\n') + 1);
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

void HandleKey( int keycode, int bDown ) {
	if (bDown && keycode > 0)
		cout << (char)keycode;
	else if (bDown && keycode == -67)
		cout << '\b';
}
void HandleButton( int x, int y, int button, int bDown ) {
	cout << x << " " << y << " " << button << " " << bDown << endl;
}
void HandleMotion( int x, int y, int mask ) {
	cout << x << " " << y << " " << mask << endl;
}
void HandleDestroy() {}
void HandleResume() {}
void HandleSuspend() {}

int main(int, char**) {

	if (!AndroidHasPermissions("CAMERA"))
		AndroidRequestAppPermissions("CAMERA");

	coutbuf buf;
	NdkCam cam(480, 640, 0);

	CNFGSetupFullscreen("Skatliste", 0);

	bool capture = false;
	char text[50] = "öäüß";

	while (CNFGHandleInput()) {

		short w, h;
		CNFGGetDimensions(&w, &h);
		CNFGSetLineWidth(1);
		CNFGBGColor = 0x000080ff;
		CNFGClearFrame();

		unsigned tex = cam.get_rgba(CNFGTexImage);
		if (w * cam.h() < h * cam.w())
			CNFGBlitTex(tex, 0, 0, w, cam.h() * w / cam.w());
		else
			CNFGBlitTex(tex, 0, 0, cam.w() * h / cam.h(), h);
		CNFGDeleteTex(tex);

		CNFGColor(0xffffffff);
		CNFGPenX = 20;
		CNFGPenY = 20;
		CNFGDrawText(buf.c_str(), 4);

		ImGui::Text("Hello, world!");
		ImGui::InputText("Hallo", text, sizeof(text));
		if (ImGui::Button(capture? "Stop": "Start")) {
			if (capture)
				cam.stop();
			else
				cam.start();
			capture = !capture;
		}
		ImGui::TextUnformatted(buf.c_str());

		ImGui::ShowDemoWindow();

		CNFGSwapBuffers();
	}
}

