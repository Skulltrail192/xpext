
#include "common.h"

/*
Windows 7 SP1 32λ 6.1.7601.17514
@���� 2021.4.12

CV�ķ���
CV�ڲ�Ҫ�������ǣ��ͷ���-����-�������ѣ���ȡ��
������ʵ�ֻ�����SRW��Ϊ���ƣ�������������������
ǰ��28λ�ǽڵ�ָ�룬����4λ�Ǳ��
����SleepXXXʱ�����������ͷ������һ��
�����ǲ����һ��Ϊlast����һ��Ϊfirst
last��backһֱ�����ҵ�first��first��nextһֱ�����ҵ�last
��ȻҲ�����Ƶ������Ż�����

����Ҫ�Ĳ�ͬ���ڻ���
�����ǰû�������̲߳�������ֱ�ӵ��û��Ѻ�������
�����߳����ڲ�������ʱ�����̻߳�����CVF_Link��ǣ���ֹ�����̷߳���
����ʱ�л������󣬻��ں�3λ��flag���¼�¼��Ȼ����������
ÿ����������ĺ�������󶼻����һ�λ��Ѻ�������������ִ�л���
���3λ��flagʵ���Ǹ�count������1���͸����count+1��3λ����ܴ�7������
�������ڵ���7ʱ����Ϊȫ�����ѣ��ᵼ��spurious wakeup��

���Ѻ���ͳ�����������������7ȫ�����ѣ�С��7�������ȡ����Ӧ�ĸ���
��������������ж��پͻ��Ѷ��٣���������ȫ�������е㲻һ����
ȫ�����ѽ�status��0������ȡlast�ڵ㣬����back���������нڵ�
���������ѻ�׼��һ���µ�����collector�����������Ƴ��Ľڵ��ռ�����
�Ƴ��Ӿ������first��ʼ����next����ֱ��lastΪֹ�����������Ļ���
�Ƴ��Ľڵ�ӵ�ǰ�ڵ�ĵ�λ�ò��룬��Ϊback�ڵ�
oldcurr->next->back=NULL;
newcurr->back=oldcurr;
���ԭ�����last��first����back����������54321
��ô�������collector��head����tail����back������������12345
��ô����֤�˲�����ʽ��һ���ԣ�������������back���������

����ʱ������һ���Ż����ԣ��������SRW�Ļ������SRW��״̬
���SRW��ռ�ã���ʱ������û������ģ����ŵ���AcquireSRW�ֻ��������
����CV��ʵ�ֽ�SRW�����3���֣���һ������SRW��״̬�ж�
�ڶ�������SRW�����ߣ�����������SRW���Ѻ��ȡʧ�ܣ���ѭ��һ��
���Ѻ���ʵ���˵�һ���֣����߳��ӵ�SRW�ȴ�����ȥ��ʡȥ�˵ڶ����ֵĿ���
SRW���ͷű䶯ʱ���̲߳ű����ѣ���������AcquireSRW���Ի�ȡ��
�ɹ��򷵻أ�ʧ����ͣ������ȴ�������SRW����ѭ����һ��һ��

��SRW��ͬ��CV���ܵȴ���ʱ���Լ���������
��ʱֻҪ�������Ӧ�Ľڵ��������ɾ�����Ϳ��Է�����
������Ϊ���ѵ��Ż����ԣ��ڵ���ܲ����������ˣ����Ǳ��ӵ�SRW��
������ҪѰ�ҽڵ㣬�ֱ����������
�ҵ��ڵ�һ��������ɾ������ڵ㲢����
�Ҳ����ڵ�˵��SRW��ռ�ã������ǻ��ѵ�ʱ�򣬾ͼ�������
*/

DWORD ConditionVariableSpinCount=0x400;

void NTAPI RtlpInitConditionVariable(PEB* pPeb)
{
	if (pPeb->NumberOfProcessors==1)
		ConditionVariableSpinCount=0; 
}

void NTAPI RtlInitializeConditionVariable(RTL_CONDITION_VARIABLE* ConditionVariable)
{
	ConditionVariable->Ptr=NULL;
}

