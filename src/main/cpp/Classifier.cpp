//Copyright (C) 2023, 2024 Carsten Paproth - Licensed under MIT License

#include "Classifier.h"
#include <fstream>
#include <map>
#include <iostream>

using namespace std;

Classifier::Classifier(int width, int height) : w(width), h(height) {
	ifstream file("/storage/emulated/0/Download/chars.181.ubyte", ifstream::binary);
	char mem[w * h + 1];
	while (file.read(mem, w * h + 1)) {
		labels.push_back(mem[0]);
		chars.emplace_back(mem + 1, mem + w * h + 1);
	}
	cout << "characters: " << labels.size() << endl;
}

void Classifier::learn(uint8_t* chr, uint8_t l) {
	ofstream file("/storage/emulated/0/Download/chars.181.ubyte", ofstream::binary | ofstream::app);
	if (file) {
		file.write((char*)&l, 1);
		file.write((char*)chr, w * h);
	}
	labels.push_back(l);
	chars.emplace_back(chr, chr + w * h);
}

tuple<uint8_t, uint8_t, uint8_t, uint8_t> Classifier::classify(uint8_t* chr) {
	multimap<float, uint8_t> knn;
	for (int c = 0; c < chars.size(); c++) {
		float diff = 0.f;
		for (int i = 0; i < w * h; i++)
			diff += pow(chr[i] - chars[c][i], 2.f);
		knn.insert({sqrt(diff / w / h), labels[c]});
		if (knn.size() > 10)
			knn.erase(prev(knn.end()));
	}
	if (knn.size() == 0)
		return {0, 0, 0, 255};
	vector<int> count(256, 0);
	for (auto it = knn.begin(); it != knn.end(); it++)
		count[it->second]++;
	auto ma = max_element(count.begin(), count.end());
	return {ma - count.begin(), *ma * 10, knn.begin()->second, knn.begin()->first};
}
