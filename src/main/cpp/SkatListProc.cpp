//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include "SkatListProc.h"
#include <iostream>

using namespace std;

SkatListProc::SkatListProc() : finished(false) {

}

SkatListProc::~SkatListProc() {
	if (worker.joinable())
		worker.join();
}

void SkatListProc::scan(vector<uint8_t>& l, int32_t width, int32_t height) {
	if (worker.joinable())
		return;

	w = width;
	h = height;
	swap(input, l);

	worker = thread(&SkatListProc::process, this);
}

void SkatListProc::process() {
	//cout << "huhu" << endl;

	finished = true;
}

bool SkatListProc::result(vector<float>&) {
	if (!finished)
		return false;

	worker.join();
	finished = false;

	return true;
}


