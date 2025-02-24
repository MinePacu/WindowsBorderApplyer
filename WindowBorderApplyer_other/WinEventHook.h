#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct WinEventHook
{
	DWORD event;
	HWND hwnd;
	LONG idObject;
	LONG idChild;
	DWORD idEventThread;
	DWORD dwmsEventTime;
};