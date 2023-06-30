//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#define CNFG_IMPLEMENTATION
#include "CNFG.h"
#include "Program.h"

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

Program* program = 0;
void HandleKey(int, int) {}
void HandleButton(int, int, int, int) {}
void HandleMotion(int, int, int) {}
void HandleDestroy() {}
void HandleResume() {
	if (!program && ImGui::GetCurrentContext())
		program = new Program();
}
void HandleSuspend() {
	delete program;
	program = 0;
}

int main(int, char**) {
	coutbuf buf;

	if (!AndroidHasPermissions("CAMERA"))
		AndroidRequestAppPermissions("CAMERA");

	CNFGSetupFullscreen("", 0);
	HandleResume();

	while (CNFGHandleInput()) {
		CNFGBGColor = 0x000080ff;
		CNFGClearFrame();

		if (program)
			program->draw();

		static char text[50] = "öäüß";
		static bool showDemo = false;
		ImGui::InputText("Test", text, sizeof(text));
		ImGui::Checkbox("Demo", &showDemo);
		ImGui::TextUnformatted(buf.c_str());
		if (showDemo)
			ImGui::ShowDemoWindow(&showDemo);

		CNFGSwapBuffers();
	}
}

