
#include "common.h"

typedef unsigned __int64 QWORD;

/*
ϵͳ���������ڵ�ʱ��=ϵͳ���������ڵ�tick��*һ��tick������ʱ��
һ��tick������ʱ����ܲ���������������TickCountMultiplier��Divisor���ϱ�ʾ
���ҵ�XP�ϣ�TickCountMultiplierΪ0x0FA00000��Divisor��Ϊ�̶���0x01000000����15.625ms
���GetTickCount�ķ���ֵ����15.625�ı����������MSDN��˵������ԭ��

GetTickCount���صĺ���������Ϊ0xFFFFFFFF��Լ����49.7�죬��ʵ�ʵļ��������Զ�һЩ
��ÿ��tick����15.625����Ϊ��������������776.7�죬�ӽ�36λ�������ֵ��__int64���أ�����������������
ʵ���ϣ�һ��tick������ʱ����������8λ��MultiplierΪ32λ��DivisorΪ24λ��
����Windows��һ��tick�ĳ���ʱ������������˵���磬������������ܳ��֣����������ʱ��������40λ

������ˣ��һ���ûѡ�����������������xpextk.sys��ģ��Win7������������������KUserSharedData::TickCount

DWORD WINAPI GetTickCount_XP()
{
	KUSER_SHARED_DATA* KUserSharedData=(KUSER_SHARED_DATA*)0x7FFE0000;
	QWORD TickCount=KUserSharedData->TickCountLow*KUserSharedData->TickCountMultiplier;
	return (DWORD)(TickCount>>24);
}

DWORD WINAPI GetTickCount_Win7x32()
{
	KUSER_SHARED_DATA* KUserSharedData=(KUSER_SHARED_DATA*)0x7FFE0000;
	while (KUserSharedData->TickCount.High1Time!=KUserSharedData->TickCount.High2Time)		//ͬ��
		_mm_pause();
	QWORD LowPart=(KUserSharedData->TickCount.LowPart*KUserSharedData->TickCountMultiplier)>>24;
	//High1Time�Ǹ�32λ������ǰӦ��������32λ�����Ǻ���tickת��λҪ����24λ����������������8λ
	//DWORD HighPart=(KUserSharedData->TickCount.High1Time<<32)*KUserSharedData->TickCountMultiplier>>24;
	DWORD HighPart=(KUserSharedData->TickCount.High1Time<<8)*KUserSharedData->TickCountMultiplier;
	return (DWORD)LowPart+HighPart;
}

ULONGLONG WINAPI GetTickCount64_Win7x32()
{
	KUSER_SHARED_DATA* KUserSharedData=(KUSER_SHARED_DATA*)0x7FFE0000;
	while (KUserSharedData->TickCount.High1Time!=KUserSharedData->TickCount.High2Time)
		_mm_pause();
	QWORD LowPart=(KUserSharedData->TickCount.LowPart*KUserSharedData->TickCountMultiplier)>>24;
	QWORD HighPart=KUserSharedData->TickCount.High1Time*KUserSharedData->TickCountMultiplier;
	HighPart=HighPart*0x100;	//��0x100�ȼ�������8λ��ԭ��������ntdll._allmul()
	return HighPart+LowPart;
}

ULONGLONG WINAPI GetTickCount64_Win7x64()
{
	KUSER_SHARED_DATA* KUserSharedData=(KUSER_SHARED_DATA*)0x7FFE0000;
	QWORD TickCount=*(QWORD*)&KUserSharedData->TickCount;
	QWORD HighPart=(TickCount>>32)*KUserSharedData->TickCountMultiplier<<8;
	QWORD LowPart=(DWORD)TickCount*KUserSharedData->TickCountMultiplier>>24;
	return HighPart+LowPart;
}
*/

//��Ҫxpextk.sys֧��
ULONGLONG WINAPI K32GetTickCount64()
{
	KUSER_SHARED_DATA* KUserSharedData=(KUSER_SHARED_DATA*)0x7FFE0000;
	QWORD LowPart=(KUserSharedData->TickCount.LowPart*KUserSharedData->TickCountMultiplier)>>24;
	QWORD HighPart=KUserSharedData->TickCount.High1Time*KUserSharedData->TickCountMultiplier<<8;
	return LowPart+HighPart;
}

