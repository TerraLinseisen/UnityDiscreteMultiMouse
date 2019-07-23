#include "UnityPlugin.h"
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <windows.h>

std::vector<MouseState> mouseStates;
std::vector<HANDLE> devices; // lookup table, device HANDLE to index in mouseStates
std::mutex mtx; // controls access to mouseStates
std::thread inputThread;
HWND window;

// index is assigned by order that input is first recieved by device
size_t getIndex(HANDLE device) {
	size_t i = 0;
	for (; i < devices.size(); ++i) {
		if (devices[i] == device) {
			return i;
		}
	}
	devices.push_back(device);
	mouseStates.emplace_back();
	return i;
}

void updateMouseState(PRAWINPUT rawInput) {
	size_t index = getIndex(rawInput->header.hDevice);
	MouseState& mouseState = mouseStates[index];

	mouseState.x += rawInput->data.mouse.lLastX;
	mouseState.y -= rawInput->data.mouse.lLastY;

	if (rawInput->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
		mouseState.left.down = true;
		mouseState.left.held = true;
	} else if (rawInput->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) {
		mouseState.left.up = true;
		mouseState.left.held = false;
	}

	if (rawInput->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
		mouseState.right.down = true;
		mouseState.right.held = true;
	} else if (rawInput->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) {
		mouseState.right.up = true;
		mouseState.right.held = false;
	}
}

void onRawInput(HRAWINPUT rawInputHandle) {
	UINT dataSize;
	GetRawInputData(rawInputHandle, RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));
	if (dataSize == 0) {
		std::cerr << "RawInput data size 0" << std::endl;
		return;
	}

	BYTE* data = new BYTE[dataSize];
	if (GetRawInputData(rawInputHandle, RID_INPUT, data, &dataSize, sizeof(RAWINPUTHEADER)) != dataSize) {
		std::cerr << "RawInput data size wrong" << std::endl;
		delete[] data;
		return;
	}

	PRAWINPUT rawInput = (RAWINPUT*)data;
	if (rawInput->header.dwType == RIM_TYPEMOUSE) {
		updateMouseState(rawInput);
	}

	delete[] data;
}

LRESULT CALLBACK WndProc(HWND window, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
		case WM_CLOSE:
			PostQuitMessage(0);
			break;
		case WM_INPUT:
			onRawInput((HRAWINPUT)lp);
			break;
	}
	return DefWindowProc(window, msg, wp, lp);
}

void recieveInput() {
	WNDCLASSEX windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = WndProc;	// message handler
	windowClass.lpszClassName = L"inputCapture";
	windowClass.hInstance = GetModuleHandle(NULL);
	// don't register windowClass if it already exists
	if (!GetClassInfoEx(GetModuleHandle(NULL), windowClass.lpszClassName, &windowClass)) {
		if (!RegisterClassEx(&windowClass)) {
			std::cerr << "window class registration failed with error: " << GetLastError() << std::endl;
			return;
		}
	}

	window = CreateWindow(windowClass.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);
	if (window == NULL) {
		std::cerr << "window creation failed with error: " << GetLastError() << std::endl;
		return;
	}

	RAWINPUTDEVICE rawInputDevice;
	rawInputDevice.usUsagePage = 0x01;	// generic desktop controls
	rawInputDevice.usUsage = 0x02;		// mouse
	rawInputDevice.dwFlags = RIDEV_INPUTSINK;	// receive input even when not in the foreground
	rawInputDevice.hwndTarget = window;
	if (!RegisterRawInputDevices(&rawInputDevice, 1, sizeof(RAWINPUTDEVICE))) {
		std::cerr << "registration failed with error: " << GetLastError() << std::endl;
		return;
	}

	MSG msg;
	// WM_INPUT as GetMessage parameters stops us from recieving other message types
	while (GetMessage(&msg, window, WM_INPUT, WM_INPUT)) {
		mtx.lock();
		DispatchMessage(&msg);
		mtx.unlock();
	}
	return;
}

void unityplugin_init() {
	inputThread = std::thread(recieveInput);
}

void unityplugin_poll(MouseState** arr, int* length) {
	mtx.lock();
	*arr = &mouseStates[0];
	*length = mouseStates.size();
}

void unityplugin_reset() {
	for (size_t i = 0; i < mouseStates.size(); ++i) {
		mouseStates[i].reset();
	}
	mtx.unlock();
}

void unityplugin_kill() {
	SendMessage(window, WM_CLOSE, NULL, NULL); // calls PostQuitMessage(0) breaking us out of GetMessage loop
	inputThread.join();
	for (size_t i = 0; i < mouseStates.size(); ++i) {
		mouseStates[i] = {};
	}
}
