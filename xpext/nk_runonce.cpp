
#include "common.h"

/*
Windows 7 SP1 32λ 6.1.7601.17514
@���� 2021.4.18

��kernel32��⼸��������InitOnceXXX
��ntdll��⼸��������RtlRunOnceXXX
��֪��Ϊʲô��winnt.h�ﶨ����RtlRunOnceXXX
��ֻ�ý�����������ΪRtlRunOnceXXX2

����Ϊû��ʮ����Ѫ˨��Ʋ��������ֽӿ�
�Ѽ򵥵����鸴�ӻ��������ѧϰ�ɱ�������һ������
ʹ���⼸��������Ҫ��ϸ�Ķ�MSDN�ϵ��ĵ���ע�����еĸ�������

Ȼ���⼸��������������������α���룺

//ͬ��ģʽ
TYPE* g_obj=NULL;

TYPE* GetObject()
{
	EnterLock();
	if (g_obj==NULL)
		g_obj=CreateObject();
	LeaveLock();
	return g_obj;
}

void UseObject()
{
	TYPE* obj=GetObject();
	obj->DoWork();
}

//�첽ģʽ
TYPE* g_obj=NULL;

TYPE* GetObject()
{
	return g_obj;
}

void UpdateObject()
{
	TYPE* obj=CreateObject();
	EnterLock();
	if (g_obj==NULL)
		g_obj=obj;
	LeaveLock();
	if (g_obj!=obj)
		DestroyObject(obj);
}

void UseObject()
{
	TYPE* obj=GetObject();
	if (obj==NULL)
	{
		UpdateObject();
		obj=GetObject();
	}
	obj->DoWork();
}

�򵥵�˵�����ڶ��̻߳����´���һ����������Singleton Object��
������󲻴��ڣ����½�һ�����˺������̶߳����������
���򣬶����Ѿ��������̴߳�������ʹ���ִ�Ķ���
������˵��������ֻ�Ƿ���������ʵ����Ҳ������ִ��һЩ��ʼ�����룩

��Щ����������ʹ��ģʽ��ͬ��ģʽ���첽ģʽ
ͬ��ģʽ�£���һ�����õ��߳�ִ�д����������̵߳ȴ�
��һ���̴߳�����ϣ������̱߳����ѣ�ȡ���Ѵ����Ķ���
�첽ģʽ�£��������û�д����������߳�һ�𴴽�����
��һ��������ɵ��߳̽�����ָ����£��˺������̶߳�ʹ���������
Ȼ����һ�����߳���Ҫ�����Լ�����һ��Ķ���

����RtlRunOnceExecuteOnce��ͬ��ģʽ��
RtlRunOnceBeginInitialize��RtlRunOnceComplete�ķ�װ
����ʹ�÷�ʽ���ҵ������������죬���Բο�΢�������ʹ��ʾ����
https://docs.microsoft.com/en-us/windows/win32/sync/using-one-time-initialization
*/

RUNONCESTATUS NTAPI RtlpRunOnceWaitForInit(RUNONCESTATUS OldStatus,RTL_RUN_ONCE* RunOnce)
{
	//ջ�ϵ����ݶ��ǰ�4�ֽڶ���ģ�����Ҫ__declspec(align(4))
	//��Ϊitem��ջ�ϣ��Ҵ�СΪ4�ֽڣ����������ɻ��ʱ���˸��Ż�
	//��OldStatus��ֵ����ecx���ջ�ϵ�OldStatus��λ��������item
	RUNONCEITEM item;
	//���Լ��Ľڵ��������������ȴ�
	//ֻ��ͬ��ģʽ��δ��ɲ���Ҫ�ȴ���״̬��ȻΪsync+pend
	RUNONCESTATUS NewStatus=(((RUNONCESTATUS)&item)&(RUNONCEM_ITEM+RUNONCEF_SyncPend))|RUNONCEF_SyncPend;
	do 
	{
		item.next=(RUNONCEITEM*)(OldStatus&RUNONCEM_ITEM);
		RUNONCESTATUS CurrStatus=InterlockedCompareExchange((RUNONCESTATUS*)RunOnce,NewStatus,OldStatus);
		if (CurrStatus==OldStatus)
		{
			//XP��֧�ֵ�һ����������NULL
			NtWaitForKeyedEvent(GlobalKeyedEventHandle,&item,FALSE,NULL);
			CurrStatus=(RUNONCESTATUS)RunOnce->Ptr;
		}
		OldStatus=CurrStatus;
	} while ((OldStatus&RUNONCEM_FLAG)==RUNONCEF_SyncPend);
	return OldStatus;
}

