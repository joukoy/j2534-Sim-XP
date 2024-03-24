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

// #include <stdafx.h>
#include "stdafx.h"


#include <tchar.h>
#include <windows.h> 

#include "j2534_v0404.h"
#include "shim_debug.h"
#include "shim_loader.h"
#include "shim_output.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include <afxdisp.h>
#include "CMessages.h"
#include <vector>
#include <queue>
#include <stdint.h>
#define _POSIX_C_SOURCE 199309L
#include <time.h>
/// Convert seconds to milliseconds
#define SEC_TO_MS(sec) ((sec)*1000)
/// Convert seconds to microseconds
#define SEC_TO_US(sec) ((sec)*1000000)
/// Convert seconds to nanoseconds
#define SEC_TO_NS(sec) ((sec)*1000000000)

/// Convert nanoseconds to seconds
#define NS_TO_SEC(ns)   ((ns)/1000000000)
/// Convert nanoseconds to milliseconds
#define NS_TO_MS(ns)    ((ns)/1000000)
/// Convert nanoseconds to microseconds
#define NS_TO_US(ns)    ((ns)/1000)

using namespace std;
string message;
int Readreply = 0;
string PathOfDll;
BOOL Echo = false;
BOOL Echo2 = false;
//int numofresponse;
int ChannelId = 2;
int proto = 5;
std::queue<std::string> responses;

extern CString TestFile;
void AppendLog(std::string Txt);
void CloseLog();
void CloseDebugFile();

std::vector<std::string> msgKeys;
std::vector<std::string> msgValues;

struct TimedMsg
{
	string Msg;
	long long SendTime;
};
std::vector<TimedMsg> timedMsgs;

string thisDllDirPath()
{
	CStringW thisPath = L"";
	WCHAR path[MAX_PATH];
	HMODULE hm;
	if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPWSTR)&thisDllDirPath, &hm))
	{
		GetModuleFileNameW(hm, path, MAX_PATH);
		PathRemoveFileSpecW(path);
		thisPath = CStringW(path);
		if (!thisPath.IsEmpty() &&
			thisPath.GetAt(thisPath.GetLength() - 1) != '\\')
			thisPath += L"\\";
	}
	//	else if (_DEBUG) std::wcout << L"GetModuleHandle Error: " << GetLastError() << std::endl;

	//	if (_DEBUG) std::wcout << L"thisDllDirPath: [" << CStringW::PCXSTR(thisPath) << L"]" << std::endl;
	return { thisPath.GetString(), thisPath.GetString() + thisPath.GetLength() };
}

void ReadTestFile()
{
	string line;
	//ifstream myfile(PathOfDll + "test.txt");
	ifstream myfile(TestFile);
	int row = 0;
	std::vector<std::string> fileLines;

	if (myfile && myfile.is_open())
	{
		msgKeys.clear();
		msgValues.clear();
		while (getline(myfile, line))
		{
			line.erase(remove(line.begin(), line.end(), ' '), line.end()); //remove spaces
			transform(line.begin(), line.end(), line.begin(),::toupper);
			int c = line.find('/');
			if (c >= 0)
			{
				line = line.substr(0, c); // remove comments
			}
			c = line.find('=');
			if (c > 2)
			{
				msgKeys.push_back(line.substr(0, c));
				msgValues.push_back(line.substr(c + 1, line.size() - c));
			}
		}
		myfile.close();
	}
}

#define SHIM_CHECK_DLL() \
{ \
	if (! shim_checkAndAutoload()) \
	{ \
       	shim_setInternalError(_T("PassThruShim has not loaded a J2534 DLL")); \
		dbug_printretval(ERR_FAILED); \
		return ERR_FAILED; \
	} \
}
 /*
#define SHIM_CHECK_FUNCTION(fcn) \
{ \
	if (__FUNCTION__ == NULL) \
	{ \
		shim_setInternalError(_T("DLL loaded but does not export %s"), __FUNCTION__); \
		dbug_printretval(ERR_FAILED); \
		return ERR_FAILED; \
	} \
}
*/

