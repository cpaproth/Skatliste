//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include <camera/NdkCameraManager.h>
#include <media/NdkImageReader.h>
#include <vector>

class NdkCam {
    int32_t width;
    int32_t height;
    std::vector<uint8_t> rgba;
    std::vector<uint8_t> lum;

    ACameraManager* manager = 0;
    ACameraIdList* idlist = 0;
    ACameraDevice* device = 0;
    AImageReader* reader = 0;
    ANativeWindow* window = 0;
    ACameraOutputTarget* target = 0;
    ACaptureRequest* request = 0;
    ACaptureSessionOutput* output = 0;
    ACaptureSessionOutputContainer* outputs = 0;
    ACameraCaptureSession* session = 0;

    void printprops(const char*);
    static void onimage(void*, AImageReader*);
public:
    NdkCam(int32_t, int32_t, size_t);
    ~NdkCam();
    uint32_t* rgbadata() {return (uint32_t*)rgba.data();}
};
