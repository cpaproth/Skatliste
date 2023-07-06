//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include <vector>
#include <array>
#include <thread>
#include <atomic>

class ListProc {
public:
	using Lines = std::vector<std::array<float, 4>>;
	float thfeat = 10.f;

	ListProc();
	~ListProc();

	void scan(std::vector<uint8_t>&, int32_t, int32_t);
	void result(Lines&);

private:
	int32_t w = 0;
	int32_t h = 0;
	std::vector<uint8_t> input;
	std::thread worker;
	std::atomic<bool> finished;
	Lines lines;

	void process();
};