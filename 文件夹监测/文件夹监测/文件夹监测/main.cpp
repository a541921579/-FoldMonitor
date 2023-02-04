#include "FileSystemWatcher.h"
#include <stdio.h>
#include <locale.h>
#include <assert.h>
#include <tchar.h>
#include <conio.h>
#include<vector>
#include <iostream>
#include<tchar.h>
#include"ThreadPool.h"
using namespace std;
typedef HWND(WINAPI *PROCGETCONSOLEWINDOW)();
PROCGETCONSOLEWINDOW GetConsoleWindow;

string sourceDir = "E:\\sda";
string descDir ="E:\\222";
int fileCount = 0;
std::ThreadPool thread_pool{ 1 };

string WCharToMByte(LPCWSTR lpcwszStr)
{
	string str;
	DWORD dwMinSize = 0;
	LPSTR lpszStr = NULL;
	dwMinSize = WideCharToMultiByte(CP_OEMCP, NULL, lpcwszStr, -1, NULL, 0, NULL, FALSE);
	if (0 == dwMinSize)
	{
		return FALSE;
	}
	lpszStr = new char[dwMinSize];
	WideCharToMultiByte(CP_OEMCP, NULL, lpcwszStr, -1, lpszStr, dwMinSize, NULL, FALSE);
	str = lpszStr;
	delete[] lpszStr;
	return str;
}

LPCWSTR stringToLPCWSTR(std::string orig)
{
	wchar_t *wcstring = 0;
	try
	{
		size_t origsize = orig.length() + 1;
		const size_t newsize = 100;
		size_t convertedChars = 0;
		if (orig == "")
		{
			wcstring = (wchar_t *)malloc(0);
			mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);
		}
		else
		{
			wcstring = (wchar_t *)malloc(sizeof(wchar_t)*(orig.length() - 1));
			mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);
		}
	}
	catch (std::exception e)
	{
	}
	return wcstring;
}

void copyFile(string filename)
{
	int nameLen = filename.length();
	string cat = "\\";
	string source;
	source.append(sourceDir).append(cat).append(filename);
	
	string desc;
	desc.append(descDir).append(cat).append(filename);

	Sleep(1000);
	for (int i = 0; i < 100; i++)
	{
		//wprintf_s(L"descName- %ws\n", stringToLPCWSTR(source));
		//wprintf_s(L"descName- %ws\n", stringToLPCWSTR(desc));
		LPCWSTR sourceFile = stringToLPCWSTR(source);
		printf("%s--------------------\n", sourceFile);

		if (!MoveFile(stringToLPCWSTR(source), stringToLPCWSTR(desc)))
		{
			DWORD getlasterror;
			getlasterror = GetLastError();
			printf("%s-------------------->ErrorCode: %d\n", sourceFile, getlasterror);
			if (getlasterror == 2)//系统找不到指定的文件。
			{
				break;
			}
			if (getlasterror == 32)//另一个程序正在使用此文件，进程无法访问。 
			{
				Sleep(1000);
			}	
		}
		else
		{
			printf("Cut Sucess -- %s\n", source.c_str());
			break;
		}

	}
	

}
void __stdcall MyDeal(FileSystemWatcher::ACTION act, LPCWSTR filename, LPVOID lParam)
{
	static FileSystemWatcher::ACTION pre = FileSystemWatcher::ACTION_ERRSTOP;
	switch (act)
	{
	case FileSystemWatcher::ACTION_ADDED:
	{
		wprintf_s(L"find - %s\n", filename);
		
		string findFileName = WCharToMByte(filename);
		//printf("11111111111\n");
		printf_s("find - %s\n", findFileName);
		int pos = findFileName.find("TEMP_");
		if (pos == 0) break;
		//Sleep(1000);
		std::future<void> fg = thread_pool.commitTask(copyFile, findFileName);
		wprintf_s(L"ACTION_ADDED   - %s\n", filename);
		break;
	}
		
	case FileSystemWatcher::ACTION_REMOVED:
		wprintf_s(L"ACTION_REMOVED   - %s\n", filename);
		break;
	case FileSystemWatcher::ACTION_MODIFIED:
		wprintf_s(L"ACTION_MODIFIED  - %s\n", filename);
		break;
	case FileSystemWatcher::ACTION_RENAMED_OLD:
		wprintf_s(L"ACTION_RENAMED_OLD(O) - %s\n", filename);
		break;
	case FileSystemWatcher::ACTION_RENAMED_NEW:
		assert(pre == FileSystemWatcher::ACTION_RENAMED_OLD);
		wprintf_s(L"ACTION_RENAMED_NEW - %s\n", filename);
		break;
	case FileSystemWatcher::ACTION_ERRSTOP:
		wprintf_s(L"ACTION_ERRSTOP - %s\n", filename);
	default:
		wprintf_s(L"---default---%s\n", filename);
		break;
	}
	pre = act;
}


int main()
{

	DWORD dwNotifyFilter =
		FileSystemWatcher::FILTER_FILE_NAME | FileSystemWatcher::FILTER_DIR_NAME;
	printf("请输入被监控文件夹:");
	char temp1[256];
	cin >> temp1;
	sourceDir = temp1;
	int pos = sourceDir.find("YCSP2001\\");
	if (pos > 0)
	{
		sourceDir.replace(pos, 9, "192.168.1.105\\");
		sourceDir.append("\\");

	}
	cout << "sDir:" << sourceDir.c_str() << endl;
	cout << "请输入目标文件夹:";
	char temp2[256];
	cin >> temp2;
	descDir= temp2;
	cout << "dDir:" << descDir.c_str() << endl;

	FileSystemWatcher fsw;
	bool r = fsw.Run(stringToLPCWSTR(sourceDir), false, dwNotifyFilter, &MyDeal, 0);
	
	if (!r) {
		cout << "Error!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
		return -1;
	}

	TCHAR * pLanguage =_tsetlocale(LC_CTYPE, TEXT("chs"));
	if (pLanguage == NULL)
	{
		cout << "SET Language False" << endl;
	}
	cout << "Watch:" << sourceDir.c_str() << endl;
	cout << "Press <q> to quit." << endl;

	HMODULE hKernel32 = GetModuleHandleA("kernel32");
	GetConsoleWindow = (PROCGETCONSOLEWINDOW)GetProcAddress(hKernel32, "GetConsoleWindow");
	//ShowWindow(GetConsoleWindow(), SW_HIDE);
	while (_getch() != 'q');

	fsw.Close(1000);

	return 0;
}