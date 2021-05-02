
#include "nt_def.h"
#include <intrin.h>

#define XPEXT_INTERNAL
#define XPEXT_EXTERNAL
#define XPEXT_UNIMPLEMENT

//main.cpp - init and hardcode
//utility.cpp - custom function
extern HANDLE GlobalKeyedEventHandle;
extern BYTE* LdrpShutdownInProgress;	//pointer
extern DWORD* BaseDllTag;	//pointer

HANDLE NTAPI OpenGlobalKeyedEvent();
void NTAPI CloseGlobalKeyedEvent(HANDLE hKeyedEvent);
PVOID WINAPI FindDllBase(WCHAR* szName);
//void SetReplaceHook();
//void RecoverReplaceHook();
//void SetFilterHook();
//void RecoverFilterHook();


//nt_privilege.cpp
XPEXT_INTERNAL NTSTATUS NTAPI RtlpOpenThreadToken(ACCESS_MASK DesiredAccess,PHANDLE TokenHandle);
XPEXT_EXTERNAL NTSTATUS NTAPI RtlAcquirePrivilege(PULONG Privilege,ULONG NumPriv,ULONG Flags,PVOID *ReturnedState);
XPEXT_EXTERNAL VOID NTAPI RtlReleasePrivilege(PVOID ReturnedState);

//nt_errorcode.cpp
XPEXT_EXTERNAL ULONG NTAPI RtlNtStatusToDosError(NTSTATUS Status);
XPEXT_EXTERNAL ULONG NTAPI RtlNtStatusToDosErrorNoTeb(NTSTATUS StatusCode);

//nt_miscellaneous.cpp
XPEXT_INTERNAL BOOL NTAPI RtlpWaitCouldDeadlock();
XPEXT_INTERNAL void NTAPI RtlBackoff(DWORD* pCount);
XPEXT_INTERNAL BOOL NTAPI RtlIsAnyDebuggerPresent();
XPEXT_INTERNAL int NTAPI RtlpTerminateFailureFilter(NTSTATUS ExceptionCode,EXCEPTION_POINTERS* ms_exc_ptr);
XPEXT_INTERNAL void NTAPI RtlReportCriticalFailure(DWORD ExceptionCode,ULONG_PTR ExceptionParam1);
XPEXT_EXTERNAL void NTAPI RtlSetLastWin32Error(DWORD Win32ErrorCode);
XPEXT_EXTERNAL NTSTATUS NTAPI RtlInitAnsiStringEx(PANSI_STRING DestinationString,PCSTR szSourceString);

//nk_criticalsection.cpp
BOOL WINAPI K32InitializeCriticalSectionEx(LPCRITICAL_SECTION lpCriticalSection,DWORD dwSpinCount,DWORD Flags);

typedef struct _SYNCITEM
{
	_SYNCITEM* back;	//�ϸ�����Ľڵ�
	_SYNCITEM* first;	//��һ������Ľڵ�
	_SYNCITEM* next;	//�¸�����Ľڵ�
	DWORD count;		//�������
	DWORD attr;			//�ڵ�����
	RTL_SRWLOCK* lock;
} SYNCITEM;

typedef size_t SYNCSTATUS;

//M-mask F-flag SYNC-common
#define SYNC_Exclusive	1	//��ǰ�Ƕ�ռ���ڵȴ��������ǹ�����
#define SYNC_Spinning	2	//��ǰ�̼߳������ߣ������������л��Ѻ�
#define SYNC_SharedLock	4	//��������ʹ�ù������ȴ��������Ƕ�ռ��

#define SRWM_FLAG	0x0000000F
#define SRWM_ITEM	0xFFFFFFF0	//64λϵͳӦ�øĳ�0xFFFFFFFFFFFFFFF0
#define SRWM_COUNT	SRWM_ITEM

#define SRWF_Free	0	//����
#define SRWF_Hold	1	//���߳�ӵ������
#define SRWF_Wait	2	//���߳����ڵȴ�
#define SRWF_Link	4	//�޸�����Ĳ���������
#define SRWF_Many	8	//��ռ����֮ǰ�ж������������

#define CVM_COUNT	0x00000007
#define CVM_FLAG	0x0000000F
#define CVM_ITEM	0xFFFFFFF0

#define CVF_Full	7	//��������������ȫ������
#define CVF_Link	8	//�޸�����Ĳ���������

#define SRW_COUNT_BIT	4
#define SRW_HOLD_BIT	0
#define SYNC_SPIN_BIT	1	//��0��ʼ��

