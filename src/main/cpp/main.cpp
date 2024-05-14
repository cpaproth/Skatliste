//Copyright (C) 2023, 2024 Carsten Paproth - Licensed under MIT License

#define CNFG_IMPLEMENTATION
#include "CNFG.h"
#include "Program.h"
#include <string>
#include <iostream>
#include <fstream>

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
		program = new Program(gapp->activity->externalDataPath);
}
void HandleSuspend() {
	delete program;
	program = 0;
}

int main(int, char**) {
	coutbuf buf;

	if (!AndroidHasPermissions("CAMERA"))
		AndroidRequestAppPermissions("CAMERA");

	AAssetDir* dir = AAssetManager_openDir(gapp->activity->assetManager, "");
	for (const char* name = AAssetDir_getNextFileName(dir); name != 0; name = AAssetDir_getNextFileName(dir)) {
		AAsset* asset = AAssetManager_open(gapp->activity->assetManager, name, AASSET_MODE_STREAMING);
		char mem[BUFSIZ];
		ofstream file(gapp->activity->externalDataPath + string("/") + name, ofstream::binary);
		for (int n = AAsset_read(asset, mem, BUFSIZ); n > 0; n = AAsset_read(asset, mem, BUFSIZ))
			file.write(mem, n);
		AAsset_close(asset);
	}
	AAssetDir_close(dir);

	CNFGSetupFullscreen("", 0);
	HandleResume();

	while (CNFGHandleInput()) {
		CNFGBGColor = 0x000080ff;
		CNFGClearFrame();

		if (program)
			program->draw();

		static bool demo = false;
		ImGui::Checkbox("GUI Demo", &demo);
		ImGui::TextUnformatted(buf.c_str());
		if (demo)
			ImGui::ShowDemoWindow(&demo);

		CNFGSwapBuffers();
	}
}