void NTAPI RtlpRunOnceWakeAll(RTL_RUN_ONCE* RunOnce)
{
	//����next���ϵ����нڵ�
	RUNONCEITEM* item=(RUNONCEITEM*)((RUNONCESTATUS)RunOnce->Ptr&RUNONCEM_ITEM);
	if (item!=NULL)
	{
		do 
		{
			//�ݴ�next����ֹ���Ѻ�ʧЧ
			RUNONCEITEM* next=item->next;
			//XP��֧�ֵ�һ����������NULL
			NtReleaseKeyedEvent(GlobalKeyedEventHandle,item,FALSE,NULL);
			item=next;
		} while (item!=NULL);
	}
}

//void WINAPI K32InitOnceInitialize(LPINIT_ONCE InitOnce)
void NTAPI RtlRunOnceInitialize2(RTL_RUN_ONCE* RunOnce)
{
	RunOnce->Ptr=NULL;
}

NTSTATUS NTAPI RtlRunOnceBeginInitialize2(RTL_RUN_ONCE* RunOnce,DWORD Flags,PVOID* Context)
{
	//Flags������INIT_ONCE_CHECK_ONLY��INIT_ONCE_ASYNC
	if ((Flags&~(INIT_ONCE_CHECK_ONLY|INIT_ONCE_ASYNC))!=0)	//0xFFFFFFFC
		return STATUS_INVALID_PARAMETER_2;
	//flagsΪ3ʱ��flags&(flags-1)=3&2=2
	//flagsΪ2ʱ��flags&(flags-1)=2&1=0
	//flagsΪ1ʱ��flags&(flags-1)=1&0=0
	//flagsΪ0ʱ��and��Ϊ0
	//������������Flagͬʱ����
	//��MSDN��˵This parameter can have a value of 0, or one or more of the following flags.
	if (Flags & (Flags-1))
		return STATUS_INVALID_PARAMETER_2;

	RUNONCESTATUS OldStatus=(RUNONCESTATUS)RunOnce->Ptr;
	//ԭ�����ջ�ϵ�Flags��λ�ô洢Result
	NTSTATUS Result=STATUS_SUCCESS;
	//�����߳���ɴ������������е�
	if ((OldStatus&RUNONCEM_FLAG)==RUNONCEF_Complete)
	{
		//��һ��ûִ����Ч�Ĳ������ȼ���nop���𱻹���˼άƭ��
		//��Win7 x64���һ����lock or [rsp+0], 0
		//�Ʋ������ˢ���ڴ滺���һ�ַ�ʽ��
		InterlockedExchange((size_t*)&RunOnce,Flags);
		if (Context!=NULL)
			*(size_t*)Context=OldStatus&RUNONCEM_ITEM;
		return Result;
	}
	//������û��ɣ�ȡ��ʧ��
	if (Flags&RTL_RUN_ONCE_CHECK_ONLY)
		return STATUS_UNSUCCESSFUL;

	BOOL IsSyncMode=((Flags&RTL_RUN_ONCE_ASYNC)==0);

	while (1)
	{
		BYTE StatusFlag=OldStatus&RUNONCEM_FLAG;
		//�Լ��ǵ�һ�����õ��̣߳�����Flags���ú�sync��async������pending��������д���
		if (StatusFlag==RUNONCEF_NoRequest)
		{
			//��ָ����RTL_RUN_ONCE_ASYNC����ΪAsync+Pend������ΪSync+Pend
			//��Щ�����������Ի���Ϊ�����Ǳ���������λ�����ɵ�
			RUNONCESTATUS NewStatus=(((!IsSyncMode)<<1)|RUNONCEF_SyncPend)&RUNONCEM_FLAG;
			RUNONCESTATUS CurrStatus=InterlockedCompareExchange((RUNONCESTATUS*)RunOnce,NewStatus,OldStatus);
			if (CurrStatus==OldStatus)
			{
				Result=STATUS_PENDING;
				return Result;
			}
			OldStatus=CurrStatus;
		}
		//��һϵ�е�����syncģʽ���������߳̽��д������ȴ������
		//�������OldStatus���ܺ��еȴ��Ľڵ�����
		else if (StatusFlag==RUNONCEF_SyncPend)
		{
			//ָ����asyncģʽ��֮ǰ��syncģʽ��ͻ
			if (IsSyncMode==FALSE)
			{
				Result=STATUS_INVALID_PARAMETER_2;
				return Result;
			}
			OldStatus=RtlpRunOnceWaitForInit(OldStatus,RunOnce);
		}
		//��һϵ�е�����asyncģʽ������pending������д���
		else if (StatusFlag==RUNONCEF_AsyncPend)
		{
			//ָ����syncģʽ��֮ǰ��asyncģʽ��ͻ
			if (IsSyncMode)
				Result=STATUS_INVALID_PARAMETER_2;
			else
				Result=STATUS_PENDING;
			return Result;
		}
		//�����߳���ɴ������������е�
		//�������OldStatus����context
		else if (StatusFlag==RUNONCEF_Complete)
		{
			if (Context!=NULL)
				*(size_t*)Context=OldStatus&RUNONCEM_ITEM;
			return Result;
		}
		//��֪��Ϊʲô��ԭ������ʹ��OldStatusȷ��Async+Pend�����
		//if (StatusFlag==RUNONCEF_NoRequest) ...
		//else if (StatusFlag==RUNONCEF_SyncPend) ...
		//else if (OldStatus==RUNONCEF_AsyncPend) ...
		//else ...
		//��Complete�����������OldStatus����context��û��ֱ�ӱȽ�
		//������ж�������3������󣬷�����else��
	}
	return Result;
}

