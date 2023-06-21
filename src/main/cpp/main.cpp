//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_tables.cpp"
#include "imgui_widgets.cpp"
#include "imgui_impl_android.cpp"
#include "imgui_impl_opengl3.cpp"


#define CNFG_IMPLEMENTATION
#include "rawdraw_sf.h"

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
    if (x < 200 && y < 200)
        AndroidDisplayKeyboard(!bDown);
}
void HandleMotion( int x, int y, int mask ) {
    cout << x << " " << y << " " << mask << endl;
}
void HandleDestroy() {}
void HandleResume() {}
void HandleSuspend() {}
void HandleWindowTermination() {}

int main(int, char**) {

    if (!AndroidHasPermissions("CAMERA"))
        AndroidRequestAppPermissions("CAMERA");

    coutbuf buf;
    NdkCam cam(480, 640, 0);
    cam.start();

    CNFGSetupFullscreen("Skatliste", 0);
    CNFGSetLineWidth(1);

    AndroidDisplayKeyboard(true);

    ImGui::CreateContext();
    ImGui_ImplAndroid_Init(native_window);
    ImGui_ImplOpenGL3_Init("#version 300 es");
    ImFontConfig font_cfg;
    font_cfg.SizePixels = 50.0f;
    ImGui::GetIO().Fonts->AddFontDefault(&font_cfg);

    while (CNFGHandleInput()) {

        CNFGBGColor = 0x000080ff;
        CNFGClearFrame();

        short w, h;
        CNFGGetDimensions(&w, &h);

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
        CNFGFlushRender();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplAndroid_NewFrame();
        ImGui::NewFrame();
        //ImGui::Text("Hello, world!");
        ImGui::TextEx(buf.c_str());
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        CNFGSwapBuffers();
    }
}

