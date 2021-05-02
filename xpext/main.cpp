
#include "common.h"
#pragma comment(lib,"E:\\WDK\\lib\\wxp\\i386\\ntdll.lib")

//����ʱ�ǵ�ע��ϵͳdll�汾��ÿ���汾��dllƫ�ƶ���һ��
#define BASE_NT		0x7C920000
#define BASE_K32	0x7C800000

//�����˳�ʱ������ֵ��ΪTRUE����ֹ�˳��ڼ���еĸ����½���Ϊ
BYTE* LdrpShutdownInProgress;
//��RtlCreateTagHeap�����Ķѣ�����������ʱ����
//Win7�У��˱������KernelBaseGlobalData+0x2c��
DWORD* BaseDllTag;
//XP��KeyedEvent��Ҫ�õ���������ڸ��ֿ��ǣ�������һ���µ�
HANDLE GlobalKeyedEventHandle;

void XPEXT_InitDll()
{
	DWORD dwNtBaseNow=(DWORD)FindDllBase(L"ntdll.dll");
	LdrpShutdownInProgress=(BYTE*)(0x7C99B0C4-BASE_NT+dwNtBaseNow);
	DWORD dwK32BaseNow=(DWORD)FindDllBase(L"kernel32.dll");
	BaseDllTag=(DWORD*)(0x7C8856D4-BASE_K32+dwK32BaseNow);
	GlobalKeyedEventHandle=OpenGlobalKeyedEvent();
	RtlpInitSRWLock(NtCurrentTeb()->ProcessEnvironmentBlock);
	RtlpInitConditionVariable(NtCurrentTeb()->ProcessEnvironmentBlock);
}

void XPEXT_UninitDll()
{
	CloseGlobalKeyedEvent(GlobalKeyedEventHandle);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		OutputDebugString(L"xpext loaded: ver 1.0\r\n");
		XPEXT_InitDll();
		break;
	case DLL_PROCESS_DETACH:
		XPEXT_UninitDll();
		OutputDebugString(L"xpext unload: ver 1.0\r\n");
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}