/*
extern "C" long J2534_API PassThruLoadLibrary(char * szFunctionLibrary)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;

	shim_clearInternalError();
*/
	// dtDebug(_T("%.3fs ++ PTLoadLibrary(%s)\n"), GetTimeSinceInit(), (szFunctionLibrary==NULL)?_T("*NULL*"):_T("test")/*szLibrary*/);
	/*
	if (szFunctionLibrary == NULL)
	{
		// Return an error. Perhaps we want to change NULL to do an autodetect and popup?
	//	MessageBoxA(NULL, "Line 65.PassThruLoadLibrary", "if szfunction library is null.", MB_OK | MB_ICONERROR);
		shim_setInternalError(_T("szFunctionLibrary was zero"));
		dbug_printretval(ERR_NULL_PARAMETER);
		return ERR_NULL_PARAMETER;
	}

	CStringW cstrLibrary(szFunctionLibrary);
	bool fSuccess;
	fSuccess = true; //  shim_loadLibrary(cstrLibrary);
	if (! fSuccess)
	{
		shim_setInternalError(_T("Failed to open '%s'"), cstrLibrary);
		dbug_printretval(ERR_FAILED);
		return ERR_FAILED;
	}

	dbug_printretval(STATUS_NOERROR);
	return STATUS_NOERROR;
}
*/
/*
extern "C" long J2534_API PassThruUnloadLibrary()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;

	shim_clearInternalError();

	dtDebug(_T("%.3fs ++ PTUnloadLibrary()\n"), GetTimeSinceInit());

	shim_unloadLibrary();

	dbug_printretval(STATUS_NOERROR);
	return STATUS_NOERROR;
}
*/
long long GetUnixTime()
{
	const long long WINDOWS_TICK = 10000000LL;
	const long long SEC_TO_UNIX_EPOCH = 11644473600LL;

	SYSTEMTIME st;
	GetSystemTime(&st);

	FILETIME ft;
	if (!SystemTimeToFileTime(&st, &ft))
		return -1;

	ULARGE_INTEGER uli;
	uli.LowPart = ft.dwLowDateTime;
	uli.HighPart = ft.dwHighDateTime;
	return uli.QuadPart / WINDOWS_TICK - SEC_TO_UNIX_EPOCH;
}

extern "C" long J2534_API PassThruWriteToLogA(char *szMsg)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CStringW cstrMsg(szMsg);

	dtDebug(_T("%.3fs ** '%s'\n"), GetTimeSinceInit(), cstrMsg);

	return STATUS_NOERROR;
}

extern "C" long J2534_API PassThruWriteToLogW(wchar_t *szMsg)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	dtDebug(_T("%.3fs ** '%s'\n"), GetTimeSinceInit(), szMsg);

	return STATUS_NOERROR;
}

extern "C" long J2534_API PassThruSaveLog(char *szFilename)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;

	shim_clearInternalError();

	dtDebug(_T("%.3fs ++ PTSaveLog(%s)\n"), GetTimeSinceInit(), (szFilename==NULL)?_T("*NULL*"):_T("")/*pName*/);

	CStringW cstrFilename(szFilename);
	shim_writeLogfile(cstrFilename, false);

	dbug_printretval(STATUS_NOERROR);
	return STATUS_NOERROR;
}

extern "C" long J2534_API PassThruOpen(void *pName, unsigned long *pDeviceID)
{
	PathOfDll = thisDllDirPath();
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;
	unsigned long retval;

	//pName = "J2534 Sim";
	if (*pDeviceID > 0x0F)
		*pDeviceID = 1;

	shim_clearInternalError();
	if (!shim_checkAndAutoload()) 
	{ 
		shim_setInternalError(_T("PassThruShim has not loaded a J2534 DLL")); 
		dbug_printretval(ERR_FAILED); 
		return ERR_FAILED; 
	} 

	dtDebug(_T("%.3fs ++ PTOpen(%s, Device ID %1d)  "), GetTimeSinceInit(), (pName==NULL)?_T("*NULL*"):_T("")/*pName*/, *pDeviceID);

	retval = 0; // _PassThruOpen(pName, pDeviceID);
//	dtDebug(_T("Returning DeviceID: %ld\n"), *pDeviceID);

	dbug_printretval(retval);
	return retval;
}

extern "C" long J2534_API PassThruClose(unsigned long DeviceID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;
	long retval;

	shim_clearInternalError();
	dtDebug(_T("%.3fs -- PTClose(%ld)\n"), GetTimeSinceInit(), DeviceID);

	retval = 0; // _PassThruClose(DeviceID);

	dbug_printretval(retval);
	CloseLog();
	CloseDebugFile();

	return retval;
}

