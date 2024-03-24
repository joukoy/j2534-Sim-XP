/*
**
** Copyright (C) 2009 Drew Technologies Inc.
** Author: Joey Oravec <joravec@drewtech.com>
**
** This library is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation, either version 3 of the License, or (at
** your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, <http://www.gnu.org/licenses/>.
**
*/


#include "stdafx.h"
#include <afxmt.h>

// #include "j2534_v0404.h"
#include "SelectionBox.h"
#include "shim_debug.h"
#include "shim_loader.h"
#include "shim_output.h"
#include "CMessages.h"

// Pointers to J2534 API functions in the loaded library
PTOPEN _PassThruOpen = 0;
PTCLOSE _PassThruClose = 0; PTCONNECT _PassThruConnect = 0;
PTDISCONNECT _PassThruDisconnect = 0;
PTREADMSGS _PassThruReadMsgs = 0;
PTWRITEMSGS _PassThruWriteMsgs = 0;
PTSTARTPERIODICMSG _PassThruStartPeriodicMsg = 0;
PTSTOPPERIODICMSG _PassThruStopPeriodicMsg = 0;
PTSTARTMSGFILTER _PassThruStartMsgFilter = 0;
PTSTOPMSGFILTER _PassThruStopMsgFilter = 0;
PTSETPROGRAMMINGVOLTAGE _PassThruSetProgrammingVoltage = 0;
PTREADVERSION _PassThruReadVersion = 0;
PTGETLASTERROR _PassThruGetLastError = 0;
PTIOCTL _PassThruIoctl = 0;

static HINSTANCE hDLL = NULL;

static bool fLibLoaded = false;
static LARGE_INTEGER ticksPerSecond;
static LARGE_INTEGER tick;
static CRITICAL_SECTION mAutoLock;

// Vista-forward has a great function InitOnceExecuteOnce() to thread-safe execute
// a callback exactly once, but we want to support Windows XP. Instead we'll guard
// with a globally initialized CCriticalSection.
static CCriticalSection CritSectionPerformanceCounter;
static bool fPerformanceCounterInitialized = false;
static CCriticalSection CritSectionAutoLock;
static bool fAutoLockInitialized = false;

CString TestFile;
CString msgLofGile;
void CreateLogFile(LPCTSTR FileName, BOOL append);

// static void EnumPassThruInterfaces(std::set<cPassThruInfo> &registryList);

auto_lock::auto_lock()
{
	// ONCE -- the first time somebody creates an autolock we need to initialize the mutex
	CritSectionAutoLock.Lock();
	if (! fAutoLockInitialized)
	{
		InitializeCriticalSection(&mAutoLock);
		fAutoLockInitialized = true;
	}
	CritSectionAutoLock.Unlock();

	if (! TryEnterCriticalSection(&mAutoLock))
	{
		dtDebug(_T("Multi-threading error"));
		EnterCriticalSection(&mAutoLock);
	}
}

auto_lock::~auto_lock()
{
	LeaveCriticalSection(&mAutoLock);
}



double GetTimeSinceInit()
{
	LARGE_INTEGER tock;
    double time;

	// ONCE -- the first time somebody gets a timestamp set the timer to 0.000s
	CritSectionPerformanceCounter.Lock();
	if (! fPerformanceCounterInitialized)
	{
		QueryPerformanceFrequency(&ticksPerSecond);
		QueryPerformanceCounter(&tick);
		fPerformanceCounterInitialized = true;
	}
	CritSectionPerformanceCounter.Unlock();

	QueryPerformanceCounter(&tock);
	time = (double)(tock.QuadPart-tick.QuadPart)/(double)ticksPerSecond.QuadPart;
	return time;
}

