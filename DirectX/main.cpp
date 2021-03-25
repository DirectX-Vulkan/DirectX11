//#include <vld.h> //memory leak detection tool

#include <Windows.h>
#include <iostream>

#include "Window.h"
#include "Renderer.h"
#include "Graphics.h"
#include "Benchmark.h"
#include <iostream>


#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

//const string MODEL_PATH = "models/cat.obj";
//const string TEXTURE_PATH = "textures/cat.jpg";
//const string MODEL_PATH = "models/cube.obj";
//const string MODEL_PATH = "models/viking_room.obj";
//const string TEXTURE_PATH = "textures/viking_room.png";


int main(HINSTANCE appInstance, HINSTANCE prevInstance, LPSTR cmdLine, int cmdCount) {
	// Get & parse arguments from cmdLine
	int argc;
	LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &argc);
	char** argv = new char* [argc];
	for (int i = 0; i < argc; i++) {
		int lgth = wcslen(szArglist[i]);
		argv[i] = new char[lgth + 1];
		for (int j = 0; j <= lgth; j++)
			argv[i][j] = char(szArglist[i][j]);
	}
	LocalFree(szArglist);

	int runTime;
	string name;
	string MODEL_PATH;
	string TEXTURE_PATH;
	if (argc == 1) {
		runTime = 20;
		name = "pcName";
		MODEL_PATH = "models/viking_room.obj";
		TEXTURE_PATH = "textures/viking_room.png";
	}
	else if (argc == 5) {
		name = (string)argv[1];
		runTime = stoi(argv[2]);
		MODEL_PATH = (string)argv[3];
		TEXTURE_PATH = (string)argv[4];
	}
	else
	{
		MessageBox(NULL, "Please specifiy at least 4 arguments \n\nExample: ./directx.exe ManfredsPc 20 models\\object.obj textures\\texture.jpg \n\nFirst arg: refference name \nSecond arg: benchmark run time \nThird arg: path of the model \nFourth arg: path of the texture", "Wrong arguments", MB_OK);
		return EXIT_FAILURE;
	}

	Window window(800, 600, name);

	Renderer renderer(window);
	Graphics graphics(renderer, MODEL_PATH, TEXTURE_PATH);
	Benchmark benchmark(runTime, name, "directx11", MODEL_PATH);

	MSG msg = { 0 };

	// Main message loop:
	while (WM_QUIT != msg.message && benchmark.run())
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		const float c = sin(benchmark.PeekTimer()) / 2.0f + 0.5f;
		renderer.beginFrame(c, c, c);

		graphics.draw(&renderer, -benchmark.PeekTimer(), 0.0f, 1.0f);

		benchmark.UpdateBenchmark();

		renderer.endFrame();
	}

	return (int)msg.wParam;
}