//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include "ListProc.h"
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
	input.resize(w * h);
	swap(input, l);

	worker = thread(&ListProc::process, this);
}

void ListProc::process() {
	for (int x = 1; x < w - 1; x++) for (int y = 1; y < h - 1; y++) {
		float dx = input[y * w + x + 1] - input[y * w + x - 1];
		float dy = input[(y + 1) * w + x] - input[(y - 1) * w + x];
		if (sqrtf(dx * dx + dy * dy) > 100.f);
	}


	finished = true;
}

void ListProc::result(Lines& lines) {
	if (!finished)
		return;

	worker.join();
	finished = false;

	lines.clear();

	lines.push_back({10.f, 20.f, 300.f, 50.f});
	lines.push_back({300.f, 50.f, 400.f, 500.f});

}


