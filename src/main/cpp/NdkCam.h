//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include <camera/NdkCameraManager.h>
#include <media/NdkImageReader.h>
#include <vector>
#include <mutex>

class NdkCam {
	int32_t width;
	int32_t height;
	int32_t rotate = 0;
	std::vector<uint8_t> rgba;
	std::vector<uint8_t> lum;
	std::mutex mut;

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

	int32_t w() {
		return width;
	}
	int32_t h() {
		return height;
	}
	unsigned get_rgba(std::function<unsigned(uint32_t*, int, int)> f) {
		std::lock_guard<std::mutex> lg(mut);
		return f((uint32_t*)rgba.data(), width, height);
	}
	void get_lum(std::function<void(uint8_t*, int, int)> f) {
		std::lock_guard<std::mutex> lg(mut);
		f(lum.data(), width, height);
	}
	void start() {
		ACameraCaptureSession_setRepeatingRequest(session, 0, 1, &request, 0);
   }
	void stop() {
		ACameraCaptureSession_stopRepeating(session);
	}
};
