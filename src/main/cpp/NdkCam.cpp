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

    manager = ACameraManager_create();
    ACameraManager_getCameraIdList(manager, &idlist);
    const char* id = i < idlist->numCameras? idlist->cameraIds[i]: "0";
    printprops(id);
    ACameraManager_openCamera(manager, id, &deviceCallbacks, &device);
    if ((rotate / 90 & 1) == 0)
        AImageReader_new(width, height, AIMAGE_FORMAT_YUV_420_888, 1, &reader);
    else
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
        cout << "orientation: " << (rotate = entry.data.i32[0]) << endl;

    if (ACameraMetadata_getConstEntry(metadata, ACAMERA_LENS_FACING, &entry) == ACAMERA_OK)
        cout << "backfacing: " << (int)entry.data.u8[0] << endl;

    ACameraMetadata_free(metadata);
}

void NdkCam::onimage(void* context, AImageReader* reader) {
    auto cam = (NdkCam*)context;

    AImage* image = 0;
    AImageReader_acquireLatestImage(reader, &image);

    uint8_t *Y = 0, *U = 0, *V = 0;
    int lY = 0, lU = 0, lV = 0;
    AImage_getPlaneData(image, 0, &Y, &lY);
    AImage_getPlaneData(image, 1, &U, &lU);
    AImage_getPlaneData(image, 2, &V, &lV);

    int32_t w, h, psU, psV, rsU, rsV;
    AImage_getWidth(image, &w);
    AImage_getHeight(image, &h);
    AImage_getPlanePixelStride(image, 1, &psU);
    AImage_getPlanePixelStride(image, 2, &psV);
    AImage_getPlaneRowStride(image, 1, &rsU);
    AImage_getPlaneRowStride(image, 2, &rsV);

    lock_guard<mutex> lg(cam->mut);
    int ro = cam->rotate / 90;
    cam->width = (ro & 1) == 0? w: h;
    cam->height = (ro & 1) == 0? h: w;
    cam->rgba.resize(w * h * 4, 255);
    cam->lum.resize(w * h, 255);

    for (unsigned y = 0; y < cam->height; y++) {
        for (unsigned x = 0; x < cam->width; x++) {
            size_t xi = ro == 0? x: ro == 1? y: ro == 2? w - x - 1: w - y - 1;
            size_t yi = ro == 0? y: ro == 1? h - x - 1: ro == 2? h - y - 1: x;
            size_t pos = 4 * (y * cam->width + x);
            size_t posY = yi * w + xi;
            size_t posU = (yi >> 1) * rsU + (xi >> 1) * psU;
            size_t posV = (yi >> 1) * rsV + (xi >> 1) * psV;
            if (posY < lY && posU < lU && posV < lV) {
                cam->lum[pos >> 2] = Y[posY];
                cam->rgba[pos + 1] = max(0, min(Y[posY] + U[posU] - 128, 255));
                cam->rgba[pos + 2] = max(0, min(Y[posY] - U[posU] - V[posV] + 256, 255));
                cam->rgba[pos + 3] = max(0, min(Y[posY] + V[posV] - 128, 255));
            }
        }
    }

    AImage_delete(image);
}
