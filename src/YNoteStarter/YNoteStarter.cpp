#include "YNoteStarter.h"
#include <windows.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <string>
using std::wstring;

DWORD StartProcess(const wstring &app_name, const wstring &cmd)
{
	STARTUPINFO start_info = { sizeof(start_info) };
	PROCESS_INFORMATION process_info = { 0 };
	if (!CreateProcess(app_name.c_str(), (LPWSTR)cmd.c_str(), NULL, NULL, FALSE, NULL, NULL, NULL, &start_info, &process_info))
		return 0;
	WaitForInputIdle(process_info.hProcess, INFINITE);
	CloseHandle(process_info.hThread);
	CloseHandle(process_info.hProcess);
	return process_info.dwProcessId;
}

bool InjectModule(DWORD process_id, const wstring &module_name)
{
	//��ȡҪע���ģ�����·��
	wchar_t self_path[MAX_PATH + 1] = { 0 };
	GetModuleFileName(NULL, self_path, MAX_PATH);
	wcsrchr(self_path, L'\\');
	wstring inject_module_path = self_path;
	size_t last_backslash = inject_module_path.rfind(L'\\');
	if (last_backslash == wstring::npos)
		return false;
	inject_module_path = inject_module_path.substr(0, last_backslash + 1);
	inject_module_path += module_name;
	if (_waccess(inject_module_path.c_str(), 0) != 0)
		return false;

	HANDLE cmb_process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);
	if (cmb_process == NULL)
		return false;
	//��Ŀ������з����ڴ沢д���ע��ģ���·��
	int mem_size = (inject_module_path.length() + 1) * sizeof(wchar_t);
	void *module_name_buffer = VirtualAllocEx(cmb_process, NULL, mem_size, MEM_COMMIT, PAGE_READWRITE);
	if (module_name_buffer == NULL)
	{
		CloseHandle(cmb_process);
		return false;
	}
	if (!WriteProcessMemory(cmb_process, module_name_buffer, inject_module_path.c_str(), mem_size, NULL))
	{
		VirtualFreeEx(cmb_process, module_name_buffer, mem_size, MEM_RELEASE);
		CloseHandle(cmb_process);
		return false;
	}

	//����Զ���߳�
	HMODULE kernel_module = GetModuleHandle(L"kernel32.dll");
	LPTHREAD_START_ROUTINE start_function_addr = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(kernel_module, "LoadLibraryW"));
	HANDLE remote_thread = CreateRemoteThread(cmb_process, NULL, 0, start_function_addr, module_name_buffer, 0, NULL);
	if (remote_thread == NULL)
	{
		VirtualFreeEx(cmb_process, module_name_buffer, mem_size, MEM_RELEASE);
		CloseHandle(cmb_process);
		return false;
	}
	WaitForSingleObject(remote_thread, INFINITE);
	CloseHandle(remote_thread);
	VirtualFreeEx(cmb_process, module_name_buffer, mem_size, MEM_RELEASE);
	CloseHandle(cmb_process);
	return true;
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	//������е�������show�����ͻ������������
	DWORD process_id = StartProcess(L"YoudaoNote.exe", L" show");
	if (process_id == 0)
	{
		MessageBox(NULL, L"�޷�����YoudaoNote.exe", L"��ʾ", MB_OK);
		return 1;
	}
	if (!InjectModule(process_id, L"YNotePatch.dll"))
	{
		MessageBox(NULL, L"�޷�ע��YNotePatch.dll", L"��ʾ", MB_OK);
		return 2;
	}
	return 0;
}

