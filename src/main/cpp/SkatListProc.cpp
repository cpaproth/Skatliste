//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include "SkatListProc.h"

using namespace std;

void SkatListProc::newlist(vector<uint8_t>& l, int32_t width, int32_t height) {
	w = width;
	h = height;
	swap(input, l);
}