//����TRUE�����ѣ�����FALSE����
BOOL NTAPI RtlpQueueWaitBlockToSRWLock(SYNCITEM* Item,RTL_SRWLOCK* SRWLock,BOOL IsSharedLock)
{
	SYNCSTATUS OldStatus=(SYNCSTATUS)SRWLock->Ptr;
	DWORD dwBackOffCount=0;
	//û�˳��������ɻ�ȡ������
	if ((OldStatus&SRWF_Hold)==0)
	{
		do 
		{
			if (IsSharedLock==FALSE)
				Item->attr|=SYNC_Exclusive;
			else
			{
				//������˵ȴ����ҹ����������0��˵���ǳ������ǹ�����
				//���߳�Ҫ��ȡ���������ɻ�ȡ������
				//����ǰ�����߳��ڵȴ������߳�ҲӦ�ø��ں����
				if (!(OldStatus&SRWF_Wait) && (OldStatus&CVM_ITEM)!=0)
					return FALSE;
			}
			//ǰ��������һ����ռ���ڵȴ������º����Ŷ�
			//����ռ���ķ�ʽ������ڵ����
			Item->next=NULL;
			SYNCSTATUS NewStatus;
			if (OldStatus&SRWF_Wait)
			{
				Item->first=NULL;
				Item->count=0;
				Item->back=(SYNCITEM*)(OldStatus&CVM_ITEM);
				NewStatus=(OldStatus&SRWM_FLAG)|(SYNCSTATUS)Item;
			}
			else
			{
				Item->count=OldStatus>>SRW_COUNT_BIT;
				Item->first=Item;
				NewStatus=(SYNCSTATUS)Item;
				if (Item->count>1)
					NewStatus|=SRWF_Hold|SRWF_Wait|SRWF_Many;
				else
					NewStatus|=SRWF_Hold|SRWF_Wait;
			}
			SYNCSTATUS CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)SRWLock,NewStatus,OldStatus);
			//����SRW�ɹ�������TRUE������
			if (CurrStatus==OldStatus)
				return TRUE;
			RtlBackoff(&dwBackOffCount);
			OldStatus=(SYNCSTATUS)SRWLock->Ptr;
		} while (OldStatus&SRWF_Hold);
	}
	return FALSE;
}

void NTAPI RtlpWakeConditionVariable(RTL_CONDITION_VARIABLE* ConditionVariable,SYNCSTATUS OldStatus,int WakeCount)
{
	SYNCITEM* notify=NULL;
	int CollectNum=0;
	SYNCSTATUS CurrStatus;
	SYNCITEM** InsertPos=&notify;

	while (1)
	{
		//�Ǽǵ�����������ȫ�����ѣ���last������֪ͨ�����������
		if ((OldStatus&CVF_Full)==CVF_Full)
		{
			CurrStatus=InterlockedExchange((SYNCSTATUS*)ConditionVariable,0);
			*InsertPos=(SYNCITEM*)(CurrStatus&CVM_ITEM);
			break;
		}
		//���������������ѣ���first�˿�ʼ
		SYNCITEM* curr=(SYNCITEM*)(OldStatus&CVM_ITEM);
		int RequestNum=(OldStatus&CVM_COUNT)+WakeCount;
		SYNCITEM** FirstPos=&curr->first;
		if (*FirstPos==NULL)
		{
			do 
			{
				SYNCITEM* temp=curr;
				curr=curr->back;
				curr->next=temp;
			} while (curr->first==NULL);
		}
		curr=curr->first;
		//���ڵ�ӵȴ������Ƴ���ת�Ƶ�֪ͨ����
		if (CollectNum<RequestNum)
		{
			do 
			{
				SYNCITEM* next=curr->next;
				if (next==NULL)		//����head��û�и�����
					break;
				CollectNum++;
				*InsertPos=curr;	//��һ������notifyͷ���룬���涼����back������
				//curr��Ϊ������Ķ˵㣬��backΪNULL
				//ʵ�ʾ�����ÿ�Ƴ�һ��������ľͻὫback��ΪNULL������϶�ΪNULL
				//��Ȼ�½��������ò���next�����������next��ΪNULL������ǿ��֢
				curr->back=NULL;
				*FirstPos=next;		//��first���Ƴ�һ���󣬵�ǰ��first���Ϊfirst->next��head����¼��firstҪͬ������
				next->back=NULL;	//curr���������룬next��Ϊ�µ�first
				InsertPos=&curr->back;
				curr=next;			//��first��last����
			} while (CollectNum<RequestNum);
		}
		//��������������ж���֪ͨ����
		if (CollectNum<RequestNum)
		{
			CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)ConditionVariable,0,OldStatus);
			if (CurrStatus==OldStatus)
			{
				//û�и����ˣ��������һ������ʼ֪ͨ��CollectNum�Ѿ��ò�����
				*InsertPos=curr;
				curr->back=NULL;
				break;
			}
			else
			{
				//����������µĵȴ��ڵ㣬notify�����CollectNum�������������ռ�
				//����û�������һ������Ϊ���������Ͳ�����ϣ������һ���ܹ���
				OldStatus=CurrStatus;
				continue;
			}
		}
		//�ռ������������������������������㣬��ʼ֪ͨ
		CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)ConditionVariable,OldStatus&CVM_ITEM,OldStatus);
		if (OldStatus==CurrStatus)
			break;
		OldStatus=CurrStatus;
	}
	//֪ͨnotify�����ռ��Ľڵ㣬��back������
	if (notify!=NULL)
	{
		do 
		{
			SYNCITEM* back=notify->back;
			if (InterlockedBitTestAndReset((LONG*)&notify->attr,SYNC_SPIN_BIT)==0)
			{
				//CV�ڵ��SRW�ڵ�ṹһ���������ظ�����
				if (notify->lock!=NULL || 
					RtlpQueueWaitBlockToSRWLock(notify,notify->lock,(notify->attr&SYNC_SharedLock))==FALSE)
				{
					NtReleaseKeyedEvent(GlobalKeyedEventHandle,notify,FALSE,NULL);
				}
				notify=back;
			}
		} while (notify!=NULL);
	}
}