extern "C" long J2534_API PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long *pChannelID)
{
	
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;
	long retval;
	
	shim_clearInternalError();
	dtDebug(_T("%.3fs ++ PTConnect(Decive #%ld, %s, 0x%08X, %ld, Channel #%1d)\n"), GetTimeSinceInit(), DeviceID, dbug_prot(ProtocolID).c_str(), Flags, Baudrate, *pChannelID);

	dbug_printcflag(Flags);
	retval = 0; // _PassThruConnect(DeviceID, ProtocolID, Flags, Baudrate, pChannelID);
	*pChannelID =  ChannelId;
	if (pChannelID == NULL)
		dtDebug(_T("  pChannelID was NULL\n"));
	else
		dtDebug(_T("  returning ChannelID: %ld\n"), *pChannelID);

	dbug_printretval(retval);
	ChannelId++;
	return retval;
}

extern "C" long J2534_API PassThruDisconnect(unsigned long ChannelID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;
	long retval;

	dtDebug(_T("%.3fs -- PTDisconnect(%ld)\n"), GetTimeSinceInit(), ChannelID);
	retval = 0; // = _PassThruDisconnect(ChannelID);

	dbug_printretval(retval);

	return retval;
}

int char2int(char input)
{
	if (input >= '0' && input <= '9')
		return input - '0';
	if (input >= 'A' && input <= 'F')
		return input - 'A' + 10;
	if (input >= 'a' && input <= 'f')
		return input - 'a' + 10;
	throw std::invalid_argument("Invalid input string");
}
vector<string> split(string str, string token) {
	vector<string>result;
	while (str.size()) {
		int index = str.find(token);
		if (index != string::npos) {
			result.push_back(str.substr(0, index));
			str = str.substr(index + token.size());
			if (str.size() == 0)result.push_back(str);
		}
		else {
			result.push_back(str);
			str = "";
		}
	}
	return result;
}

void AddResponse(string Rmessage)
{
	if (Rmessage.find("[") != std::string::npos)
	{
		string messageEnd;
		int e = Rmessage.find('[');
		if (e > 1)
		{
			messageEnd = Rmessage.substr(e, Rmessage.size() - e);
			Rmessage = Rmessage.substr(0, e);
		}
		vector<string> strs = split(messageEnd, "[");
		for (int i = 0;i < strs.size();i++)
		{
			string posStr = strs.at(i);
			if (!posStr.empty())
			{
				if (posStr.find("]") != std::string::npos)
				{
					posStr = posStr.substr(0, posStr.size() - 1);
				}
				int pos = atoi(posStr.c_str());
				Rmessage += message.substr(pos * 2, 2);
			}
		}
	}
	responses.push(Rmessage);
}
/// Get a time stamp in microseconds.
uint64_t micros()
{
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
	return us;
}
uint64_t millis()
{
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	uint64_t ms = SEC_TO_MS((uint64_t)ts.tv_sec) + NS_TO_MS((uint64_t)ts.tv_nsec);
	return ms;
}

/// Get a time stamp in nanoseconds.
uint64_t nanos()
{
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	uint64_t ns = SEC_TO_NS((uint64_t)ts.tv_sec) + (uint64_t)ts.tv_nsec;
	return ns;
}

extern "C" long J2534_API PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG * pMsg, unsigned long* pNumMsgs, unsigned long Timeout)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;
	long retval;
	unsigned long reqNumMsgs;
	reqNumMsgs = 0;
	long long currentT = millis();
	if (pNumMsgs != NULL) reqNumMsgs = *pNumMsgs;
	retval = 0; // _PassThruReadMsgs(ChannelID, pMsg, pNumMsgs, Timeout);
	//pMsg->ProtocolID = 0x05;
	pMsg->ProtocolID = proto;
