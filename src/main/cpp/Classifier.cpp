//Copyright (C) 2023, 2024 Carsten Paproth - Licensed under MIT License

#include "Classifier.h"
#include <fstream>
#include <iostream>

using namespace std;

Classifier::Classifier(int e) : extent(e) {
	ifstream file("/storage/emulated/0/Download/chars.181.ubyte", ifstream::binary);
	while (file) {
		labels.push_back(0);
		chars.emplace_back(e);
		file.read((char*)&labels.back(), 1);
		file.read((char*)chars.back().data(), e);
	}
	cout<<labels.size()<<" "<<chars.size()<< " "<<(int)labels[132]<<endl;
}

void Classifier::learn(uint8_t* chr, uint8_t l) {
	ofstream file("/storage/emulated/0/Download/chars.181.ubyte", ofstream::binary | ofstream::app);
	if (file) {
		file.write((char*)&l, 1);
		file.write((char*)chr, extent);
	}
}

tuple<uint8_t, float> Classifier::get(vector<uint8_t>&) {
	return {4, 5.f};
}