//����TRUE׼����ɣ�����FALSE��������
BOOL NTAPI RtlpWakeSingle(RTL_CONDITION_VARIABLE* ConditionVariable,SYNCITEM* Item)
{
	SYNCSTATUS CurrStatus,NewStatus;
	SYNCSTATUS OldStatus=(SYNCSTATUS)ConditionVariable->Ptr;
	//�϶��Ҳ���Ŀ��ڵ���
	if (OldStatus==0)
		return FALSE;
	while (1)
	{
		//��������û���ü�����һ���ᷢ�����������Ǽ������߰�
		int Count=OldStatus&CVM_COUNT;
		if (Count==CVF_Full)
			return FALSE;
		//�����ڲ��������Ǽǻ������루ΪʲôҪȫ�����Ѷ�����+1����
		if (OldStatus&CVF_Link)
		{
			NewStatus=OldStatus|CVF_Full;
			CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)ConditionVariable,NewStatus,OldStatus);
			if (CurrStatus==OldStatus)
				return FALSE;
		}
		else
		{
			//׼����������
			NewStatus=OldStatus+CVF_Link;
			CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)ConditionVariable,NewStatus,OldStatus);
			if (CurrStatus==OldStatus)
				break;
		}
		OldStatus=CurrStatus;
		if (CurrStatus==0)
			return FALSE;
	}

	//���óɹ���CurrStatus����NewStatus
	SYNCITEM* curr=(SYNCITEM*)(NewStatus&CVM_ITEM);
	OldStatus=NewStatus;
	SYNCITEM* last=curr;
	SYNCITEM* next=NULL;
	int result=FALSE;

	//��last��back����
	if (curr!=NULL)
	{
		do 
		{
			//û�ҵ�Ŀ�꣬��back��������
			if (curr!=Item)	
			{
				curr->next=next;	//˳·��ȫnext��
				next=curr;			//����curr�����´�ѭ���У�curr=curr->back��next����������next��
				curr=curr->back;
			}
			//�ҵ�Ŀ�꣬�������Ƴ��������������������ظ��ڵ㣩
			else
			{
				if (next==NULL)	//curr��last
				{
					SYNCITEM* back=curr->back;
					NewStatus=(SYNCSTATUS)back;
					if (back!=NULL)
					{
						//NewStatus&CVM_FLAG=0_0;
						//OldStatus&CVM_FLAG=0_flag
						//0_0^0_flag=0_flag
						//back_0^0_flag=back_flag
						//��Ϊback�ǽڵ�ָ�룬�������4λΪ0
						//��һ����򣬽�OldStatus�ĺ�4λ��ֵ��back��4λ��0
						//������and���ض�ǰ��Ľ����ǰ28λΪ0��ֻ���º�4λ
						//�ڶ�����򣬽�backǰ28λ��ָ�벿�ָ�ֵ���м���ǰ���0
						//����������NewStatus= (OldStatus & 0x0F) | (back & 0xFFFFFFF0)
						NewStatus=((NewStatus ^ OldStatus) & CVM_FLAG) ^ (SYNCSTATUS)back;
					}
					//�Ƴ�curr������curr->back��Ϊ�µ�last
					CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)ConditionVariable,NewStatus,OldStatus);
					if (CurrStatus==OldStatus)
					{
						OldStatus=NewStatus;
						if (back==NULL)	//�������
							return TRUE;
						else
							result=TRUE;
					}
					else
					{
						//���óɹ���CurrStatus����NewStatus
						OldStatus=CurrStatus;
					}
					//����ָ�룬��������
					curr=(SYNCITEM*)(OldStatus&CVM_ITEM);
					last=curr;
					next=NULL;
				}
				else	//curr����last
				{
					curr=curr->back;
					result=TRUE;
					next->back=curr;	//һ�߶Ͽ�����
					if (curr==NULL)		//��ͷ�ˣ�firstǰ��û����
						break;
					curr->next=next;	//��һ�߶Ͽ�����
				}
			}
		} while (curr!=NULL);
		//������ͷ��currΪNULL��nextΪfirst
		if (last!=NULL)
			last->first=next;
	}

	RtlpWakeConditionVariable(ConditionVariable,NewStatus,0);
	return result;
}

