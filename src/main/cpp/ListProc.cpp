//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include "ListProc.h"
#include "utils.h"
#include <iostream>

using namespace std;

ListProc::ListProc() : finished(false) {

}

ListProc::~ListProc() {
	if (worker.joinable())
		worker.join();
}

void ListProc::scan(vector<uint8_t>& l, int32_t width, int32_t height) {
	if (worker.joinable())
		return;

	w = width;
	h = height;
	swap(input, l);
	input.resize(w * h);

	worker = thread(&ListProc::process, this);
}

void ListProc::result(Lines& l) {
	if (!finished)
		return;

	worker.join();
	finished = false;

	swap(lines, l);
}

void ListProc::process() {
	auto finish = guard([&] {finished = true;});

	lines.clear();

	for (int x = 1; x < w - 1; x++) for (int y = 1; y < h - 1; y++) {
			float dx = input[y * w + x + 1] - input[y * w + x - 1];
			float dy = input[(y + 1) * w + x] - input[(y - 1) * w + x];
			if (dx * dx + dy * dy > thfeat) {
				lines.push_back({float(x) - 2.f, float(y), float(x) + 2.f, float(y)});
			}
		}

}