/*
	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;
	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;
	*/
	//pMsg->Timestamp = GetTickCount();
	pMsg->Timestamp =micros();

	if (Echo2) // && (responses.size() == 0)
	{
		dtDebug(_T("%.3fs << PTReadMsgs(Channel #%ld,  %1d message, %ldms timeout)\n"), GetTimeSinceInit(), ChannelID, *pNumMsgs, Timeout);
		//string Smessagebyte;
		char Smessagechar[9000]; // = Rmessage;
		AppendLog(message);
		strcpy(Smessagechar, message.c_str());
		pMsg->DataSize = message.length() / 2;
		if ((message.length() / 2) > 1)
		{
			pMsg->RxStatus = 0x1;
			for (int i = 0; i < message.length(); i = i + 2)
			{
				pMsg->Data[i / 2] = char2int(Smessagechar[i]) * 16 + char2int(Smessagechar[i + 1]); //Int32ToByte(Smessagebyte, 16);
			}
			dtDebug(_T("  read %ld of %ld messages\n"), *pNumMsgs, reqNumMsgs);
			dbug_printmsg(pMsg, _T("Msg"), pNumMsgs, FALSE);
			dbug_printretval(retval);
		}
		Echo2 = false;
		return 0;   // No error
	}
	if (!timedMsgs.empty())
	{
		
		for (int i = timedMsgs.size() - 1; i >= 0;i--)
		{
			if (currentT > timedMsgs[i].SendTime)
			{
				AddResponse(timedMsgs[i].Msg);
				timedMsgs.erase(timedMsgs.begin() + i);
			}
		}
	}
	// ok so look at total num of responses or responses[0] for null?
	if (!responses.empty())
	 {
	    string Rmessage = "";
	    string line;
		pMsg->RxStatus = 0;
	
		dtDebug(_T("%.3fs << PTReadMsgs(Channel #%ld,  %1d message, %ldms timeout)\n"), GetTimeSinceInit(), ChannelID, *pNumMsgs, Timeout);
		message = "";
		Rmessage = responses.front();
		responses.pop();
		AppendLog(Rmessage);
	
		string Rmessagebyte;
		char Rmessagechar[100]; // = Rmessage;
		strcpy(Rmessagechar, Rmessage.c_str());
		pMsg->DataSize = Rmessage.length() / 2;
		for (int i = 0; i < Rmessage.length(); i = i + 2)
		{
			pMsg->Data[i / 2] = char2int(Rmessagechar[i]) * 16 + char2int(Rmessagechar[i + 1]); //Int32ToByte(Rmessagebyte, 16);
		}
		if (pNumMsgs != NULL) dtDebug(_T("  read %ld of %ld messages\n"), *pNumMsgs, reqNumMsgs);
		dbug_printmsg(pMsg, _T("Msg"), pNumMsgs, FALSE);

		dbug_printretval(retval);
		pMsg->RxStatus = 0;
		return retval;
	 } // end of returning message from responses array 
	 else 
	 {
		 pMsg->RxStatus = 1;
		 pMsg->DataSize = 0;
		 
		 pMsg->TxFlags = 0;
		return 0x10; // 0x10; // empty buffer, nothing to read

	 }
}



//Send message with delay
//Example: 6c fe f0 35 7f 78:10:1000:5+1; 6c fe 10 76 36
//send first reply 10 times in 1000ms interval and will increase byte 5, +1 each time, than send second message.
void AddTimedResponse(string Rmessage, UINT32 delay)
{
	if (Rmessage.find("[") != std::string::npos)
	{
		
		string messageEnd;
		int e = Rmessage.find('[');
		if (e > 1)
		{
			messageEnd = Rmessage.substr(e, Rmessage.size() - e);
			Rmessage = Rmessage.substr(0, e);
		}
		vector<string> strs = split(messageEnd, "[");
		for (int i = 0;i < strs.size();i++)
		{
			string posStr = strs.at(i);
			if (!posStr.empty())
			{
				if (posStr.find("]") != std::string::npos)
				{
					posStr = posStr.substr(0, posStr.size() - 1);
				}
				int pos = atoi(posStr.c_str());
				Rmessage += message.substr(pos * 2, 2);
			}
		}
	}
	TimedMsg tMsg;
	tMsg.Msg = Rmessage;
	tMsg.SendTime = millis() + delay;
	timedMsgs.push_back(tMsg);
}

void AddResponses(string Rmessage)
{
	if (Rmessage.find(":") != std::string::npos)
	{
		//Timed msg
		const char* hex = "0123456789ABCDEF";

		vector<string> strs = split(Rmessage, ";");
		long long delay = 0;
		long long nextdelay = 0;
		int count = 1;
		int increaseByte = -1;
		int increase = 0;

		for (int i = 0;i < strs.size();i++)
		{
			string msg = strs.at(i);
			if (msg.find(":") != std::string::npos)
			{
				vector<string> tStrs = split(msg, ":");
				string  timemsg = tStrs.at(0); //Message without timing part
				if (tStrs.size() > 1)
				{
					count = stoi(tStrs.at(1));
					delay = stoi(tStrs.at(2));

					if (tStrs.size() > 3)
					{
						string tPart = tStrs[3];
						if (tPart.find("+") != std::string::npos)
						{
							vector<string> rStrs = split(tPart, "+");
							increaseByte = stoi(rStrs.at(0));
							increase = stoi(rStrs.at(1));

							char Rmessagechar[100]; // = Rmessage;
							strcpy(Rmessagechar, timemsg.c_str());
							byte b = char2int(Rmessagechar[increaseByte * 2]) * 16 + char2int(Rmessagechar[increaseByte * 2 + 1]);
							for (int ib = 0;ib < count;ib++)
							{
								Rmessagechar[increaseByte * 2] = hex[(b >> 4) & 0x0f];
								Rmessagechar[increaseByte * 2 + 1] = hex[b & 0x0f];
								nextdelay += delay;
								AddTimedResponse(string(Rmessagechar), nextdelay);
								b += increase;
							}
						}
					}
					else
					{
						for (int ib = 0;ib < count;ib++)
						{
							nextdelay += delay;
							AddTimedResponse(timemsg, nextdelay);
						}
					}
				}
			}
			else
			{
				nextdelay += delay;
				AddTimedResponse(msg, nextdelay);
			}
		}
	}
	else if (Rmessage.find(";") != std::string::npos)
	{
		vector<string> strs = split(Rmessage, ";");
		for (int i = 0;i < strs.size();i++)
		{
			AddResponse(strs.at(i));
		}
	}
	else
	{
		AddResponse(Rmessage);
	}

}