//nt_srwlock.cpp
XPEXT_INTERNAL void NTAPI RtlpInitSRWLock(PEB* pPEB);
XPEXT_EXTERNAL void NTAPI RtlInitializeSRWLock(RTL_SRWLOCK* SRWLock);
XPEXT_INTERNAL void NTAPI RtlpWakeSRWLock(RTL_SRWLOCK* SRWLock,SYNCSTATUS OldStatus);
XPEXT_INTERNAL void NTAPI RtlpOptimizeSRWLockList(RTL_SRWLOCK* SRWLock,SYNCSTATUS OldStatus);
XPEXT_EXTERNAL void NTAPI RtlAcquireSRWLockExclusive(RTL_SRWLOCK* SRWLock);
XPEXT_EXTERNAL void NTAPI RtlAcquireSRWLockShared(RTL_SRWLOCK* SRWLock);
XPEXT_EXTERNAL void NTAPI RtlReleaseSRWLockExclusive(RTL_SRWLOCK* SRWLock);
XPEXT_EXTERNAL void NTAPI RtlReleaseSRWLockShared(RTL_SRWLOCK* SRWLock);
XPEXT_EXTERNAL BOOL NTAPI RtlTryAcquireSRWLockExclusive(RTL_SRWLOCK* SRWLock);
XPEXT_EXTERNAL BOOL NTAPI RtlTryAcquireSRWLockShared(RTL_SRWLOCK* SRWLock);

//nk_conditionvariable.cpp
XPEXT_INTERNAL void NTAPI RtlpInitConditionVariable(PEB* pPeb);
XPEXT_EXTERNAL void NTAPI RtlInitializeConditionVariable(RTL_CONDITION_VARIABLE* ConditionVariable);
XPEXT_INTERNAL BOOL NTAPI RtlpQueueWaitBlockToSRWLock(SYNCITEM* Item,RTL_SRWLOCK* SRWLock,BOOL IsSharedLock);
XPEXT_INTERNAL void NTAPI RtlpWakeConditionVariable(RTL_CONDITION_VARIABLE* ConditionVariable,SYNCSTATUS OldStatus,int WakeCount);
XPEXT_INTERNAL BOOL NTAPI RtlpWakeSingle(RTL_CONDITION_VARIABLE* ConditionVariable,SYNCITEM* Item);
XPEXT_INTERNAL void NTAPI RtlpOptimizeConditionVariableWaitList(RTL_CONDITION_VARIABLE* ConditionVariable,SYNCSTATUS OldStatus);
XPEXT_INTERNAL NTSTATUS NTAPI RtlSleepConditionVariableCS(RTL_CONDITION_VARIABLE* ConditionVariable,RTL_CRITICAL_SECTION* CriticalSection,LARGE_INTEGER* Timeout);
XPEXT_INTERNAL NTSTATUS NTAPI RtlSleepConditionVariableSRW(RTL_CONDITION_VARIABLE* ConditionVariable,RTL_SRWLOCK* SRWLock,LARGE_INTEGER* Timeout,ULONG Flags);
XPEXT_EXTERNAL void NTAPI RtlWakeConditionVariable(RTL_CONDITION_VARIABLE* ConditionVariable);
XPEXT_EXTERNAL void NTAPI RtlWakeAllConditionVariable(RTL_CONDITION_VARIABLE* ConditionVariable);
XPEXT_EXTERNAL BOOL WINAPI K32SleepConditionVariableCS(PCONDITION_VARIABLE ConditionVariable,PCRITICAL_SECTION CriticalSection,DWORD dwMilliseconds);
XPEXT_EXTERNAL BOOL WINAPI K32SleepConditionVariableSRW(PCONDITION_VARIABLE ConditionVariable,PSRWLOCK SRWLock,DWORD dwMilliseconds,ULONG Flags);

typedef struct _RUNONCEITEM
{
	_RUNONCEITEM* next;
} RUNONCEITEM;

typedef size_t RUNONCESTATUS;

#define RUNONCEM_ITEM	0xFFFFFFFC
#define RUNONCEM_FLAG	0x00000003

//ע�⣬RUNONCESTATUS���FLAG�Ͳ������Flags��������
//�������Flags�ǲ���ѡ����FLAG��RunOnce�����״̬
#define RUNONCEF_NoRequest	0
#define RUNONCEF_SyncPend	1
#define RUNONCEF_Complete	2
#define RUNONCEF_AsyncPend	3

//nk_runonce.cpp
XPEXT_INTERNAL RUNONCESTATUS NTAPI RtlpRunOnceWaitForInit(RUNONCESTATUS OldStatus,RTL_RUN_ONCE* RunOnce);
XPEXT_INTERNAL void NTAPI RtlpRunOnceWakeAll(RTL_RUN_ONCE* RunOnce);
XPEXT_EXTERNAL void NTAPI RtlRunOnceInitialize2(RTL_RUN_ONCE* RunOnce);
XPEXT_INTERNAL NTSTATUS NTAPI RtlRunOnceBeginInitialize2(RTL_RUN_ONCE* RunOnce,DWORD Flags,PVOID* Context);
XPEXT_INTERNAL NTSTATUS NTAPI RtlRunOnceComplete2(RTL_RUN_ONCE* RunOnce,DWORD Flags,PVOID Context);
XPEXT_INTERNAL NTSTATUS NTAPI RtlRunOnceExecuteOnce2(RTL_RUN_ONCE* RunOnce,RTL_RUN_ONCE_INIT_FN InitFn,PVOID Parameter,PVOID* Context);
XPEXT_EXTERNAL BOOL WINAPI K32InitOnceBeginInitialize(LPINIT_ONCE lpInitOnce,DWORD dwFlags,PBOOL fPending,LPVOID* lpContext);
XPEXT_EXTERNAL BOOL WINAPI K32InitOnceExecuteOnce(LPINIT_ONCE lpInitOnce,PINIT_ONCE_FN InitFn,LPVOID lpParameter,LPVOID* lpContext);
XPEXT_EXTERNAL BOOL WINAPI K32InitOnceComplete(LPINIT_ONCE lpInitOnce,DWORD dwFlags,LPVOID lpContext);

