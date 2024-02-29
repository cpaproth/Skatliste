//Copyright (C) 2023, 2024 Carsten Paproth - Licensed under MIT License

#include "Classifier.h"
#include <fstream>
#include <map>
#include <iostream>

using namespace std;

NeuralNet::~NeuralNet() {
	for (int i = 0; i < layers.size(); i++)
		delete layers[i];
}

NeuralNet::Values NeuralNet::forward(Values in) {
	in.resize(I);
	for (int i = 0; i < layers.size(); i++)
		in = layers[i]->forward(in);
	return in;
}

void NeuralNet::backprop(Values in, const Values& out) {
	vector<Values> neurons(1, in);
	for (int i = 0; i < layers.size(); i++)
		neurons.emplace_back(in = layers[i]->forward(in));

	Values delta(neurons.back().size());
	for (int i = 0; i < delta.size() && i < out.size(); i++)
		delta[i] = neurons.back()[i] - out[i];

	for (int i = neurons.size() - 1; i > 0; i--)
		delta = layers[i - 1]->backprop(delta, neurons[i], neurons[i - 1]);
}

void NeuralNet::load(const string& filename) {
	ifstream file(filename.c_str(), ifstream::binary);
	for (int i = 0; i < layers.size() && file; i++)
		file.read((char*)layers[i]->W.data(), layers[i]->W.size() * sizeof(float));
}

void NeuralNet::save(const string& filename) {
	ofstream file(filename.c_str(), ofstream::binary);
	for (int i = 0; i < layers.size() && file; i++)
		file.write((char*)layers[i]->W.data(), layers[i]->W.size() * sizeof(float));
}

void NeuralNet::addLinear(int O) {
	Layer* l = new Linear();
	l->O = O;
	l->I = forward(Values(I)).size();
	l->init(rate);
	layers.push_back(l);
}

void NeuralNet::addSigmoid() {
	layers.push_back(new Sigmoid());
}

void NeuralNet::addReLU() {
	layers.push_back(new ReLU());
}

void NeuralNet::addAvgPooling(int Ow, int Oh, int Kw, int Kh) {
	Layer* l = new AvgPooling();
	l->Iw = Ow * Kw;
	l->Ih = Oh * Kh;
	l->Ow = Ow;
	l->Oh = Oh;
	l->Kw = Kw;
	l->Kh = Kh;
	l->I = forward(Values(I)).size() / (l->Iw * l->Ih);
	layers.push_back(l);
}

void NeuralNet::addMaxPooling(int Ow, int Oh, int Kw, int Kh) {
	Layer* l = new MaxPooling();
	l->Iw = Ow * Kw;
	l->Ih = Oh * Kh;
	l->Ow = Ow;
	l->Oh = Oh;
	l->Kw = Kw;
	l->Kh = Kh;
	l->I = forward(Values(I)).size() / (l->Iw * l->Ih);
	layers.push_back(l);
}

void NeuralNet::addConvolutional(int O, int Ow, int Oh, int Kw, int Kh) {
	Layer* l = new Convolutional();
	l->O = O;
	l->Iw = Ow + Kw - 1;
	l->Ih = Oh + Kh - 1;
	l->Ow = Ow;
	l->Oh = Oh;
	l->Kw = Kw;
	l->Kh = Kh;
	l->I = forward(Values(I)).size() / (l->Iw * l->Ih);
	l->init(rate);
	layers.push_back(l);
}

NeuralNet::Values NeuralNet::Linear::forward(const Values& in) {
	Values out(O);
	for (int o = 0; o < O; o++) {
		out[o] = W[o * Ws + I];
		for (int i = 0; i < I; i++)
			out[o] += in[i] * W[o * Ws + i];
	}
	return out;
}

NeuralNet::Values NeuralNet::Linear::backprop(const Values& delta, const Values&, const Values& prev) {
	Values out(I);
	Values grad(W.size());
	for (int o = 0; o < O; o++) {
		grad[o * Ws + I] = delta[o];
		for (int i = 0; i < I; i++) {
			grad[o * Ws + i] += prev[i] * delta[o];
			out[i] += W[o * Ws + i] * delta[o];
		}
	}
	descent(grad);
	return out;
}

NeuralNet::Values NeuralNet::Sigmoid::forward(const Values& in) {
	Values out(in.size());
	for (int i = 0; i < in.size(); i++)
		out[i] = 1.f / (1.f + exp(-in[i]));
	return out;
}

NeuralNet::Values NeuralNet::Sigmoid::backprop(const Values& delta, const Values& cur, const Values&) {
	Values out(delta.size());
	for (int i = 0; i < delta.size(); i++)
		out[i] = delta[i] * cur[i] * (1.f - cur[i]);
	return out;
}

NeuralNet::Values NeuralNet::ReLU::forward(const Values& in) {
	Values out(in.size());
	for (int i = 0; i < in.size(); i++)
		out[i] = max(0.f, 0.25f * in[i]);
	return out;
}

NeuralNet::Values NeuralNet::ReLU::backprop(const Values& delta, const Values&, const Values& prev) {
	Values out(delta.size());
	for (int i = 0; i < delta.size(); i++)
		out[i] = prev[i] > 0.f? 0.25f * delta[i]: 0.f;
	return out;
}

