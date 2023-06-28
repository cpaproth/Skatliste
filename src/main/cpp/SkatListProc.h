//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include <vector>

class SkatListProc {
	int32_t w = 0;
	int32_t h = 0;
	std::vector<uint8_t> input;


public:

	void newlist(std::vector<uint8_t>&, int32_t, int32_t);

};