NTSTATUS NTAPI RtlRunOnceComplete2(RTL_RUN_ONCE* RunOnce,DWORD Flags,PVOID Context)
{
	//Flags������RTL_RUN_ONCE_ASYNC��RTL_RUN_ONCE_INIT_FAILED
	if ((Flags&~(RTL_RUN_ONCE_ASYNC|RTL_RUN_ONCE_INIT_FAILED))!=0)	//0xFFFFFFF9
		return STATUS_INVALID_PARAMETER_2;
	//flagsΪ6ʱ��flags&(flags-1)=6&5=4
	//flagsΪ4ʱ��flags&(flags-1)=4&3=0
	//flagsΪ2ʱ��flags&(flags-1)=2&1=0
	//flagsΪ0ʱ��and��Ϊ0
	//������������Flagͬʱ����
	if (Flags & (Flags-1))
		return STATUS_INVALID_PARAMETER_2;

	//ԭ�������������ģ�
	//DWORD NewFlags=((~(Flags>>1))^Flags)&3^Flags;
	//�ṹΪ(target ^ complement) & range ^ complement
	//��Ϊ��range��Ϊ1��λΪ׼������target�ж�Ӧ��λ��
	//ʣ�µ�λ��complement�ж�Ӧ��λ���
	//���ں���Ĵ���ֻ�õ����2λ�����Եȼ������⼸�д���
	//����˵���ֿ���������������ô������������ƽӿڵ������Ӳ�����
	//д�������Ҳ�Ǹ���̱������˵���Ǹ�������һ���ˣ�
	BOOL IsSuccess=!(Flags&RTL_RUN_ONCE_INIT_FAILED);
	BOOL IsSyncMode=!(Flags&RTL_RUN_ONCE_ASYNC);
	DWORD NewFlags=(IsSuccess<<1)|IsSyncMode;

	if (Context!=NULL)
	{
		//ʧ��ģʽ����������Context
		if ((NewFlags & 2)==0)
			return STATUS_INVALID_PARAMETER_3;
		//Context����DWORD���루��ճ����RTL_RUN_ONCE_CTX_RESERVED_BITSλ��
		if (((size_t)Context & 3)!=0)
			return STATUS_INVALID_PARAMETER_3;
	}

	RUNONCESTATUS OldStatus=(RUNONCESTATUS)RunOnce->Ptr;
	//����ʧ��ģʽ��NewFlags & 2Ϊ0��ContextҲΪ0���ϳ�NoRequest״̬
	//���ǳɹ�ģʽ��NewFlags & 2Ϊ1��Context�ǽ�����ϳ�Complete״̬
	RUNONCESTATUS NewStatus=(NewFlags & 2) | (size_t)Context;

	BYTE StatusFlag=OldStatus&RUNONCEM_FLAG;
	if (StatusFlag==RUNONCEF_SyncPend)
	{
		//ָ����asyncģʽ��֮ǰ��syncģʽ��ͻ
		if ((NewFlags & 1)==0)
			return STATUS_INVALID_PARAMETER_2;
		RUNONCESTATUS CurrStatus=InterlockedExchange((RUNONCESTATUS*)RunOnce,NewStatus);
		//syncģʽֻ����һ���̲߳����������߳��޸�״̬�ǳ�������
		if ((CurrStatus&RUNONCEM_FLAG)!=RUNONCEF_SyncPend)
			return STATUS_INVALID_OWNER;
		//����ջ��Flags�Ŀռ���ʱ������һ��RTL_RUN_ONCE
		//����Win7 x64�ϣ�������ֱ��չ��RtlpRunOnceWakeAll��ʡȥ����һ��
		RTL_RUN_ONCE temp;
		temp.Ptr=(PVOID)CurrStatus;
		RtlpRunOnceWakeAll(&temp);
		return STATUS_SUCCESS;
	}
	else if (StatusFlag==RUNONCEF_AsyncPend)
	{
		//ָ����syncģʽ��֮ǰ��asyncģʽ��ͻ
		if ((NewFlags & 1)!=0)
			return STATUS_INVALID_PARAMETER_2;
		RUNONCESTATUS CurrStatus=InterlockedCompareExchange((RUNONCESTATUS*)RunOnce,NewStatus,OldStatus);
		//�����߳��Ѿ��ύ�ɹ���ʹ�ô˽��
		if (CurrStatus!=OldStatus)
			return STATUS_OBJECT_NAME_COLLISION;
		//���߳��ύ�ɹ�
		return STATUS_SUCCESS;
	}
	else
	{
		//��RUNONCEF_NoRequest��˵����ֹ������ֱ���ύ���
		//��RUNONCEF_Complete��˵������Ѿ��ύ����������
		return STATUS_UNSUCCESSFUL;
	}
}

