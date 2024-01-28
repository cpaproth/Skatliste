//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include <camera/NdkCameraManager.h>
#include <media/NdkImageReader.h>
#include <vector>
#include <mutex>

class NdkCam {
public:
	NdkCam(int32_t, int32_t, size_t);
	~NdkCam();

	int32_t w() {
		return width;
	}
	int32_t h() {
		return height;
	}
	bool cap() {
		return capture;
	}
	void get_rgba(std::function<void(void*, int, int)> f) {
		std::lock_guard<std::mutex> lg(mut);
		f(rgba.data(), width, height);
	}
	void swap_lum(std::function<void(std::vector<uint8_t>&, int32_t, int32_t)> f) {
		std::lock_guard<std::mutex> lg(mut);
		f(lum, width, height);
	}
	void start() {
		ACaptureRequest_setEntry_u8(request, ACAMERA_FLASH_MODE, 1, (uint8_t[]){ACAMERA_FLASH_MODE_TORCH});
		ACameraCaptureSession_setRepeatingRequest(session, 0, 1, &request, 0);
		capture = true;
	}
	void stop() {
		capture = false;
		ACameraCaptureSession_stopRepeating(session);
		ACaptureRequest_setEntry_u8(request, ACAMERA_FLASH_MODE, 1, (uint8_t[]){ACAMERA_FLASH_MODE_OFF});
		ACameraCaptureSession_capture(session, 0, 1, &request, 0);
	}

private:
	int32_t width;
	int32_t height;
	int32_t rotate = 0;
	std::vector<uint8_t> rgba;
	std::vector<uint8_t> lum;
	std::mutex mut;
	bool capture = false;

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
};
