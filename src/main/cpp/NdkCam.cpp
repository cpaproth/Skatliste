//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include "NdkCam.h"
#include <iostream>

using namespace std;

NdkCam::NdkCam(int32_t w, int32_t h, size_t i) : width(w), height(h) {
    rgba.resize(w * h * 4, 255);
    lum.resize(w * h, 255);

    ACameraDevice_stateCallbacks deviceCallbacks{0, 0, 0};
    AImageReader_ImageListener imageCallbacks{this, onimage};
    ACameraCaptureSession_stateCallbacks sessionCallbacks{0, 0, 0, 0};
    ACameraCaptureSession_captureCallbacks captureCallbacks{0, 0, 0, 0, 0, 0, 0, 0};

    manager = ACameraManager_create();
    ACameraManager_getCameraIdList(manager, &idlist);
    const char* id = i < idlist->numCameras? idlist->cameraIds[i]: "0";
    printprops(id);
    ACameraManager_openCamera(manager, id, &deviceCallbacks, &device);
    AImageReader_new(height, width, AIMAGE_FORMAT_YUV_420_888, 1, &reader);
    AImageReader_setImageListener(reader, &imageCallbacks);
    AImageReader_getWindow(reader, &window);
    ANativeWindow_acquire(window);
    ACameraOutputTarget_create(window, &target);
    ACameraDevice_createCaptureRequest(device, TEMPLATE_PREVIEW, &request);
    ACaptureRequest_addTarget(request, target);
    ACaptureSessionOutput_create(window, &output);
    ACaptureSessionOutputContainer_create(&outputs);
    ACaptureSessionOutputContainer_add(outputs, output);
    ACameraDevice_createCaptureSession(device, outputs, &sessionCallbacks, &session);
    ACameraCaptureSession_setRepeatingRequest(session, &captureCallbacks, 1, &request, 0);
}

NdkCam::~NdkCam() {
    ACameraCaptureSession_close(session);
    ACaptureSessionOutputContainer_free(outputs);
    ACaptureSessionOutput_free(output);
    ACaptureRequest_free(request);
    ACameraOutputTarget_free(target);
    ANativeWindow_release(window);
    AImageReader_delete(reader);
    ACameraDevice_close(device);
    ACameraManager_deleteCameraIdList(idlist);
    ACameraManager_delete(manager);
}

void NdkCam::printprops(const char* id) {
    ACameraMetadata* metadata = 0;
    ACameraMetadata_const_entry entry;

    ACameraManager_getCameraCharacteristics(manager, id, &metadata);

    if (ACameraMetadata_getConstEntry(metadata, ACAMERA_SENSOR_INFO_EXPOSURE_TIME_RANGE, &entry) == ACAMERA_OK)
        cout << "exposure: " << entry.data.i64[0] << " - " << entry.data.i64[1] << endl;

    if (ACameraMetadata_getConstEntry(metadata, ACAMERA_SENSOR_INFO_SENSITIVITY_RANGE, &entry) == ACAMERA_OK)
        cout << "sensitivity: " << entry.data.i32[0] << " - " << entry.data.i32[1] << endl;

    ACameraMetadata_getConstEntry(metadata, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry);
    for (int i = 0; i < entry.count; i += 4)
        if (!entry.data.i32[i + 3] && entry.data.i32[i] == AIMAGE_FORMAT_YUV_420_888)
            cout << "size: " << entry.data.i32[i + 1] << " x " << entry.data.i32[i + 2] << endl;

    if (ACameraMetadata_getConstEntry(metadata, ACAMERA_SENSOR_ORIENTATION, &entry) == ACAMERA_OK)
        cout << "orientation: " << entry.data.i32[0] << endl;

    if (ACameraMetadata_getConstEntry(metadata, ACAMERA_LENS_FACING, &entry) == ACAMERA_OK)
        cout << "backfacing: " << (int)entry.data.u8[0] << endl;

    ACameraMetadata_free(metadata);
}

void NdkCam::onimage(void* context, AImageReader* reader) {
    NdkCam* cam = (NdkCam*)context;
    AImage *image = 0;
    AImageReader_acquireLatestImage(reader, &image);
    uint8_t *data = 0;
    int len = 0;
    AImage_getPlaneData(image, 0, &data, &len);

    int32_t w,h;
    AImage_getWidth(image, &w);
    AImage_getHeight(image, &h);
    //cout<<w<<" x "<<h<<endl;

    for (unsigned i = 0; i < 640 * 480 && i < len; i++) {
        cam->rgba[4 * i] = 255;
        cam->rgba[4 * i + 1] = data[i];
        cam->rgba[4 * i + 2] = data[i];
        cam->rgba[4 * i + 3] = data[i];
    }

    AImage_delete(image);
}