void Findresponses() 
{
	string Rmessage = "";
	string line;
	int row = 0;
	BOOL matchFound = false;

	ReadTestFile();

	if (!msgKeys.empty())
	{
		for (int l=0; l<msgKeys.size();l++)
		{
			row++;
			line = msgKeys.at(l);
			// cout << line << '\n';
			if (message == line)
			{
				cout << message << " Matched with " << line << '\n';
				dtDebug(_T("      Found match in line %d \n"), row);
				matchFound = true;
				Rmessage = msgValues.at(l);
				AddResponses(Rmessage);
				break;
			}
		}
		if (!matchFound)
		{
			row = 0;
			for (int l = 0; l < msgKeys.size();l++)
			{
				row++;
				line = msgKeys.at(l);
				string tmpMsg;
				tmpMsg.assign(message);
				if (line.find("*") != std::string::npos)
				{
					int e = line.find('*');
					if (e > 1)
						line = line.substr(0, e); // remove *
					if (message.length() > line.length())
						tmpMsg = message.substr(0, line.length());
				}
				if (tmpMsg == line)
				{
					cout << message << " Matched with " << line << '\n';
					dtDebug(_T("      Found wildcard match in line %d \n"), row);
					Rmessage = msgValues.at(l);
					AddResponses(Rmessage);
					break;
					//	cout << "Sending " << Rmessage << '\n';
				}
			}
		}
		//  totalnumresponses = numofresponse; // if tnr = 0 then there were no matches? if tnr = 1 then there was 1 response
	}
}


extern "C" long J2534_API PassThruWriteMsgs(unsigned long ChannelID, PASSTHRU_MSG * pMsg, unsigned long* pNumMsgs, unsigned long Timeout)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;
	long retval;
	unsigned long reqNumMsgs = *pNumMsgs;
	string Numofmsgs = "Number of messages is:" + std::to_string(reqNumMsgs);
	const char* hex = "0123456789ABCDEF";

//	cout << Numofmsgs << '\n';
	shim_clearInternalError();
	 // dtDebug(_T("%.3fs >> PTWriteMsgs(%ld, 0x%08X, 0x%08X, %ld)\n"), GetTimeSinceInit(), ChannelID, pMsg, pNumMsgs, Timeout);
     dtDebug(_T("%.3fs >> PTWriteMsgs(Channel #%ld,  %1d message(s), %ldms timeout)\n"), GetTimeSinceInit(), ChannelID, *pNumMsgs, Timeout);
	if (pNumMsgs != NULL)
		reqNumMsgs = *pNumMsgs;
	dbug_printmsg(pMsg, _T("Msg"), pNumMsgs, true);
	retval = 0; // = _PassThruWriteMsgs(ChannelID, pMsg, pNumMsgs, Timeout);
	if (pNumMsgs != NULL)
		dtDebug(_T("  sent %ld of %ld messages\n"), *pNumMsgs, reqNumMsgs);

	dbug_printretval(retval);
	
	// cout << pMsg->DataSize << " bytes were sent to the ecu" << '\n';
			
//	char hexbyte;
	message = "";
    /*	for (int i = 0; i < pMsg->DataSize; ++i)
	{
		messagebyte = pMsg->Data[i];
		message1 += hex[(messagebyte >> 4) & 0xF];
		message1 += hex[(messagebyte++) & 0xF];
		// message1 += " ";
		
	}
	// cout << "Message sent was : " << message1 << '\n';
	*/
	for (int i = 0; i < pMsg->DataSize; ++i)
	{
		message += hex[(pMsg->Data[i] >> 4) & 0x0f];
		message += hex[(pMsg->Data[i]) & 0x0f];
		// message += " ";
	}
	AppendLog(message);
	if (message.size() >= 12)
	{
		string msgStart = message.substr(0, 12);
		if (msgStart == "000007E0AA01")
		{
			proto = 5;
		}
		else
		{
			proto = pMsg->ProtocolID;
		}
	}
	else
	{
		proto = pMsg->ProtocolID;
	}
	// cout << "Message sent was : " << message << '\n';
	if (Echo) //&& responses.empty()
	{
		Echo2 = true;
	}
	Findresponses();
	return retval;
}





