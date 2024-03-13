//Copyright (C) 2023, 2024 Carsten Paproth - Licensed under MIT License

#include <GLES3/gl3.h>

#include "NdkCam.h"
#include "ListProc.h"
#include "Classifier.h"

class Program {
public:
	Program();
	~Program();

	void draw();

private:
	GLuint cap_tex = 0;
	GLuint dig_tex = 0;
	NdkCam cam;
	ListProc proc;
	ListProc::Lines lines;
	Fields fields;
	Classifier clss;
	static const std::vector<const char*> chars;

	void read_field();

};
