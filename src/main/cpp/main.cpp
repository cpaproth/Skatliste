//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#define CNFG_IMPLEMENTATION
#include "external/rawdraw_sf.h"

#include <string>
#include <vector>
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

#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
#include <media/NdkImageReader.h>


void printCamProps(ACameraManager *cameraManager, const char *id) {
    // exposure range
    ACameraMetadata *metadataObj;
    ACameraManager_getCameraCharacteristics(cameraManager, id, &metadataObj);

    ACameraMetadata_const_entry entry = {0};
    ACameraMetadata_getConstEntry(metadataObj, ACAMERA_SENSOR_INFO_EXPOSURE_TIME_RANGE, &entry);

    int64_t minExposure = entry.data.i64[0];
    int64_t maxExposure = entry.data.i64[1];
    cout << "exposure: "<<minExposure<<" - "<< maxExposure<<endl;
    ////////////////////////////////////////////////////////////////

    // sensitivity
    ACameraMetadata_getConstEntry(metadataObj,
                                  ACAMERA_SENSOR_INFO_SENSITIVITY_RANGE, &entry);

    int32_t minSensitivity = entry.data.i32[0];
    int32_t maxSensitivity = entry.data.i32[1];

    cout<<"sensitivity: "<<minSensitivity<<" - "<<maxSensitivity<<endl;
    ////////////////////////////////////////////////////////////////

    // JPEG format
    ACameraMetadata_getConstEntry(metadataObj,
                                  ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry);

    for (int i = 0; i < entry.count; i += 4)
    {
        // We are only interested in output streams, so skip input stream
        int32_t input = entry.data.i32[i + 3];
        if (input)
            continue;

        int32_t format = entry.data.i32[i + 0];
        if (format == AIMAGE_FORMAT_YUV_420_888)
        {
            int32_t width = entry.data.i32[i + 1];
            int32_t height = entry.data.i32[i + 2];
            cout<<"maxsize="<<width<<" x "<<height<<endl;
        }
    }

    // cam facing
    ACameraMetadata_getConstEntry(metadataObj,
                                  ACAMERA_SENSOR_ORIENTATION, &entry);

    int32_t orientation = entry.data.i32[0];
    cout<<"orientation: "<<orientation<<endl;

    ACameraMetadata_getConstEntry(metadataObj, ACAMERA_LENS_FACING, &entry);
    cout<<"backfacing: "<<(int)entry.data.u8[0]<<endl;

}



vector<uint8_t> mem(640 * 480 * 4);
void captureImage() {
    ACameraManager* cameraManager = ACameraManager_create();
    ACameraIdList* cameraIdList = nullptr;
    ACameraManager_getCameraIdList(cameraManager, &cameraIdList);

    printCamProps(cameraManager, cameraIdList->cameraIds[0]);

    ACameraDevice* cameraDevice = nullptr;
    ACameraDevice_stateCallbacks cameraDeviceCallbacks{0, [](void*, ACameraDevice*){}, [](void*, ACameraDevice*, int) {}};
    ACameraManager_openCamera(cameraManager, cameraIdList->cameraIds[0], &cameraDeviceCallbacks, &cameraDevice);

    AImageReader* imageReader = nullptr;
    AImageReader_new(640, 480, AIMAGE_FORMAT_YUV_420_888, 1, &imageReader);

    AImageReader_ImageListener listener{
            .context = nullptr,
            .onImageAvailable = [](void* context, AImageReader* reader) {
                AImage *image = nullptr;
                AImageReader_acquireLatestImage(reader, &image);
                uint8_t *data = nullptr;
                int len = 0;
                AImage_getPlaneData(image, 0, &data, &len);

                int32_t w,h;
                AImage_getWidth(image, &w);
                AImage_getHeight(image, &h);
                //cout<<w<<" x "<<h<<endl;

                for (unsigned i = 0; i < 640 * 480 && i < len; i++) {
                    mem[4 * i] = 255;
                    mem[4 * i + 1] = data[i];
                    mem[4 * i + 2] = data[i];
                    mem[4 * i + 3] = data[i];
                }

                AImage_delete(image);
            }
    };
    AImageReader_setImageListener(imageReader, &listener);

    ANativeWindow* window = nullptr;
    AImageReader_getWindow(imageReader, &window);
    ANativeWindow_acquire(window);

    ACaptureRequest* request = nullptr;
    ACameraOutputTarget* jpegTarget = nullptr;
    ACameraDevice_createCaptureRequest(cameraDevice, TEMPLATE_PREVIEW, &request);

    ACameraOutputTarget_create(window, &jpegTarget);
    ACaptureRequest_addTarget(request, jpegTarget);



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
    coutbuf buf;

    if (!AndroidHasPermissions("CAMERA"))
        AndroidRequestAppPermissions("CAMERA");

    captureImage();

    CNFGSetupFullscreen("Skatliste", 0);
    CNFGSetLineWidth(1);

    AndroidDisplayKeyboard(true);

    while (CNFGHandleInput()) {

        CNFGBGColor = 0x000080ff;

        short w, h;
        CNFGClearFrame();

        CNFGBlitImage((uint32_t*)mem.data(), 100, 100, 640, 480);

        //CNFGGetDimensions( &w, &h );

        CNFGColor( 0xffffffff );
        CNFGPenX = 20;
        CNFGPenY = 20;
        CNFGDrawText(buf.c_str(), 4);

        CNFGSwapBuffers();
    }
}

