//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

#include <GLES3/gl3.h>

#include "NdkCam.h"
#include "SkatListProc.h"

class Program {
	GLuint cap_tex = 0;
	GLuint dig_tex = 0;
	NdkCam cam;
	SkatListProc proc;

public:
	Program();
	~Program();

	void draw();
};