extern "C" long J2534_API PassThruStartPeriodicMsg(unsigned long ChannelID, PASSTHRU_MSG *pMsg,
                      unsigned long *pMsgID, unsigned long TimeInterval)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;
	long retval;

	shim_clearInternalError();
	dtDebug(_T("%.3fs ++ PTStartPeriodicMsg(%ld, 0x%08X, 0x%08X, %ld)\n"), GetTimeSinceInit(), ChannelID, pMsg, pMsgID, TimeInterval);
//	SHIM_CHECK_DLL();
// SHIM_CHECK_FUNCTION(_PassThruStartPeriodicMsg);
	
	dbug_printmsg(pMsg, _T("Msg"), 1, true);
	retval = 0; //  _PassThruStartPeriodicMsg(ChannelID, pMsg, pMsgID, TimeInterval);
	if (pMsgID != NULL)
		dtDebug(_T("  returning PeriodicID: %ld\n"), *pMsgID);

	dbug_printretval(retval);
	return retval;
}

extern "C" long J2534_API PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;
	long retval;

	shim_clearInternalError();
	dtDebug(_T("%.3fs -- PTStopPeriodicMsg(%ld, %ld)\n"), GetTimeSinceInit(), ChannelID, MsgID);
//	SHIM_CHECK_DLL();
//	SHIM_CHECK_FUNCTION(_PassThruStopPeriodicMsg);

	retval = 0; //  _PassThruStopPeriodicMsg(ChannelID, MsgID);

	dbug_printretval(retval);
	return retval;
}

extern "C" long J2534_API PassThruStartMsgFilter(unsigned long ChannelID,
                      unsigned long FilterType, PASSTHRU_MSG *pMaskMsg, PASSTHRU_MSG *pPatternMsg,
					  PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;
	long retval;

	shim_clearInternalError();
	dtDebug(_T("%.3fs ++ PTStartMsgFilter(%ld, %s, 0x%08X, 0x%08X, 0x%08X, 0x%08X)\n"), GetTimeSinceInit(), ChannelID, dbug_filter(FilterType).c_str(),
		pMaskMsg, pPatternMsg, pFlowControlMsg, pMsgID);
//	SHIM_CHECK_DLL();
//	SHIM_CHECK_FUNCTION(_PassThruStartMsgFilter);

	dbug_printmsg(pMaskMsg, _T("Mask"), 1, true);
	dbug_printmsg(pPatternMsg, _T("Pattern"), 1, true);
	dbug_printmsg(pFlowControlMsg, _T("FlowControl"), 1, true);
	retval = 0; // _PassThruStartMsgFilter(ChannelID, FilterType, pMaskMsg, pPatternMsg, pFlowControlMsg, pMsgID);
	*pMsgID = 1;
	if (pMsgID != NULL)
		dtDebug(_T("  returning FilterID: %ld\n"), *pMsgID);

	dbug_printretval(retval);
	return retval;
}

extern "C" long J2534_API PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;
	long retval;

	shim_clearInternalError();
	dtDebug(_T("%.3fs -- PTStopMsgFilter(%ld, %ld)\n"), GetTimeSinceInit(), ChannelID, MsgID);
//	SHIM_CHECK_DLL();
//	SHIM_CHECK_FUNCTION(_PassThruStopMsgFilter);
	MsgID = 1;
	retval = 0; //  _PassThruStopMsgFilter(ChannelID, MsgID);

	dbug_printretval(retval);
	return retval;
}

extern "C" long J2534_API PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long Pin, unsigned long Voltage)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;
	long retval;

	shim_clearInternalError();
	dtDebug(_T("%.3fs ** PTSetProgrammingVoltage(%ld, %ld, %ld)\n"), GetTimeSinceInit(), DeviceID, Pin, Voltage);
