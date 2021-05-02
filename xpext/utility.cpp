
#include "common.h"

//�Զ���ı�������

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

void SetReplaceHook(PVOID TargetAddr,PVOID NewAddr,DWORD* OldData)
{

}

void RecoverReplaceHook()
{

}

void SetFilterHook()
{

}

void RecoverFilterHook()
{

}