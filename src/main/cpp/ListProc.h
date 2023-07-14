//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include <vector>
#include <array>
#include <thread>
#include <atomic>

class ListProc {
public:
	using Lines = std::vector<std::array<float, 4>>;
	int edge_th = 10;
	int line_th = 50;

	ListProc();
	~ListProc();

	void scan(std::vector<uint8_t>&, int32_t, int32_t);
	void result(Lines&);

private:
	int32_t w = 0;
	int32_t h = 0;
	static const int angs = 21;
	static const int range = 5;
	std::vector<float> cosa;
	std::vector<float> sina;

	std::vector<uint8_t> input;
	std::thread worker;
	std::atomic<bool> finished;
	Lines lines;

	std::vector<std::array<float, 2>> filter(std::vector<int>&);
	void process();
};