//	SHIM_CHECK_DLL();
//	SHIM_CHECK_FUNCTION(_PassThruSetProgrammingVoltage);

	switch (Voltage)
	{
	case VOLTAGE_OFF:
		dtDebug(_T("  Pin %ld remove voltage\n"), Pin);
		message = "Voltage" + std::to_string(Pin) + "OFF";
		Findresponses();
		break;
	case SHORT_TO_GROUND:
		dtDebug(_T("  Pin %ld short to ground\n"), Pin);
		message = "Voltage" + std::to_string(Pin) + "GND";
		Findresponses();
		break;
	default:
		dtDebug(_T("  Pin %ld at %f Volts\n"), Pin, Voltage / (float) 1000);
		message = "Voltage" + std::to_string(Pin) + "ON";
		Findresponses();
		break;
	}
	//retval = _PassThruSetProgrammingVoltage(DeviceID, Pin, Voltage);
	retval = 0;

	dbug_printretval(retval);
	return retval;
}

extern "C" long J2534_API PassThruReadVersion(unsigned long DeviceID, char *pFirmwareVersion, char *pDllVersion, char *pApiVersion)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;
	long retval;

	//string paramValue = "";
	string line;
	ifstream myfile(PathOfDll + "serial.txt");

	sprintf(pFirmwareVersion,"4.04");
	sprintf(pDllVersion, "1.01");
	sprintf(pApiVersion, "1.02");

	if (myfile)
	{

		//	dtDebug(_T(" %T path location unknown\n"), myfile);

		if (myfile.is_open())
		{
			while (getline(myfile, line))
			{
				int c = line.find('=');
				if (c > 1)
				{
					string parameter = line.substr(0, c);
					string paramValue = line.substr(c+1).c_str();
					if (parameter == "FirmwareVersion")
						strncpy_s(pFirmwareVersion,20, paramValue.c_str(),60);
					if (parameter == "DllVersion")
						strncpy_s(pDllVersion, 20, paramValue.c_str(),60);
					if (parameter == "ApiVersion")
						strncpy_s(pApiVersion,20, paramValue.c_str(),60);
				}
			}
			myfile.close();
			//  totalnumresponses = numofresponse; // if tnr = 0 then there were no matches? if tnr = 1 then there was 1 response
		}
	}
	else cout << "No such file";

	shim_clearInternalError();
	dtDebug(_T("%.3fs ** PTReadVersion(Device #%ld, %1dX, %1d, %1d)\n"), GetTimeSinceInit(), DeviceID, *pFirmwareVersion, *pDllVersion, *pApiVersion);
	retval = 0; // _PassThruReadVersion(DeviceID, pFirmwareVersion, pDllVersion, pApiVersion);
	CStringW cstrFirmwareVersion(pFirmwareVersion); //  = 4.04;
	CStringW cstrDllVersion(pDllVersion);
	CStringW cstrApiVersion(pApiVersion);

	dtDebug(_T("  Firmware: %s\n"), cstrFirmwareVersion);
	dtDebug(_T("  DLL:      %s\n"), cstrDllVersion);
	dtDebug(_T("  API:      %s\n"), cstrApiVersion);

	dbug_printretval(retval);
	return retval;
}

long shim_PassThruGetLastError(char *pErrorDescription)
{
	if (shim_hadInternalError())
	{
		if (pErrorDescription == NULL)
			return ERR_NULL_PARAMETER;

		// We'll intercept GetLastError if we're reporting something about the shim
		CStringA cstrInternalLastError((LPCTSTR) shim_getInternalError());
		strncpy_s(pErrorDescription, 80, cstrInternalLastError, _TRUNCATE);
		return STATUS_NOERROR;
	}
	else
	{
		// These macros call shim_setInternalError() which does not work the way
		// this function is documented. They should be replaced with code that
		// prints an error to the debug log and copies the text to pErrorDescription
		// if the pointer is non-NULL
	//	SHIM_CHECK_DLL();
		//SHIM_CHECK_FUNCTION(_PassThruGetLastError);

		return _PassThruGetLastError(pErrorDescription);
	}
}

extern "C" long J2534_API PassThruGetLastError(char *pErrorDescription)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;
	long retval;

	// pErrorDescription returns the text description for an error detected
	// during the last function call (EXCEPT PassThruGetLastError). This
	// function should not modify the last internal error

	dtDebug(_T("%.3fs ** PTGetLastError(0x%08X)\n"), GetTimeSinceInit(), pErrorDescription);

	if (pErrorDescription == NULL)
	{
		dtDebug(_T("  pErrorDescription is NULL\n"));
	}

	retval = 0; // shim_PassThruGetLastError(pErrorDescription);

	if (pErrorDescription != NULL)
	{
#ifdef UNICODE
		CStringW cstrErrorDescriptionW(pErrorDescription);
		dtDebug(_T("  %s\n"), (LPCWSTR) cstrErrorDescriptionW);
#else
		dtDebug(_T("  %s\n"), pErrorDescription);
#endif
	}

	// Log the return value for this function without using dbg_printretval().
	// Even if an error occured inside this function, the error text was not
	// updated to describe the error.
	dtDebug(_T("  %s\n"), dbug_return(retval).c_str());
	return retval;
}

