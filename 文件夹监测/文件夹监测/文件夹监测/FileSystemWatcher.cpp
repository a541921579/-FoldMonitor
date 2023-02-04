#include "FileSystemWatcher.h"
#include <cassert>

FileSystemWatcher::FileSystemWatcher() : m_hDir(INVALID_HANDLE_VALUE), m_hThread(NULL)
{
	assert(FILTER_FILE_NAME == FILE_NOTIFY_CHANGE_FILE_NAME);
	assert(FILTER_DIR_NAME == FILE_NOTIFY_CHANGE_DIR_NAME);
	assert(FILTER_ATTR_NAME == FILE_NOTIFY_CHANGE_ATTRIBUTES);
	assert(FILTER_SIZE_NAME == FILE_NOTIFY_CHANGE_SIZE);
	assert(FILTER_LAST_WRITE_NAME == FILE_NOTIFY_CHANGE_LAST_WRITE);
	assert(FILTER_LAST_ACCESS_NAME == FILE_NOTIFY_CHANGE_LAST_ACCESS);
	assert(FILTER_CREATION_NAME == FILE_NOTIFY_CHANGE_CREATION);
	assert(FILTER_SECURITY_NAME == FILE_NOTIFY_CHANGE_SECURITY);

	assert(ACTION_ADDED == FILE_ACTION_ADDED);
	assert(ACTION_REMOVED == FILE_ACTION_REMOVED);
	assert(ACTION_MODIFIED == FILE_ACTION_MODIFIED);
	assert(ACTION_RENAMED_OLD == FILE_ACTION_RENAMED_OLD_NAME);
	assert(ACTION_RENAMED_NEW == FILE_ACTION_RENAMED_NEW_NAME);
}

FileSystemWatcher::~FileSystemWatcher()
{
	Close();
}

bool FileSystemWatcher::Run(LPCWSTR dir, bool bWatchSubtree, DWORD dwNotifyFilter, LPDEALFUNCTION dealfun, LPVOID lParam)
{
	Close();
	//wprintf_s(L"dir   - %s\n", dir);

	m_hDir = CreateFile(//dir目录不能以'\'结尾，否则监测不到dir目录被删除，不以\结尾，可以检测到（仅限于空目录时）
		dir,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL
	);
	if (INVALID_HANDLE_VALUE == m_hDir) return false;

	m_bWatchSubtree = bWatchSubtree;
	m_dwNotifyFilter = dwNotifyFilter;
	m_DealFun = dealfun;
	m_DealFunParam = lParam;
	m_bRequestStop = false;

	//DWORD ThreadId;

	std::thread my_thread(&FileSystemWatcher::Routine, this);
	m_hThread = my_thread.native_handle();
	if (NULL == m_hThread)
	{
		CloseHandle(m_hDir);
		m_hDir = INVALID_HANDLE_VALUE;
	}
	my_thread.detach();
	//auto newCallable = std::bind(&FileSystemWatcher::Routine, this);
	//std::future<DWORD> gg = thread_pool.commitTask(newCallable);
	/*m_hThread = CreateThread(NULL, 0, FileSystemWatcher::Routine, NULL, 0, &ThreadId);
	if (NULL == m_hThread)
	{
		CloseHandle(m_hDir);
		m_hDir = INVALID_HANDLE_VALUE;
	}*/
	//return NULL != m_hThread;
	return NULL != m_hThread;
}

void FileSystemWatcher::Close(DWORD dwMilliseconds)
{
	if (NULL != m_hThread)
	{
		m_bRequestStop = true;
		if (WAIT_TIMEOUT == WaitForSingleObject(m_hThread, dwMilliseconds))
			TerminateThread(m_hThread, 0);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	if (INVALID_HANDLE_VALUE != m_hDir)
	{
		CloseHandle(m_hDir);
		m_hDir = INVALID_HANDLE_VALUE;
	}
}

DWORD WINAPI FileSystemWatcher::Routine(LPVOID lParam)
{
	
	FileSystemWatcher& obj = *(FileSystemWatcher*)lParam;
	
	BYTE buf[2 * (sizeof(FILE_NOTIFY_INFORMATION) + 2 * MAX_PATH) + 2];
	FILE_NOTIFY_INFORMATION* pNotify = (FILE_NOTIFY_INFORMATION *)buf;
	DWORD BytesReturned;
	while (!obj.m_bRequestStop)
	{
		if (ReadDirectoryChangesW(obj.m_hDir,
			pNotify,
			sizeof(buf) - 2,
			obj.m_bWatchSubtree,
			obj.m_dwNotifyFilter,
			&BytesReturned,
			NULL,
			NULL)) // 无限等待，应当使用异步方式
		{
			for (FILE_NOTIFY_INFORMATION* p = pNotify; p; )
			{
				WCHAR c = p->FileName[p->FileNameLength / 2];
				p->FileName[p->FileNameLength / 2] = L'\0';

				obj.m_DealFun((ACTION)p->Action, p->FileName, obj.m_DealFunParam);

				p->FileName[p->FileNameLength / 2] = c;

				if (p->NextEntryOffset)
					p = (PFILE_NOTIFY_INFORMATION)((BYTE*)p + p->NextEntryOffset);
				else
					p = 0;
			}
		}
		else
		{
			obj.m_DealFun((ACTION)ACTION_ERRSTOP, 0, obj.m_DealFunParam);
			break;
		}
	}

	return 0;
}