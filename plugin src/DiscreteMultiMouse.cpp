#include "DiscreteMultiMouse.h"
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map> 
#include <vector>
#include <windows.h>

std::vector<MouseState> mouseStates;
std::unordered_map<HANDLE, size_t> devices; // lookup table, device HANDLE to index in mouseStates
std::vector<BYTE> buffer(sizeof(RAWINPUT));	// to avoid repeated allocations in tight loop
std::mutex mtx; // controls access to mouseStates
std::thread inputThread;
HWND window;

// index is assigned by order that input is first recieved by device
size_t getIndex(HANDLE device) {
	auto result = devices.try_emplace(device, devices.size());
	if (result.second) {
		mouseStates.emplace_back();
	}
	return result.first->second;
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

	if (buffer.size() < dataSize) {
		buffer.resize(dataSize);
	}

	if (GetRawInputData(rawInputHandle, RID_INPUT, buffer.data(), &dataSize, sizeof(RAWINPUTHEADER)) != dataSize) {
		std::cerr << "RawInput data size wrong" << std::endl;
		return;
	}

	PRAWINPUT rawInput = (RAWINPUT*)buffer.data();
	if (rawInput->header.dwType == RIM_TYPEMOUSE) {
        mtx.lock();
		updateMouseState(rawInput);
        mtx.unlock();
	}
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
		DispatchMessage(&msg);
	}
	return;
}

void unityplugin_init() {
	if (!inputThread.joinable()) {
		inputThread = std::thread(recieveInput);
	}
}

void unityplugin_poll(MouseState** arr, int* length) {
	mtx.lock();
	*arr = &mouseStates[0];
	*length = mouseStates.size();
}

void unityplugin_resetMouseStates() {
	for (size_t i = 0; i < mouseStates.size(); ++i) {
		mouseStates[i].reset();
	}
	mtx.unlock();
}

void unityplugin_resetMouseList() {
	mtx.lock();
	devices.clear();
	mouseStates.clear();
	mtx.unlock();
}

void unityplugin_reRegisterMice() {
	mtx.lock();
	RAWINPUTDEVICE rawInputDevice;
	rawInputDevice.usUsagePage = 0x01;	// generic desktop controls
	rawInputDevice.usUsage = 0x02;		// mouse
	rawInputDevice.dwFlags = RIDEV_INPUTSINK;	// receive input even when not in the foreground
	rawInputDevice.hwndTarget = window;
	RegisterRawInputDevices(&rawInputDevice, 1, sizeof(RAWINPUTDEVICE));
	mtx.unlock();
}

void unityplugin_kill() {
	if (inputThread.joinable()) {
		SendMessage(window, WM_CLOSE, NULL, NULL); // calls PostQuitMessage(0) breaking us out of GetMessage loop
		inputThread.join();
		for (size_t i = 0; i < mouseStates.size(); ++i) {
			mouseStates[i] = {};
		}
	}
}