//Copyright (C) 2023, 2024 Carsten Paproth - Licensed under MIT License

#include <vector>
#include <tuple>

class Classifier {
public:

	Classifier(int, int);

	void learn(uint8_t*, uint8_t);
	std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> classify(uint8_t*);

private:
	int w;
	int h;
	std::vector<std::vector<uint8_t>> chars;
	std::vector<uint8_t> labels;
};