void NTAPI RtlpOptimizeConditionVariableWaitList(RTL_CONDITION_VARIABLE* ConditionVariable,SYNCSTATUS OldStatus)
{
	do 
	{
		SYNCITEM* curr=(SYNCITEM*)(OldStatus&CVM_ITEM);
		SYNCITEM** FirstPos=&curr->first;
		if (*FirstPos==NULL)
		{
			do 
			{
				SYNCITEM* temp=curr;
				curr=curr->back;
				curr->next=temp;
			} while (curr->first==NULL);
		}
		curr=curr->first;
		*FirstPos=curr;
		SYNCSTATUS CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)ConditionVariable,OldStatus&CVM_ITEM,OldStatus);
		if (CurrStatus==OldStatus)
			return ;
		OldStatus=CurrStatus;
	} while ((OldStatus&CVM_COUNT)==0);
	//������Լ����������ڼ䣬�����ύ�˻������룬��Ӧ�ø�����
	RtlpWakeConditionVariable(ConditionVariable,OldStatus,0);
}



NTSTATUS NTAPI RtlSleepConditionVariableCS(RTL_CONDITION_VARIABLE* ConditionVariable,RTL_CRITICAL_SECTION* CriticalSection,LARGE_INTEGER* Timeout)
{
	//volatile
	__declspec(align(16)) SYNCITEM item;
	item.next=NULL;
	item.lock=NULL;
	item.attr=SYNC_Spinning;

	SYNCSTATUS OldStatus=(SYNCSTATUS)ConditionVariable->Ptr;
	SYNCSTATUS NewStatus;

	while (1)
	{
		NewStatus=(OldStatus&CVM_FLAG)|((SYNCSTATUS)&item);
		item.back=(SYNCITEM*)(OldStatus&CVM_ITEM);
		if (item.back==NULL)
		{
			//������û�еȴ��ڵ㣬�Լ��ǵ�һ��
			item.first=&item;
		}
		else
		{
			//�������Ѿ��еȴ��ڵ㣬�����Ƿŵ�back
			item.first=NULL;
			NewStatus|=CVF_Link;
		}
		//������ڵ����
		SYNCSTATUS CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)ConditionVariable,NewStatus,OldStatus);
		if (CurrStatus==OldStatus)
			break;
		OldStatus=CurrStatus;
	}

	RtlLeaveCriticalSection(CriticalSection);

	//����3λ����0��ʼ�����������4�����
	//1.old�б�ǣ�new�б�ǣ����Ϊfalse
	//����߳����ڲ���������Ҫ�Ż����޷�����
	//2.old�б�ǣ�newû��ǣ����Ϊtrue
	//����߳����ڲ�����������������ֻ��һ����ΪʲôҪ�����Ż���
	//3.oldû��ǣ�newû��ǣ����Ϊfalse
	//���˲���������������ֻ��һ��������Ҫ�Ż�
	//4.oldû��ǣ�new�б�ǣ����Ϊtrue
	//���˲���������Ҫ�Ż������Խ����Ż�
	if ((OldStatus^NewStatus)&CVF_Link)
	{
		RtlpOptimizeConditionVariableWaitList(ConditionVariable,NewStatus);
	}

	for (int i=ConditionVariableSpinCount;i>0;i--)
	{
		if (!(item.attr&SYNC_Spinning))
			break;
		_mm_pause();
	}

	NTSTATUS result=STATUS_SUCCESS;
	if (InterlockedBitTestAndReset((long*)(&item.attr),SYNC_SPIN_BIT))
	{
		result=NtWaitForKeyedEvent(GlobalKeyedEventHandle,&item,FALSE,Timeout);
		if (result==STATUS_TIMEOUT)
		{
			//�������������ȥ�������Լ��Ľڵ��Ƴ�
			if (RtlpWakeSingle(ConditionVariable,&item)==FALSE)
			{
				NtWaitForKeyedEvent(GlobalKeyedEventHandle,&item,FALSE,NULL);
				result=STATUS_SUCCESS;
			}
		}
	}
	RtlEnterCriticalSection(CriticalSection);
	return result;
}