NTSTATUS NTAPI RtlRunOnceExecuteOnce2(RTL_RUN_ONCE* RunOnce,RTL_RUN_ONCE_INIT_FN InitFn,PVOID Parameter,PVOID* Context)
{
	//ԭ�����뽫ջ�ϵ�Context������ֽ�������¼������Ϣ
	BYTE ErrorInfo;
	//������߳��ǵ�һ�����õģ�����STATUS_PENDING��������󴴽�
	//������̲߳��ǵ�һ�����õģ����ں����ڵȴ���ֱ����һ���̴߳������
	NTSTATUS Result=RtlRunOnceBeginInitialize2(RunOnce,0,Context);
	if (NT_SUCCESS(Result))
	{
		//���߳��ǵ�һ���̣߳����ûص������������󣬳ɹ����ύ����ָ��
		if (Result==STATUS_PENDING)
		{
			if (InitFn(RunOnce,Parameter,Context)==TRUE)
			{
				//ԭ������ֱ��ʹ��Context���洢ContextData
				PVOID ContextData=NULL;
				if (Context!=NULL)
					ContextData=*Context;
				//�����ɹ�������2����0����״̬��ΪComplete�������������status��
				Result=RtlRunOnceComplete2(RunOnce,0,ContextData);
				if (NT_SUCCESS(Result))
				{
					Result=STATUS_SUCCESS;
				}
				else
				{
					ErrorInfo=1;
					RtlReportCriticalFailure(Result,(ULONG_PTR)&ErrorInfo);
				}
			}
			else
			{
				//����ʧ�ܣ�����2����RTL_RUN_ONCE_INIT_FAILED
				//������״̬��ΪNoRequest�������������߳�
				Result=RtlRunOnceComplete2(RunOnce,RTL_RUN_ONCE_INIT_FAILED,NULL);
				if (NT_SUCCESS(Result))
				{
					Result=STATUS_UNSUCCESSFUL;
				}
				else
				{
					ErrorInfo=2;
					RtlReportCriticalFailure(Result,(ULONG_PTR)&ErrorInfo);
				}
			}
		}
		else	//Result==STATUS_SUCCESS
		{
			//�����߳��Ѿ��������˶��󣬷���Context�����ֱ��ʹ��
			Result=STATUS_SUCCESS;
		}
	}
	else
	{
		ErrorInfo=0;
		RtlReportCriticalFailure(Result,(ULONG_PTR)&ErrorInfo);
	}
	return Result;
}


BOOL WINAPI K32InitOnceBeginInitialize(LPINIT_ONCE lpInitOnce,DWORD dwFlags,PBOOL fPending,LPVOID* lpContext)
{
	NTSTATUS Result=RtlRunOnceBeginInitialize2(lpInitOnce,dwFlags,lpContext);
	if (!NT_SUCCESS(Result))
	{
		BaseSetLastNTError(Result);
		return FALSE;
	}
	*fPending=(Result==STATUS_PENDING);
	return TRUE;
}

BOOL WINAPI K32InitOnceExecuteOnce(LPINIT_ONCE lpInitOnce,PINIT_ONCE_FN InitFn,LPVOID lpParameter,LPVOID* lpContext)
{
	NTSTATUS Result=RtlRunOnceExecuteOnce2(lpInitOnce,(PRTL_RUN_ONCE_INIT_FN)InitFn,lpParameter,lpContext);
	//����ֵֻ�����ֽ����STATUS_SUCCESS��STATUS_UNSUCCESSFUL
	//�������ȫ��ZwTerminateProcess����û��ҪSetLastError��
	return NT_SUCCESS(Result);
}

BOOL WINAPI K32InitOnceComplete(LPINIT_ONCE lpInitOnce,DWORD dwFlags,LPVOID lpContext)
{
	NTSTATUS Result=RtlRunOnceComplete2(lpInitOnce,dwFlags,lpContext);
	if (!NT_SUCCESS(Result))
	{
		BaseSetLastNTError(Result);
		return FALSE;
	}
	return TRUE;
}