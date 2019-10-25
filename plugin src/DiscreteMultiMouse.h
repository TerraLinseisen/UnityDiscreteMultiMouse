#pragma once

#ifdef UNITYPLUGIN_EXPORTS
	#define UNITYPLUGIN_API __declspec(dllexport)
#else
	#define UNITYPLUGIN_API __declspec(dllimport)
#endif

struct ButtonState {
	bool down, up, held;
	void reset() {
		down = false;
		up = false;
	};
};
struct MouseState {
	int x, y;
	ButtonState left, right;
	void reset() {
		x = 0;
		y = 0;
		left.reset();
		right.reset();
	};
};

extern "C" UNITYPLUGIN_API void Init();
extern "C" UNITYPLUGIN_API void Poll(MouseState** arr, int* len);
extern "C" UNITYPLUGIN_API void ResetMouseStates();
extern "C" UNITYPLUGIN_API void ResetMouseList();
extern "C" UNITYPLUGIN_API void ReRegisterMice();
extern "C" UNITYPLUGIN_API void Kill();