NTSTATUS NTAPI RtlSleepConditionVariableSRW(RTL_CONDITION_VARIABLE* ConditionVariable,RTL_SRWLOCK* SRWLock,LARGE_INTEGER* Timeout,ULONG Flags)
{
	//volatile
	__declspec(align(16)) SYNCITEM item;
	if (Flags&(~CONDITION_VARIABLE_LOCKMODE_SHARED))	//0xFFFFFFFE
		return 0xC00000F0;	//STATUS_INVALID_PARAMETER_2
	SYNCSTATUS OldStatus=(SYNCSTATUS)ConditionVariable->Ptr;
	item.next=NULL;
	item.lock=SRWLock;
	item.attr=SYNC_Spinning;
	BOOL IsSharedLock=Flags&CONDITION_VARIABLE_LOCKMODE_SHARED;
	if (IsSharedLock)
		item.attr|=SYNC_SharedLock;

	SYNCSTATUS NewStatus;
	while (1)
	{
		NewStatus=(OldStatus&CVM_FLAG)|(SYNCSTATUS)&item;
		item.back=(SYNCITEM*)(OldStatus&CVM_ITEM);
		if (item.back==NULL)
		{
			item.first=&item;
		}
		else
		{
			item.first=NULL;
			NewStatus|=CVF_Link;
		}
		SYNCSTATUS CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)ConditionVariable,NewStatus,OldStatus);
		if (CurrStatus==OldStatus)
			break;
		OldStatus=CurrStatus;
	}

	if (IsSharedLock)
		RtlReleaseSRWLockShared(SRWLock);
	else
		RtlReleaseSRWLockExclusive(SRWLock);

	if ((OldStatus^NewStatus) & CVF_Link)
		RtlpOptimizeConditionVariableWaitList(ConditionVariable,NewStatus);

	for (int i=ConditionVariableSpinCount;i>0;i--)
	{
		if (!(item.attr&SYNC_Spinning))
			break;
		_mm_pause();
	}

	NTSTATUS result=STATUS_SUCCESS;
	if (InterlockedBitTestAndReset((LONG*)(&item.attr),SYNC_SPIN_BIT))
	{
		result=NtWaitForKeyedEvent(GlobalKeyedEventHandle,&item,FALSE,Timeout);
		if (result==STATUS_TIMEOUT)
		{
			if (RtlpWakeSingle(ConditionVariable,&item)==FALSE)
			{
				NtWaitForKeyedEvent(GlobalKeyedEventHandle,&item,FALSE,NULL);
				result=STATUS_SUCCESS;
			}
		}
	}

	if (IsSharedLock)
		RtlAcquireSRWLockShared(SRWLock);
	else
		RtlAcquireSRWLockExclusive(SRWLock);
	return result;
}