//k32_processthread.cpp
XPEXT_EXTERNAL DWORD WINAPI K32GetThreadId(HANDLE Thread);
XPEXT_EXTERNAL DWORD WINAPI K32GetProcessId(HANDLE Process);
XPEXT_EXTERNAL DWORD WINAPI K32GetProcessIdOfThread(HANDLE Thread);
XPEXT_EXTERNAL void WINAPI K32DeleteProcThreadAttributeList(LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList);
XPEXT_UNIMPLEMENT BOOL WINAPI K32InitializeProcThreadAttributeList(LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,DWORD dwAttributeCount,DWORD dwFlags,PSIZE_T lpSize);
XPEXT_UNIMPLEMENT BOOL WINAPI K32UpdateProcThreadAttribute(LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,DWORD dwFlags,
	DWORD_PTR Attribute,PVOID lpValue,SIZE_T cbSize,PVOID lpPreviousValue,PSIZE_T lpReturnSize);

//k32_processor.cpp
XPEXT_EXTERNAL WORD WINAPI K32GetActiveProcessorGroupCount();
XPEXT_EXTERNAL WORD WINAPI K32GetMaximumProcessorGroupCount();
XPEXT_EXTERNAL DWORD WINAPI K32GetActiveProcessorCount(WORD GroupNumber);
XPEXT_EXTERNAL DWORD WINAPI K32GetMaximumProcessorCount(WORD GroupNumber);
XPEXT_EXTERNAL DWORD WINAPI RtlGetCurrentProcessorNumber();
XPEXT_EXTERNAL void WINAPI RtlGetCurrentProcessorNumberEx(PPROCESSOR_NUMBER ProcNumber);

//k32_miscellaneous.cpp
XPEXT_EXTERNAL ULONGLONG WINAPI K32GetTickCount64();
XPEXT_EXTERNAL VOID WINAPI K32RaiseFailFastException(PEXCEPTION_RECORD pExceptionRecord,PCONTEXT pContextRecord,DWORD dwFlags);
XPEXT_INTERNAL DWORD WINAPI BaseSetLastNTError(NTSTATUS NtStatus);
XPEXT_INTERNAL LARGE_INTEGER* WINAPI BaseFormatTimeOut(LARGE_INTEGER* pTimeOut,DWORD dwMilliseconds);

//k32_file.cpp
//test now
BOOL WINAPI K32GetFileInformationByHandleEx(HANDLE hFile,FILE_INFO_BY_HANDLE_CLASS InformationByHandleClass,LPVOID lpFileInformation,DWORD dwBufferSize);
BOOL WINAPI K32SetFileInformationByHandle(HANDLE hFile,FILE_INFO_BY_HANDLE_CLASS InformationByHandleClass,LPVOID lpFileInformation,DWORD dwBufferSize);
DWORD WINAPI K32GetFinalPathNameByHandleW(HANDLE hFile,LPWSTR lpszFilePath,DWORD cchFilePath,DWORD dwFlags);
DWORD WINAPI K32GetFinalPathNameByHandleA(HANDLE hFile,LPSTR lpszFilePath,DWORD cchFilePath,DWORD dwFlags);
BOOLEAN WINAPI K32CreateSymbolicLinkW(LPCWSTR lpSymlinkFileName,LPCWSTR lpTargetFileName,DWORD dwFlags);
BOOLEAN WINAPI K32CreateSymbolicLinkA(LPCSTR lpSymlinkFileName,LPCSTR lpTargetFileName,DWORD dwFlags);

BOOL WINAPI Basep8BitStringToDynamicUnicodeString(UNICODE_STRING* OutUnicode,LPCSTR InputAnsi);
BOOL WINAPI BasepGetObjectNTName(HANDLE hFile,LPWSTR* pszNameOut);
BOOL WINAPI BasepGetFileNameInformation(HANDLE hFile,FILE_INFORMATION_CLASS FileInformationClass,LPWSTR* pszNameOut);
BOOL WINAPI BasepGetVolumeDosLetterNameFromNTName(WCHAR* NTName,LPWSTR* pszNameOut);
BOOL WINAPI BasepGetVolumeGUIDFromNTName(WCHAR* NTName,LPWSTR* pszNameOut);