//Copyright (c) 2023 Carsten Paproth - Licensed under MIT License

#define CNFG_IMPLEMENTATION
#include "external/rawdraw_sf.h"

#include <string>
#include <sstream>
#include <vector>
#include <iostream>

using namespace std;
string text;

class mybuf : streambuf {
    mybuf() {
        cout.rdbuf(this);
    }
    virtual int_type overflow(int_type c) override {
        text.append(1, (char)c);
        return c;
    }
};
//mybuf buf();

#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
//#include <camera/NdkCameraCaptureSession.h>
//#include <camera/NdkCameraMetadata.h>
#include <media/NdkImageReader.h>


vector<uint8_t> mem(640 * 480 * 4);

void captureImage() {
    ACameraManager* cameraManager = ACameraManager_create();
    ACameraIdList* cameraIdList = nullptr;
    ACameraManager_getCameraIdList(cameraManager, &cameraIdList);

    ACameraDevice* cameraDevice = nullptr;
    ACameraDevice_stateCallbacks cameraDeviceCallbacks{0, [](void*, ACameraDevice*){}, [](void*, ACameraDevice*, int) {}};
    ACameraManager_openCamera(cameraManager, cameraIdList->cameraIds[0], &cameraDeviceCallbacks, &cameraDevice);

    AImageReader* imageReader = nullptr;
    AImageReader_new(640, 480, AIMAGE_FORMAT_RGBA_8888, 1, &imageReader);

    AImageReader_ImageListener listener{
            .context = nullptr,
            .onImageAvailable = [](void* context, AImageReader* reader) {
                AImage *image = nullptr;
                AImageReader_acquireLatestImage(reader, &image);
                uint8_t *data = nullptr;
                int len = 0;
                AImage_getPlaneData(image, 0, &data, &len);

                for (unsigned i = 0; i < mem.size() && i < len; i += 4) {
                    mem[i + 0] = data[i + 3];
                    mem[i + 1] = data[i + 2];
                    mem[i + 2] = data[i + 1];
                    mem[i + 3] = data[i + 0];
                }

                stringstream str;
                str<<len;
                //text="huch " + str.str();

                AImage_delete(image);
            }
    };
    AImageReader_setImageListener(imageReader, &listener);

    ANativeWindow* window = nullptr;
    AImageReader_getWindow(imageReader, &window);

    ACameraCaptureSession_stateCallbacks sessionStateCallbacks{
            .context = nullptr,
            .onClosed = [](void*, ACameraCaptureSession*) {},
            .onReady = [](void*, ACameraCaptureSession*) {},
            .onActive = [](void*, ACameraCaptureSession*) {}
    };

    ACaptureSessionOutput* output = nullptr;
    ACaptureSessionOutputContainer* outputs = nullptr;
    ACameraCaptureSession* session = nullptr;
    ACaptureSessionOutput_create(window, &output);
    ACaptureSessionOutputContainer_create(&outputs);
    ACaptureSessionOutputContainer_add(outputs, output);
    ACameraDevice_createCaptureSession(cameraDevice, outputs, &sessionStateCallbacks, &session);

    ACaptureRequest* request = nullptr;
    ACameraOutputTarget* jpegTarget = nullptr;
    ACameraDevice_createCaptureRequest(cameraDevice, TEMPLATE_PREVIEW, &request);

    ANativeWindow_acquire(window);
    ACameraOutputTarget_create(window, &jpegTarget);
    ACaptureRequest_addTarget(request, jpegTarget);

    ACameraCaptureSession_captureCallbacks captureCallbacks {
            .context = nullptr,
            .onCaptureStarted = nullptr,
            .onCaptureProgressed = nullptr,
            .onCaptureCompleted = 0,
            .onCaptureFailed = 0,
            .onCaptureSequenceCompleted = 0,
            .onCaptureSequenceAborted = 0,
            .onCaptureBufferLost = nullptr,
    };
    ACameraCaptureSession_setRepeatingRequest(session, &captureCallbacks, 1, &request, nullptr);



    // Create a capture session and capture the image
    //ACameraCaptureSession* captureSession = nullptr;
    //ACameraDevice_createCaptureSession(cameraDevice, window, &captureSession);
    //ACameraCaptureSession_capture(captureSession, nullptr, 1, 0);

    // Cleanup
    //ACameraCaptureSession_close(captureSession);

    //AImageReader_delete(imageReader);
    //ACameraDevice_close(cameraDevice);
    //ACameraManager_deleteCameraIdList(cameraIdList);
    //ACameraManager_delete(cameraManager);
}



void HandleKey( int keycode, int bDown ) {
    if (bDown && keycode > 0)
        //cout << (char)keycode;
        text.push_back(keycode);
    else if (bDown && keycode == -67 && !text.empty())
        text.pop_back();
}
void HandleButton( int x, int y, int button, int bDown ) {
    stringstream str;
    str << x << " " << y << " " << button << " " << bDown;
    text = str.str();
    if (x > 100 && x < 200 && y > 100 && y < 200)
        AndroidDisplayKeyboard(!bDown);
}
void HandleMotion( int x, int y, int mask ) {
    stringstream str;
    str << x << " " << y << " " << mask;
    text = str.str();
}
void HandleDestroy() {}
void HandleResume() {}
void HandleSuspend() {}
void HandleWindowTermination() {}

int main(int, char**) {
    if (!AndroidHasPermissions("CAMERA"))
        AndroidRequestAppPermissions("CAMERA");

    captureImage();

    CNFGSetupFullscreen("Skatliste", 0);
    AndroidDisplayKeyboard(true);


    int count = 0;
    while(CNFGHandleInput())
    {

        CNFGBGColor = 0x000080ff; //Dark Blue Background

        short w, h;
        CNFGClearFrame();
        CNFGGetDimensions( &w, &h );

        //Change color to white.
        CNFGColor( 0xffffffff );

        CNFGSetLineWidth(count++ % 10 + 10);
        CNFGPenX = 100;
        CNFGPenY = 1000;
        CNFGDrawText("Hello, World", 20);
        CNFGPenY = 1100;
        CNFGDrawText(text.c_str(), 20);
        //Draw a white pixel at 30, 30
        CNFGTackPixel( 30, 30 );

        //Draw a line from 50,50 to 100,50
        CNFGTackSegment( 50, 50, 100, 50 );

        //Dark Red Color Select
        CNFGColor( 0x800000FF );

        //Draw 50x50 box starting at 100,50
        CNFGTackRectangle( 100, 50, 150, 100 );

        //Bright Purple Select
        CNFGColor( 0x800000FF );

        //Draw a triangle
        RDPoint points[3] = { { 30, 36}, {20, 50}, { 40, 50 } };
        CNFGTackPoly( points, 3 );

        //Draw a bunch of random pixels as a blitted image.
        {
            static uint32_t data[640 * 640];
            int x, y;

            for (y = 0; y < 640; y++)
                for (x = 0; x < 640; x++)
                    data[x + y * 640] = 0xff | (rand() << 8);
            CNFGBlitImage(data, 150, 30, 640, 640);

            CNFGBlitImage((uint32_t*)mem.data(), 150, 1500, 640, 480);
        }
        //Display the image and wait for time to display next frame.
        CNFGSwapBuffers();
    }
}