bool shim_checkAndAutoload(void)
{
	// We're OK if a library is loaded
	if (fLibLoaded)	return true;

	
	// Define ALLOW_POPUP if you want this function to continue by scaning the registry, presenting
	// a dialog, and allowing the user to pick a J2534 DLL. Leave it undefined if you want to force
	// the app to call PassThruLoadLibrary

	// Check the registry for J2534 interfaces
	std::set<cPassThruInfo> interfaceList;
//	EnumPassThruInterfaces(interfaceList);
	
		// Multiple interfaces? Popup a selection box!
		INT_PTR retval;
		CSelectionBox Dlg(interfaceList);

		retval = Dlg.DoModal();
		
		shim_writeLogfile(Dlg.GetDebugFilename(), true);
		TestFile = Dlg.GetTestFilename();
		msgLofGile = Dlg.GetMsgLogFilename();
		BOOL append = Dlg.GetAppendMsgLog();
		CreateLogFile(msgLofGile, append);
		return true;
}

bool shim_loadLibrary(LPCTSTR szDLL)
{
	// Can't load a library if the string is NULL
	if (szDLL == NULL)
	{
		return false;
	}

	// Can't load a library if there's one currently loaded
	if (fLibLoaded)
	{
		return false;
	}

	hDLL = LoadLibrary(szDLL);
	if (hDLL == NULL)
	{
		// Try to get the error text
		// Set the internal error text based on the win32 message
		return false;
	}

	fLibLoaded = true;

	_PassThruOpen = (PTOPEN)GetProcAddress(hDLL, "PassThruOpen");
	_PassThruClose = (PTCLOSE)GetProcAddress(hDLL, "PassThruClose");
	_PassThruConnect = (PTCONNECT)GetProcAddress(hDLL, "PassThruConnect");
	_PassThruDisconnect = (PTDISCONNECT)GetProcAddress(hDLL, "PassThruDisconnect");
	_PassThruReadMsgs = (PTREADMSGS)GetProcAddress(hDLL, "PassThruReadMsgs");
	_PassThruWriteMsgs = (PTWRITEMSGS)GetProcAddress(hDLL, "PassThruWriteMsgs");
	_PassThruStartPeriodicMsg = (PTSTARTPERIODICMSG)GetProcAddress(hDLL, "PassThruStartPeriodicMsg");
	_PassThruStopPeriodicMsg = (PTSTOPPERIODICMSG)GetProcAddress(hDLL, "PassThruStopPeriodicMsg");
	_PassThruStartMsgFilter = (PTSTARTMSGFILTER)GetProcAddress(hDLL, "PassThruStartMsgFilter");
	_PassThruStopMsgFilter = (PTSTOPMSGFILTER)GetProcAddress(hDLL, "PassThruStopMsgFilter");
	_PassThruSetProgrammingVoltage = (PTSETPROGRAMMINGVOLTAGE)GetProcAddress(hDLL, "PassThruSetProgrammingVoltage");
	_PassThruReadVersion = (PTREADVERSION)GetProcAddress(hDLL, "PassThruReadVersion");
	_PassThruGetLastError = (PTGETLASTERROR)GetProcAddress(hDLL, "PassThruGetLastError");
	_PassThruIoctl = (PTIOCTL)GetProcAddress(hDLL, "PassThruIoctl");

	return true;
}

void shim_unloadLibrary()
{
	// Can't unload a library if there's nothing loaded
	if (! fLibLoaded)
		return;

	fLibLoaded = false;

	// Invalidate the function pointers
	_PassThruOpen = NULL;
	_PassThruClose = NULL;
	_PassThruConnect = NULL;
	_PassThruDisconnect = NULL;
	_PassThruReadMsgs = NULL;
	_PassThruWriteMsgs = NULL;
	_PassThruStartPeriodicMsg = NULL;
	_PassThruStopPeriodicMsg = NULL;
	_PassThruStartMsgFilter = NULL;
	_PassThruStopMsgFilter = NULL;
	_PassThruSetProgrammingVoltage = NULL;
	_PassThruReadVersion = NULL;
	_PassThruGetLastError = NULL;
	_PassThruIoctl = NULL;

	BOOL fSuccess;
	fSuccess = FreeLibrary(hDLL);
	if (! fSuccess)
	{
		// Try to get the error text
		// Set the internal error text based on the win32 message
	}
}

bool shim_hasLibraryLoaded()
{
	return fLibLoaded;
}