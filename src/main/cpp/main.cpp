//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#define CNFG_IMPLEMENTATION
#include "external/rawdraw_sf.h"

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
    if (x > 100 && x < 200 && y > 100 && y < 200)
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

    CNFGSetupFullscreen("Skatliste", 0);
    CNFGSetLineWidth(1);

    AndroidDisplayKeyboard(true);

    while (CNFGHandleInput()) {

        CNFGBGColor = 0x000080ff;
        CNFGClearFrame();

        short w, h;
        CNFGGetDimensions(&w, &h);

        unsigned tex = CNFGTexImage(cam.rgbadata(), 640, 480);
        CNFGBlitTex(tex, 0, 0, w, h);
        CNFGDeleteTex(tex);
        //CNFGBlitImage((uint32_t*)mem.data(), 100, 100, 640, 480);


        CNFGColor(0xffffffff);
        CNFGPenX = 20;
        CNFGPenY = 20;
        CNFGDrawText(buf.c_str(), 4);

        CNFGSwapBuffers();
    }
}

