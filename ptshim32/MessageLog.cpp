#include "stdafx.h"
#include <tchar.h>
#include <varargs.h>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;
//FILE* msgfp;
ofstream msgfp;

void CreateLogFile(LPCTSTR FileName, BOOL append)
{
	//_tfopen_s(&msgfp, FileName, _T("w"));
	if (append)
	{
		msgfp.open(FileName, ios::out | ios::app, _SH_DENYNO);
		msgfp << "\n";
		SYSTEMTIME LocalTime;
		GetLocalTime(&LocalTime);

		CString tmp;
		tmp.Format(_T("%04d-%02d-%02d_%02d-%02d-%02d_%04d"), LocalTime.wYear,
			LocalTime.wMonth, LocalTime.wDay, LocalTime.wHour, LocalTime.wMinute, LocalTime.wSecond,
			LocalTime.wMilliseconds);
		msgfp << CT2A(tmp);
		msgfp << "\n\n";
		msgfp.flush();
	}
	else
	{
		msgfp.open(FileName, ios::out | ios::trunc, _SH_DENYNO);
	}
}

void AppendLog(std::string Txt)
{
	//fprintf(msgfp, "%s\n", Txt.c_str());
	msgfp << Txt;
	msgfp << "\n";
	msgfp.flush();
}

void CloseLog()
{
	if (msgfp.is_open())
	{
		msgfp.flush();
		msgfp.close();
	}
}