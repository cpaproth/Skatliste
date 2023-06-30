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
	swap(input, l);

	worker = thread(&ListProc::process, this);
}

void ListProc::process() {
	//cout << "huhu" << endl;

	finished = true;
}

bool ListProc::result(vector<float>&) {
	if (!finished)
		return false;

	worker.join();
	finished = false;

	return true;
}


