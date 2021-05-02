
#include "common.h"

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

BOOL NTAPI RtlIsAnyDebuggerPresent()
{
	KUSER_SHARED_DATA* pKuserSharedData=(KUSER_SHARED_DATA*)0x7FFE0000;
	return (NtCurrentTeb()->ProcessEnvironmentBlock->BeingDebugged==TRUE) || 
		((pKuserSharedData->KdDebuggerEnabled&3)==3);	
}

int NTAPI RtlpTerminateFailureFilter(NTSTATUS ExceptionCode,EXCEPTION_POINTERS* ms_exc_ptr)
{
	//�������������ϵͳ�����쳣���˳����̣�����̫�����ӣ����ﲻʹ��
	//RtlReportException(ms_exc_ptr->ExceptionRecord,ms_exc_ptr->ContextRecord,0);
	NtTerminateProcess((HANDLE)0xFFFFFFFF,ExceptionCode);
	return EXCEPTION_EXECUTE_HANDLER;
}

void NTAPI RtlReportCriticalFailure(DWORD ExceptionCode,ULONG_PTR ExceptionParam1)
{
	__try
	{
		if (RtlIsAnyDebuggerPresent())
		{
			//DPFLTR_DEFAULT_ID
			DbgPrintEx(0x65,0,"Critical error detected %lx\n",ExceptionCode);
			_asm int 3;
		}
	}
	__except(RtlpTerminateFailureFilter(GetExceptionCode(),GetExceptionInformation()))
	{
		return ;
	}
	EXCEPTION_RECORD ExceptionRecord;
	ExceptionRecord.ExceptionCode=ExceptionCode;
	ExceptionRecord.ExceptionFlags=EXCEPTION_NONCONTINUABLE;
	ExceptionRecord.ExceptionRecord=NULL;
	ExceptionRecord.ExceptionAddress=RtlRaiseException;
	ExceptionRecord.NumberParameters=1;
	ExceptionRecord.ExceptionInformation[0]=ExceptionParam1;
	RtlRaiseException(&ExceptionRecord);
}

/*
Windows XP��
kernel32.SetLastError={BreakPoint}+{LastErrorSet}
kernel32.BaseSetLastNTError=ntdll.RtlNtStatusToDosError+kernel32.SetLastError
sdk.SetLastError=ntdll.RtlRestoreLastWin32Error
ntdll.RtlRestoreLastWin32Error={LastErrorSet}
ntdll.RtlSetLastWin32ErrorAndNtStatusFromNtStatus=ntdll.RtlNtStatusToDosError+{LastErrorSet}
Windows 7��
kernel32.SetLastError=jmp ntdll.RtlRestoreLastWin32Error
kernel32.BaseSetLastNTError=ntdll.RtlNtStatusToDosError+ntdll.RtlSetLastWin32Error
sdk.SetLastError=kernel32.SetLastError
ntdll.RtlSetLastWin32Error=ntdll.RtlRestoreLastWin32Error={BreakPoint}+{LastErrorSet}
ntdll.RtlSetLastWin32ErrorAndNtStatusFromNtStatus=ntdll.RtlNtStatusToDosError+ntdll.RtlSetLastWin32Error

XP��SetLastError�����֣�һ����kernel32�ڲ�ʹ�õ�kernel32.SetLastError�����жϵ㹦��
��һ����sdk�﹩������ʹ�õģ�ʵ����ntdll.RtlRestoreLastWin32Error��û�жϵ㹦��
Win7��Դ�������ͳһ������ʲô����SetLastError����󶼵���ntdll.RtlSetLastWin32Error�����жϵ㹦��

������ģ��Win7����kernel32.SetLastError��Hook�ˣ�������ת���ҵ�xpext.RtlSetLastWin32Error
��������ֻ���޸�kernel32�ڲ��ĵ��ã����������ntdll.RtlRestoreLastWin32Error�ĵ��û���ûЧ��
����һ������������kernel32.SetLastError��ת��ntdll.RtlRestoreLastWin32Error
Ȼ����ntdll.RtlRestoreLastWin32Error��ת��xpext.RtlSetLastWin32Error��ͳһʹ���ҵĺ���
������ϸ���룬����ô���ƶϵ㣬�����˰˱����ò�����û��Ҫ������¸�Hook��Ӱ��ϵͳ�ȶ���
���յľ����ǣ�xpext�ڲ��ĺ���ʹ��xpext.RtlSetLastWin32Error��������������޸ģ�����ԭ���ĵ���
*/

DWORD g_dwLastErrorToBreakOn=0;

void NTAPI RtlSetLastWin32Error(DWORD Win32ErrorCode)
{
	if (g_dwLastErrorToBreakOn!=0 && Win32ErrorCode==g_dwLastErrorToBreakOn)
		_asm int 3;
	TEB* CurrentTeb=NtCurrentTeb();
	if (CurrentTeb->LastErrorValue!=Win32ErrorCode)		//����ж���������
		CurrentTeb->LastErrorValue=Win32ErrorCode;
}

NTSTATUS NTAPI RtlInitAnsiStringEx(PANSI_STRING DestinationString,PCSTR szSourceString)
{
	DestinationString->Length=0;
	DestinationString->MaximumLength=0;
	DestinationString->Buffer=(PCHAR)szSourceString;
	if (szSourceString==NULL)
		return STATUS_SUCCESS;
	int Len=strlen(szSourceString);
	if (Len>65534)
		return STATUS_NAME_TOO_LONG;
	DestinationString->Length=Len;
	DestinationString->MaximumLength=Len+1;
	return STATUS_SUCCESS;
}