extern "C" long J2534_API PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, void *pInput, void *pOutput)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	auto_lock lock;
	long retval;
	int data = 0;
	SCONFIG_LIST *pList;
	shim_clearInternalError();
	dtDebug(_T("%.3fs ** PTIoctl(%ld, %s, 0x%08X, 0x%08X)\n"), GetTimeSinceInit(), ChannelID, dbug_ioctl(IoctlID).c_str(), pInput, pOutput);
	dtDebug(_T("Ioct1ID is %1d , pInput is %1d , pOutput is %1d  \n"), IoctlID, pInput, pOutput);
//	SHIM_CHECK_DLL();
//	SHIM_CHECK_FUNCTION(_PassThruIoctl);

	// Print any relevant info before making the call
	switch (IoctlID)
	{
	// Do nothing for GET_CONFIG input
	case SET_CONFIG:
		dbug_printsconfig((SCONFIG_LIST *) pInput);
		pList = (SCONFIG_LIST*)pInput;
		//s = sl.ConfigPtr;
		for (int c=0;c < pList->NumOfParams;c++)
		{
			if (pList->ConfigPtr[c].Parameter == LOOPBACK)
			{
				if (pList->ConfigPtr[c].Value == 0)
					Echo = false;
				else
					Echo = true;
			}
		}
		break;
	case READ_VBATT:
		if (pOutput != NULL)
		{
			*(int*)pOutput = 14200;
			dtDebug(_T("  %f Volts\n"), ((*(unsigned long*)pOutput)) / (float)1000);
		}
		break;
	// Do nothing for READ_VBATT input
	case FIVE_BAUD_INIT:
		dbug_printsbyte((SBYTE_ARRAY *) pInput, _T("Input"));
		break;
	case FAST_INIT:
		dbug_printmsg((PASSTHRU_MSG *) pInput, _T("Input"), 1, true);
		break;
	// Do nothing for CLEAR_TX_BUFFER
	// Do nothing for CLEAR_RX_BUFFER
	// Do nothing for CLEAR_PERIODIC_MSGS
	// Do nothing for CLEAR_MSG_FILTERS
	// Do nothing for CLEAR_FUNCT_MSG_LOOKUP_TABLE
	case ADD_TO_FUNCT_MSG_LOOKUP_TABLE:
		dbug_printsbyte((SBYTE_ARRAY *) pInput, _T("Add"));
		break;
	case DELETE_FROM_FUNCT_MSG_LOOKUP_TABLE:
		dbug_printsbyte((SBYTE_ARRAY *) pInput, _T("Delete"));
		break;
	// Do nothing for READ_PROG_VOLTAGE
	}

	retval = 0; // _PassThruIoctl(ChannelID, IoctlID, pInput, pOutput);

	// Print any changed info after making the call
	switch (IoctlID)
	{
	case GET_CONFIG:
		dbug_printsconfig((SCONFIG_LIST *) pInput);
		break;
	// Do nothing for SET_CONFIG
	case READ_VBATT:
		if (pOutput != NULL)
		{
			dtDebug(_T("  %f Volts\n"), ((*(unsigned long*)pOutput)) / (float)1000);
		}
		break;
	case FIVE_BAUD_INIT:
		dbug_printsbyte((SBYTE_ARRAY *) pInput, _T("Output"));
		break;
	case FAST_INIT:
		dbug_printmsg((PASSTHRU_MSG *) pOutput, _T("Input"), 1, false);
		break;
	// Do nothing for CLEAR_TX_BUFFER
	// Do nothing for CLEAR_RX_BUFFER
	// Do nothing for CLEAR_PERIODIC_MSGS
	// Do nothing for CLEAR_MSG_FILTERS
	// Do nothing for CLEAR_FUNCT_MSG_LOOKUP_TABLE
	// Do nothing for ADD_TO_FUNCT_MSG_LOOKUP_TABLE:
	// Do nothing for DELETE_FROM_FUNCT_MSG_LOOKUP_TABLE:
	case READ_PROG_VOLTAGE:
		if (pOutput != NULL)
			dtDebug(_T("  %f Volts\n"), ((*(unsigned long*)pOutput)) / (float) 1000);
		break;
	}

	dbug_printretval(retval);
	return retval;
}
