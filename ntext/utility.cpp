
#include "common.h"

/*
��XP�£��ں˻����ExpKeyedEventInitialization����KeyedEvent����
ÿ�ν�������ʱ�������RtlInitializeCriticalSectionAndSpinCount��
��RtlCriticalSectionLock���г�ʼ���������������˳������NtOpenKeyedEvent��
��KeyedEvent����ľ����

����Vista�Ժ��ϵͳ��KeyedEvent���˸Ľ���NtWaitForKeyedEvent��
NtReleaseKeyedEvent������Ҫ�����ֱ�Ӵ���NULL������Ч����˽�������ʱ��
������Ҫ��KeyedEvent����ľ����

NTSTATUS ExpKeyedEventInitialization()
{
	...
	HANDLE Handle;
	UNICODE_STRING DestinationString;
	//ǰ�������\\KernelObjects\\�����򷵻�STATUS_OBJECT_PATH_SYNTAX_BAD
	RtlInitUnicodeString(&DestinationString,L"\\KernelObjects\\CritSecOutOfMemoryEvent");
	OBJECT_ATTRIBUTES oa;
	oa.Length=0x18;
	oa.RootDirectory=NULL;
	oa.ObjectName=&DestinationString;
	oa.Attributes=0x10;	//OBJ_PERMANENT�������ring3���᷵��STATUS_PRIVILEGE_NOT_HELD
	oa.SecurityDescriptor=NULL;
	oa.SecurityQualityOfService=NULL;
	NTSTATUS Error=ZwCreateKeyedEvent(&Handle,0xF0003,&oa,0);	//EVENT_ALL_ACCESS&(~SYNCHRONIZE)
	if (NT_SUCCESS(Error))
		Error=ZwClose(Handle);	//��������ö���Ĵ��ں����ü����޹���
	return Error;
}*/

HANDLE NTAPI OpenGlobalKeyedEvent()
{
	UNICODE_STRING Name;
	RtlInitUnicodeString(&Name,L"\\KernelObjects\\CritSecOutOfMemoryEvent");
	OBJECT_ATTRIBUTES oa;
	oa.Length=0x18;
	oa.RootDirectory=NULL;
	oa.ObjectName=&Name;
	oa.Attributes=0;
	oa.SecurityDescriptor=NULL;
	oa.SecurityQualityOfService=NULL;
	HANDLE hKeyedEvent=NULL;
	NtOpenKeyedEvent(&hKeyedEvent,0x2000000,&oa);	//MAXIMUM_ALLOWED
	return hKeyedEvent;
}

void NTAPI CloseGlobalKeyedEvent(HANDLE hKeyedEvent)
{
	if (hKeyedEvent!=NULL)
		NtClose(GlobalKeyedEventHandle);
}

PVOID WINAPI FindDllBase(WCHAR* szName)
{
	UNICODE_STRING usName;
	RtlInitUnicodeString(&usName,szName);
	PEB* pPeb=NtCurrentTeb()->ProcessEnvironmentBlock;
	LIST_ENTRY* Head=&pPeb->Ldr->InLoadOrderModuleList;
	LIST_ENTRY* Curr=Head->Flink;
	while (Curr->Flink!=Head)
	{
		LDR_DATA_TABLE_ENTRY* Entry=CONTAINING_RECORD(Curr,LDR_DATA_TABLE_ENTRY,InLoadOrderLinks);
		if (RtlCompareUnicodeString(&Entry->BaseDllName,&usName,TRUE)==0)
			return Entry->DllBase;
		Curr=Curr->Flink;
	}
	return NULL;
}

BOOL NTAPI RtlpWaitCouldDeadlock()
{
	//byte_77F978A8���п�����LdrpShutdownInProgress
	//�����˳�ʱ��������Դ���������٣������ȴ�������ִ���Ľ��
	return *LdrpShutdownInProgress!=0;
}

//ͨ����ʱ����ʱ�˱ܾ���
void NTAPI RtlBackoff(DWORD* pCount)
{
	DWORD nBackCount=*pCount;
	if (nBackCount==0)
	{
		if (NtCurrentTeb()->ProcessEnvironmentBlock->NumberOfProcessors==1)
			return ;
		nBackCount=0x40;
		nBackCount*=2;
	}
	else
	{
		if (nBackCount<0x1FFF)
			nBackCount=nBackCount+nBackCount;
	}
	nBackCount=(__rdtsc()&(nBackCount-1))+nBackCount;
	//Win7ԭ������ò�����������ʡȥ�ֲ�����
	pCount=0;
	while ((DWORD)pCount<nBackCount)
	{
		_mm_pause();
		(DWORD)pCount++;
	}
}

void WINAPI K32SetLastError(DWORD dwErrCode)
{
	if (*g_dwLastErrorToBreakOn!=ERROR_SUCCESS && *g_dwLastErrorToBreakOn==dwErrCode)
		DbgBreakPoint();
	//ԭ�������ж�LastErrorValue!=dwErrCode������û������
	NtCurrentTeb()->LastErrorValue=dwErrCode;
}

DWORD WINAPI BaseSetLastNTError(NTSTATUS NtStatus)
{
	DWORD dwErrCode=RtlNtStatusToDosError(NtStatus);
	//��Win7�ϣ�����NTDLL.RtlSetLastWin32Error
	//XP��Ҳ�����Ƶ�NTDLL.RtlSetLastWin32ErrorAndNtStatusFromNtStatus
	//����û��ʹ��g_dwLastErrorToBreakOn������Kernel32.SetLastError����
	K32SetLastError(dwErrCode);
	return dwErrCode;
}

LARGE_INTEGER* WINAPI BaseFormatTimeOut(LARGE_INTEGER* pTimeOut,DWORD dwMilliseconds)
{
	if (dwMilliseconds==INFINITE)
		return NULL;
	pTimeOut->QuadPart=-10000*dwMilliseconds;
	return pTimeOut;
}