void NTAPI RtlWakeConditionVariable(RTL_CONDITION_VARIABLE* ConditionVariable)
{
	SYNCSTATUS OldStatus=(SYNCSTATUS)ConditionVariable->Ptr;
	SYNCSTATUS CurrStatus;
	if (OldStatus!=0)
	{
		do 
		{
			//����߳��ڲ�������
			if (OldStatus&CVF_Link)
			{
				//�Ѿ���ȫ�����ѵĵǼ��ˣ�����
				if ((OldStatus&CVF_Full)==CVF_Full)
					return ;
				//��Ҫ���ѵĸ���+1
				CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)ConditionVariable,OldStatus+1,OldStatus);
				if (CurrStatus==OldStatus)
					return ;
			}
			else
			{
				//��ǰû�б���̲߳����������л���
				CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)ConditionVariable,OldStatus+CVF_Link,OldStatus);
				if (CurrStatus==OldStatus)
				{
					RtlpWakeConditionVariable(ConditionVariable,OldStatus+CVF_Link,1);
					return ;
				}
			}
			OldStatus=CurrStatus;
		} while (OldStatus==0);
	}
}

void NTAPI RtlWakeAllConditionVariable(RTL_CONDITION_VARIABLE* ConditionVariable)
{
	SYNCSTATUS OldStatus=(SYNCSTATUS)ConditionVariable->Ptr;
	SYNCSTATUS CurrStatus;
	if (OldStatus!=0)
	{
		do 
		{
			//�Ѿ���ȫ�����ѵĵǼ��ˣ�����
			if ((OldStatus&CVF_Full)==CVF_Full)
				return ;
			//����߳��ڲ��������Ǽ�ȫ������
			if ((OldStatus&CVF_Link))
			{
				CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)ConditionVariable,OldStatus|CVF_Full,OldStatus);
				if (OldStatus==CurrStatus)
					return ;
			}
			else
			{
				//�������ÿգ���last����back���������
				CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)ConditionVariable,0,OldStatus);
				if (OldStatus==CurrStatus)
				{
					SYNCITEM* curr=(SYNCITEM*)(OldStatus&CVM_ITEM);					
					if (curr!=NULL)
					{
						do 
						{
							SYNCITEM* back=curr->back;
							if (InterlockedBitTestAndReset((LONG*)&curr->attr,SYNC_SPIN_BIT)==0)
								NtReleaseKeyedEvent(GlobalKeyedEventHandle,curr,FALSE,NULL);
							curr=back;
						} while (curr!=NULL);
					}
					return ;
				}
			}
			OldStatus=CurrStatus;
		} while (OldStatus!=0);
	}
}


BOOL WINAPI K32SleepConditionVariableCS(PCONDITION_VARIABLE ConditionVariable,PCRITICAL_SECTION CriticalSection,DWORD dwMilliseconds)
{
	LARGE_INTEGER Timeout;
	NTSTATUS Result=RtlSleepConditionVariableCS(ConditionVariable,CriticalSection,BaseFormatTimeOut(&Timeout,dwMilliseconds));
	BaseSetLastNTError(Result);
	if (NT_SUCCESS(Result) && Result!=STATUS_TIMEOUT)
		return TRUE;
	return FALSE;
}

BOOL WINAPI K32SleepConditionVariableSRW(PCONDITION_VARIABLE ConditionVariable,PSRWLOCK SRWLock,DWORD dwMilliseconds,ULONG Flags)
{
	LARGE_INTEGER Timeout;
	NTSTATUS Result=RtlSleepConditionVariableSRW(ConditionVariable,SRWLock,BaseFormatTimeOut(&Timeout,dwMilliseconds),Flags);
	BaseSetLastNTError(Result);
	if (NT_SUCCESS(Result) && Result!=STATUS_TIMEOUT)
		return TRUE;
	return FALSE;
}