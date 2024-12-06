//Copyright (C) 2023, 2024 Carsten Paproth - Licensed under MIT License

#include <vector>
#include <tuple>
#include <string>

class NeuralNet {
public:
	typedef std::vector<float> Values;

	NeuralNet(int i, float r) : I(i), rate(r) {}
	~NeuralNet();
	Values forward(Values);
	void backprop(Values, const Values&);
	void load(const std::string&);
	void save(const std::string&);
	void addLinear(int);
	void addSigmoid();
	void addReLU();
	void addAvgPooling(int, int, int, int);
	void addMaxPooling(int, int, int, int);
	void addConvolutional(int, int, int, int, int);

private:
	struct Layer {
		int I = 0, Iw = 0, Ih = 0, O = 0, Ow = 0, Oh = 0, Kw = 1, Kh = 1, Ws = 0;
		Values W;
		float rate = 1.f;
		virtual ~Layer() {}
		virtual Values forward(const Values&) = 0;
		virtual Values backprop(const Values&, const Values&, const Values&) = 0;
		void init(float r) {
			rate = r;
			Ws = I * Kw * Kh + 1;
			W.resize(O * Ws);
			for (int i = 0; i < W.size(); i++)
				W[i] = (float)rand() / (float)RAND_MAX * 2.f - 1.f;
		}
		void descent(const Values& grad) {
			for (int i = 0; i < W.size(); i++)
				W[i] -= rate * grad[i];
		}
	};
	struct Linear : Layer {
		Values forward(const Values&);
		Values backprop(const Values&, const Values&, const Values&);
	};
	struct Sigmoid : Layer {
		Values forward(const Values&);
		Values backprop(const Values&, const Values&, const Values&);
	};
	struct ReLU : Layer {
		Values forward(const Values&);
		Values backprop(const Values&, const Values&, const Values&);
	};
	struct AvgPooling : Layer {
		Values forward(const Values&);
		Values backprop(const Values&, const Values&, const Values&);
	};
	struct MaxPooling : Layer {
		Values forward(const Values&);
		Values backprop(const Values&, const Values&, const Values&);
	};
	struct Convolutional : Layer {
		Values forward(const Values&);
		Values backprop(const Values&, const Values&, const Values&);
	};

	int I;
	float rate;
	std::vector<Layer*> layers;
};

class Classifier {
public:
	Classifier(const std::string&, int, int, int);

	void learn(uint8_t*, uint8_t);
	std::tuple<uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t> classify(uint8_t*);
	NeuralNet::Values classify(const NeuralNet::Values& in) {return nn.forward(in);}
	//NeuralNet::Values classify(const NeuralNet::Values& in) {auto r = nn.forward(in); float s = 0.f; for (auto& v : r) s += v; for (auto& v : r) v /= s; return r;}

private:
	std::string path;
	int w;
	int h;
	int n;
	std::vector<std::vector<uint8_t>> chars;
	std::vector<uint8_t> labels;
	NeuralNet nn;
};