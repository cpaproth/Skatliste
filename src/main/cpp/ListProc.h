//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include <vector>
#include <array>
#include <thread>
#include <atomic>

class ListProc {
	int32_t w = 0;
	int32_t h = 0;
	std::vector<uint8_t> input;
	std::thread worker;
	std::atomic<bool> finished;

	void process();

public:
	ListProc();
	~ListProc();

	void scan(std::vector<uint8_t>&, int32_t, int32_t);

	using Lines = std::vector<std::array<float, 4>>;
	void result(Lines&);
};