//GetErrorMode=0x7C80ACDD
//�����Ϳ�����

VOID WINAPI K32RaiseFailFastException(PEXCEPTION_RECORD pExceptionRecord,PCONTEXT pContextRecord,DWORD dwFlags)
{
	EXCEPTION_RECORD ExceptionRecord;
	CONTEXT ContextRecord;
	DWORD MessageBoxResult;
	DWORD ReturnAddress;
	//����1��ջ�ϵĵ�ַ��ebp+8�����λ����-4���Ǻ������ص�ַ��ջ�е�λ��
	ReturnAddress=*((DWORD*)&pExceptionRecord-1);

	if (pExceptionRecord==NULL)
	{
		memset(&ExceptionRecord,0,sizeof(EXCEPTION_RECORD));	//sizeof(EXCEPTION_RECORD)==80
		ExceptionRecord.ExceptionCode=0xC0000602;	//STATUS_FAIL_FAST_EXCEPTION
		ExceptionRecord.ExceptionFlags=EXCEPTION_NONCONTINUABLE;
		ExceptionRecord.ExceptionAddress=(PVOID)ReturnAddress;		//[ebp+4]
		pExceptionRecord=&ExceptionRecord;
	}
	else
	{
		pExceptionRecord->ExceptionFlags|=EXCEPTION_NONCONTINUABLE;
		if (dwFlags&FAIL_FAST_GENERATE_EXCEPTION_ADDRESS)
			pExceptionRecord->ExceptionAddress=(PVOID)ReturnAddress;
	}

	if (pContextRecord==NULL)
	{
		memset(&ContextRecord,0,sizeof(CONTEXT));	//sizeof(CONTEXT)==0x2CC
		RtlCaptureContext(&ContextRecord);
		pContextRecord=&ContextRecord;
	}

	/*
	//�������������EtwEventWriteNoRegistration��GUID��{E46EEAD8-0C54-4489-9898-8FA79D059E0E}
	NTSTATUS Result=SignalStartWerSvc();
	if (NT_SUCCESS(Result))
	{
		//�������������NtWaitForSingleObject��Event��"\\KernelObjects\\SystemErrorPortReady"
		Result=WaitForWerSvc();
		if (NT_SUCCESS(Result) && Result!=STATUS_TIMEOUT)
		{
			if (*(BYTE*)0x7FFE02F0==1)	//KUSER_SHARED_DATA::DbgErrorPortPresent
			{
				NtRaiseException(pExceptionRecord,pContextRecord,FALSE);
				return ;
			}
		}
	}
	*/

	if (!(dwFlags&FAIL_FAST_NO_HARD_ERROR_DLG))
		NtRaiseHardError(pExceptionRecord->ExceptionCode|0x10000000,0,0,0,1,&MessageBoxResult);
	TerminateProcess(GetCurrentProcess(),pExceptionRecord->ExceptionCode);
}


DWORD WINAPI BaseSetLastNTError(NTSTATUS NtStatus)
{
	//xpext�����к���ʹ��xpext.BaseSetLastNTError
	//���kernel32.BaseSetLastNTError������ת����Status��ȫ
	DWORD dwWin32Error=RtlNtStatusToDosError(NtStatus);
	//XP�ڴ˴�ʹ��KERNEL32.SetLastError����Win7ʹ��NTDLL.RtlSetLastWin32Error
	//ԭ��μ�xpext.RtlSetLastWin32Error������ͳһʹ��xpext.RtlSetLastWin32Error
	RtlSetLastWin32Error(NtStatus);
	return dwWin32Error;
}

LARGE_INTEGER* WINAPI BaseFormatTimeOut(LARGE_INTEGER* pTimeOut,DWORD dwMilliseconds)
{
	if (dwMilliseconds==INFINITE)
		return NULL;
	pTimeOut->QuadPart=-10000*dwMilliseconds;
	return pTimeOut;
}