NeuralNet::Values NeuralNet::AvgPooling::forward(const Values& in) {
	Values out(I * Ow * Oh);
	for (int i = 0; i < I; i++)
		for (int y = 0; y < Ih; y++)
			for (int x = 0; x < Iw; x++)
				out[i * Ow * Oh + y / Kh * Ow + x / Kw] += in[i * Iw * Ih + y * Iw + x] / (Kw * Kh);
	return out;
}

NeuralNet::Values NeuralNet::AvgPooling::backprop(const Values& delta, const Values&, const Values&) {
	Values out(I * Iw * Ih);
	for (int i = 0; i < I; i++)
		for (int y = 0; y < Ih; y++)
			for (int x = 0; x < Iw; x++)
				out[i * Iw * Ih + y * Iw + x] = delta[i * Ow * Oh + y / Kh * Ow + x / Kw] / (Kw * Kh);
	return out;
}

NeuralNet::Values NeuralNet::MaxPooling::forward(const Values& in) {
	Values out(I * Ow * Oh, -FLT_MAX);
	for (int i = 0; i < I; i++)
		for (int y = 0; y < Ih; y++)
			for (int x = 0; x < Iw; x++)
				out[i * Ow * Oh + y / Kh * Ow + x / Kw] = max(in[i * Iw * Ih + y * Iw + x], out[i * Ow * Oh + y / Kh * Ow + x / Kw]);
	return out;
}

NeuralNet::Values NeuralNet::MaxPooling::backprop(const Values& delta, const Values& cur, const Values& prev) {
	Values out(I * Iw * Ih);
	for (int i = 0; i < I; i++)
		for (int y = 0; y < Ih; y++)
			for (int x = 0; x < Iw; x++)
				out[i * Iw * Ih + y * Iw + x] = cur[i * Ow * Oh + y / Kh * Ow + x / Kw] == prev[i * Iw * Ih + y * Iw + x]? delta[i * Ow * Oh + y / Kh * Ow + x / Kw]: 0.f;
	return out;
}

NeuralNet::Values NeuralNet::Convolutional::forward(const Values& in) {
	Values out(O * Ow * Oh);
	for (int o = 0; o < O; o++)
		for (int y = 0; y < Oh; y++)
			for (int x = 0; x < Ow; x++) {
				out[o * Ow * Oh + y * Ow + x] = W[o * Ws + Ws - 1];
				for (int i = 0; i < I; i++)
					for (int dy = 0; dy < Kh; dy++)
						for (int dx = 0; dx < Kw; dx++)
							out[o * Ow * Oh + y * Ow + x] += in[i * Iw * Ih + (y + dy) * Iw + x + dx] * W[o * Ws + i * Kw * Kh + dy * Kw + dx];
			}
	return out;
}

NeuralNet::Values NeuralNet::Convolutional::backprop(const Values& delta, const Values&, const Values& prev) {
	Values out(I * Iw * Ih);
	Values grad(W.size());
	for (int o = 0; o < O; o++)
		for (int y = 0; y < Oh; y++)
			for (int x = 0; x < Ow; x++) {
				grad[o * Ws + Ws - 1] += delta[o * Ow * Oh + y * Ow + x];
				for (int i = 0; i < I; i++)
					for (int dy = 0; dy < Kh; dy++)
						for (int dx = 0; dx < Kw; dx++) {
							grad[o * Ws + i * Kw * Kh + dy * Kw + dx] += prev[i * Iw * Ih + (y + dy) * Iw + x + dx] * delta[o * Ow * Oh + y * Ow + x];
							out[i * Iw * Ih + (y + dy) * Iw + x + dx] += W[o * Ws + i * Kw * Kh + dy * Kw + dx] * delta[o * Ow * Oh + y * Ow + x];
						}
			}
	descent(grad);
	return out;
}

Classifier::Classifier(int width, int height) : w(width), h(height), nn(w * h, 0.f) {
	ifstream file("/storage/emulated/0/Download/chars.181.ubyte", ifstream::binary);
	char mem[w * h + 1];
	while (file.read(mem, w * h + 1)) {
		labels.push_back(mem[0]);
		chars.emplace_back(mem + 1, mem + w * h + 1);
	}
	cout << "characters: " << labels.size() << endl;

	nn.addConvolutional(20, 9, 12, 4, 4);
	nn.addSigmoid();
	nn.addMaxPooling(3, 4, 3, 3);
	nn.addLinear(100);
	nn.addSigmoid();
	nn.addLinear(12);
	nn.addSigmoid();
	nn.load("/storage/emulated/0/Download/nn.1.flt");
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

tuple<uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t> Classifier::classify(uint8_t* chr) {
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
		return {0, 0, 0, 0, 0, 255};
	vector<int> count(256, 0);
	for (auto it = knn.begin(); it != knn.end(); it++)
		count[it->second]++;
	auto mk = max_element(count.begin(), count.end());

	NeuralNet::Values in(w * h), out;
	for (int i = 0; i < in.size(); i++)
		in[i] = chr[i] / 255.f;
	out = nn.forward(in);
	auto mn = max_element(out.begin(), out.end());

	return {mn - out.begin(), *mn * 100, mk - count.begin(), *mk * 10, knn.begin()->second, knn.begin()->first};
}
