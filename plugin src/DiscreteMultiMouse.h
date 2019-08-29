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

extern "C" UNITYPLUGIN_API void unityplugin_init();
extern "C" UNITYPLUGIN_API void unityplugin_poll(MouseState** arr, int* len);
extern "C" UNITYPLUGIN_API void unityplugin_resetMouseStates();
extern "C" UNITYPLUGIN_API void unityplugin_resetMouseList();
extern "C" UNITYPLUGIN_API void unityplugin_resetDeviceHwnds();
extern "C" UNITYPLUGIN_API void unityplugin_kill();
