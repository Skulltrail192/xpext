
#include "common.h"

/*
Windows 7 SP1 32λ 6.1.7601.17514
@���� 2020.6.5

SRW�ķ���
����״̬����ռ����͹��������ܻ�ȡ��
��ռ״̬����ռ����͹������󶼲��ܻ�ȡ��
����״̬�������������ܻ�ȡ������ռ�����ܻ�ȡ��
�����ϣ������ǰ�ǹ����������������й������󶼿��Ի�ȡ��
����Ϊ�˷�ֹд���̳߳��ּ���״̬��Ӧ������Թ�ƽ���㷨
һ�����ֶ�ռ���󣬺��������й�������Ҫ�ȴ�

SRWLOCK��һ���������������������ȫ����Ϣ
SRWLOCK::Ptr��ǰ28λ�����˵ȴ������ͷ�������������
��4λʹ��align(16)�ճ������ڱ���״̬
����ڵ���ջ�Ϸ��䣬�������߷���ǰһֱ��Ч������ʱ�Զ��ͷ�
ÿ��ֻ����һ���̱߳༭������SRWF_Link��ǿ���
���߳̽���ȴ�״̬ʱ���Ѿ�����ͷ����Ϊ�Լ���back�ڵ�
�������µ�����ͷ�����滻�ɵ�����ͷ������ɲ���
newi->back=status;
status=&newi;
����˳��status��backһֱ��ǰ���ң��ǰ�ʱ���ɽ���Զ����Ľڵ�
�������һ��Ϊlast��Ҳ��ͷ������һ��Ϊfirst����β��
last��backһֱ�����ҵ�first��first��nextһֱ�����ҵ�last

��Ϊfirst�ڵ��㹻��Ҫ��last�ڵ�ᱣ��firstָ�룬���ٺ�������
��һ������Ľڵ㣬��firstָ��ָ������������firstҲ��last
֮�����Ľڵ㣬������back��֮ǰ�Ľڵ㣬���������firstָ�븴�ƹ���
if (curr->first!=NULL)
last->first=curr->first;
�տ�ʼ����û��next�ֶΣ��ڴ���firstָ���ʱ����������ȫnext�ֶ�
curr->back->next=curr;
curr=curr->back;
����������ϳ��Ż����ڲ����½ڵ�ʱִ��

���ѽڵ��first�˿�ʼ����Ϊ��ߵ��̵߳ȴ�ʱ�����
���ֻ����һ���̣߳����������ɾ������ڵ㣬���Ͽ�next����
ɾ��������last�ڵ��first�ֶ��ϣ���Ϊɾ��ʱ�ܴ�������first�ڵ�
�Ż���������ͣ�£�Ȼ���ƹ�ȥ��Ҳ�����ܵ�Ӱ��
wake=last->first;
last->first=last->first->next;
wake->next=NULL;
�����Ҫ���ѵ�����Ƚϸ��ӣ��ͻỽ�����нڵ�
��ʱstatus��0����first��ʼ����next�����λ����߳�
Ȼ����Щ�߳����������������ķ��أ����������ٴ�����

Ϊ������⣬�һ�������ͼsrw_cv.png
*/

DWORD SRWLockSpinCount=0x400;

void NTAPI RtlpInitSRWLock(PEB* pPEB)
{
	if (pPEB->NumberOfProcessors==1)
		SRWLockSpinCount=0;
}

void NTAPI RtlInitializeSRWLock(RTL_SRWLOCK* SRWLock)
{
	SRWLock->Ptr=NULL;
}

void NTAPI RtlpWakeSRWLock(RTL_SRWLOCK* SRWLock,SYNCSTATUS OldStatus)
{
	SYNCSTATUS CurrStatus;
	SYNCITEM* last;
	SYNCITEM* first;
	while (1)
	{
		//�Ѿ����߳����Ȼ�ȡ������ȡ�����Ѳ���
		if (OldStatus&SRWF_Hold)	//��������while(...)�����if (...) do {} while(...)
		{
			do 
			{
				CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)SRWLock,OldStatus-SRWF_Link,OldStatus);	//�������������
				//״̬�������̸߳��£�����״̬ʧ��
				//���η���ʧЧ������״̬���·���
				//�����д������ƴ��룬�����ظ�˵��
				if (CurrStatus==OldStatus)
					return ;
				OldStatus=CurrStatus;
			} while (OldStatus&SRWF_Hold);
		}

		last=(SYNCITEM*)(OldStatus&SRWM_ITEM);
		first=last->first;
		if (first==NULL)
		{
			SYNCITEM* curr=last;
			do 
			{
				curr->back->next=curr;	//��ȫ����
				curr=curr->back;		//��������
				first=curr->first;		//���²��ҽ��
			} while (first==NULL);		//��һ����Ч��first
			//firstָ����ǰ������ĵط�
			//�Ż�������û������жϣ�����ǲ������ڵ���Ҫ�Ż�ʱ��firstһ����Ϊlast
			if (last!=curr)	
				last->first=first;
		}

		//����������нڵ�ȴ���������Ƕ�ռ����
		if ((first->next!=NULL) && (first->attr&SYNC_Exclusive))
		{
			last->first=first->next;	//��������ɾ������ڵ㣨ɾ�����Ż�ÿ�ζ��������firstָ�룩
			first->next=NULL;			//first��ԭ��������
			_InterlockedAnd((long*)SRWLock,(~SRWF_Link));	//�������ȫ����ɣ�ȥ�����
			break;
		}
		//���򣬿���ֻ����һ���ڵ�ȴ���������ǹ�������ȫ������
		else
		{
			CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)SRWLock,0,OldStatus);	//��״̬����Ϊ����
			if (OldStatus==CurrStatus)
				break;
			last->first=first;	//���ҵ���first�ŵ������λ��
			OldStatus=CurrStatus;
		}
	}

	//���λ����̣߳����ܽ���firstһ����Ҳ������first���ϵ�ȫ��
	//�����ȫ�����ѣ��������̻߳��ٴ������������������ٴ�ѭ����������������
	//�ô���ʡ���˸�������ķ��������漸�����������ɹ��������ֱ��������ռ��
	do 
	{
		//���������̻߳᷵�أ�ջ�ϵ�itemʧЧ�������ȱ���next
		SYNCITEM* next=first->next;
		//�����SYNC_Spinning��ǣ���ʾ���������ȴ���������������
		//�����lock btr������0��Ŀ���̷߳��ֺ���������
		//���û��SYNC_Spinning��ǣ�˵��Ŀ���߳�����˴˱�ǣ���ʽ��������
		//�����lock btrû��Ӱ�죬���̸߳���Ŀ���̻߳���
		//��Ҫע����ǣ�NtReleaseKeyedEvent����key��û������ʱ����������ǰ�߳�
		//ֱ�����߳��ô�key������NtWaitForKeyedEvent���Żỽ�ѣ���˲��ᶪʧ֪ͨ
		if (InterlockedBitTestAndReset((LONG*)&(first->attr),SYNC_SPIN_BIT)==0)
			NtReleaseKeyedEvent(GlobalKeyedEventHandle,first,FALSE,NULL);
		first=next;	//��������
	} while (first!=NULL);
}

void NTAPI RtlpOptimizeSRWLockList(RTL_SRWLOCK* SRWLock,SYNCSTATUS OldStatus)
{
	if (OldStatus&SRWF_Hold)
	{
		do 
		{
			SYNCITEM* last=(SYNCITEM*)(OldStatus&SRWM_ITEM);
			if (last!=NULL)
			{
				SYNCITEM* curr=last;
				while (curr->first==NULL)
				{
					curr->back->next=curr;	//��ȫ����
					curr=curr->back;		//��������
				}
				last->first=curr->first;	//��first�ŵ���������������λ�ã������´β���
			}
			//�������������������
			SYNCSTATUS CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)SRWLock,OldStatus-SRWF_Link,OldStatus);
			if (CurrStatus==OldStatus)
				return ;
			OldStatus=CurrStatus;
		} while (OldStatus&SRWF_Hold);
	}
	//�����ͷ�������ֹͣ�Ż�����Ϊ����
	RtlpWakeSRWLock(SRWLock,OldStatus);
}

void NTAPI RtlAcquireSRWLockExclusive(RTL_SRWLOCK* SRWLock)
{
	//volatile
	__declspec(align(16)) SYNCITEM item;
	BOOL IsOptimize;
	SYNCSTATUS NewStatus;
	DWORD dwBackOffCount=0;

	//�����ǰ״̬Ϊ���У�ֱ�ӻ�ȡ��
	//����ĳ���̸߳��ͷ������������Hold��ǣ������̻߳�û���ü���ȡ��
	//���߳�Ҳ���Գû���ȡ�������ñ�ǣ���Ѳ���ȡ�����Ѻ��ٴν���ȴ�
	if (InterlockedBitTestAndSet((LONG*)SRWLock,SRW_HOLD_BIT)==0)
		return ;

	SYNCSTATUS OldStatus=(SYNCSTATUS)(SRWLock->Ptr);
	SYNCSTATUS CurrStatus;
	while (1)
	{
		//�����ǰ�����̳߳����������߳̽������ڵ㣬���Լ���������
		if (OldStatus&SRWF_Hold)
		{
			if (RtlpWaitCouldDeadlock())
			{
				//GetCurrentProcess(),STATUS_THREAD_IS_TERMINATING
				ZwTerminateProcess((HANDLE)0xFFFFFFFF,0xC000004B);
			}

			item.attr=SYNC_Exclusive|SYNC_Spinning;
			item.next=NULL;
			IsOptimize=FALSE;

			//������߳��Ѿ���ǰ��ȴ��ˣ��Ͱ�֮ǰ�Ľڵ���Ϊback
			if (OldStatus&SRWF_Wait)
			{
				item.first=NULL;
				item.count=0;
				item.back=(SYNCITEM*)(OldStatus&SRWM_ITEM);
				NewStatus=((SYNCSTATUS)&item)|(OldStatus&SRWF_Many)|(SRWF_Link|SRWF_Wait|SRWF_Hold);

				if (!(OldStatus&SRWF_Link))	//��ǰû�˲����������Ż�����
					IsOptimize=TRUE;
			}
			//������߳��ǵ�һ���ȴ����̣߳�firstָ���Լ�
			//����ʱ��firstָ��Ϊ׼������Ҫ����back
			else
			{
				item.first=&item;
				//�������ӵ�����Զ�ռ��ʽ���У��������Ϊ0
				//�������ӵ�����Թ���ʽ���У��������Ϊ1�����
				item.count=OldStatus>>SRW_COUNT_BIT;
				if (item.count>1)
					NewStatus=((SYNCSTATUS)&item)|(SRWF_Many|SRWF_Wait|SRWF_Hold);
				else
					NewStatus=((SYNCSTATUS)&item)|(SRWF_Wait|SRWF_Hold);
			}
			//�ύ��״̬
			CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)SRWLock,NewStatus,OldStatus);
			if (CurrStatus==OldStatus)
			{
				if (IsOptimize)
					RtlpOptimizeSRWLockList(SRWLock,NewStatus);
				//�����ں˵Ĵ���̫�ߣ��Ƚ���һ�������ȴ�
				for (int i=SRWLockSpinCount;i>0;i--)
				{
					if (!(item.attr&SYNC_Spinning))	//�����߳̿��ܻ��ѱ��̣߳�������
						break;
					_mm_pause();
				}
				//���һֱû�ܵȵ����ѣ��ͽ����ں�����
				if (InterlockedBitTestAndReset((LONG*)(&item.attr),SYNC_SPIN_BIT))
					NtWaitForKeyedEvent(GlobalKeyedEventHandle,&item,FALSE,NULL);
				//�����Ѻ��ٴ�ѭ���������
				OldStatus=CurrStatus;
			}
			else
			{
				//�̴߳��ڼ��ҵľ����У��˱�һ��ʱ��
				RtlBackoff(&dwBackOffCount);
				OldStatus=(SYNCSTATUS)(SRWLock->Ptr);
			}
		}
		//����߳̿�������ʲô����������û���̳߳������ˣ����Ի�ȡ��
		else
		{
			CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)SRWLock,OldStatus+SRWF_Hold,OldStatus);
			if (CurrStatus==OldStatus)
				return ;
			RtlBackoff(&dwBackOffCount);
			OldStatus=(SYNCSTATUS)(SRWLock->Ptr);
		}
	}
}

void NTAPI RtlAcquireSRWLockShared(RTL_SRWLOCK* SRWLock)
{
	//volatile
	__declspec(align(16)) SYNCITEM item;
	BOOL IsOptimize;
	DWORD dwBackOffCount=0;

	SYNCSTATUS NewStatus;
	SYNCSTATUS CurrStatus;
	SYNCSTATUS OldStatus=InterlockedCompareExchange((SYNCSTATUS*)SRWLock,(1<<SRW_COUNT_BIT)|SRWF_Hold,0);
	//�����ǰ״̬Ϊ���У�ֱ�ӻ�ȡ��
	if (OldStatus==0)
		return ;

	while (1)
	{
		//���ռ����Ҫ�ȴ������
		//���ڹ�ƽ�Կ��ǣ�ֻҪ�ж�ռ�����󣬺��������й���������Ҫ�Ŷӣ���ʹ��ǰ�����ڹ���״̬��
		//����wait��ǣ�˵����1.��ǰ�Ƕ�ռ������������ʲô���͵�����Ҫ�Ŷ�
		//2.��ǰ�ǹ����������Ƕ������ж�ռ�����󣬺����Ĺ�����ҲӦ���Ŷ�
		//��Ϊ�Աȣ�����ǰ�ǹ������������ŵĹ����������ֱ�ӻ�ȡ�����������������wait���
		//����һ�������������ǰ�Ƕ�ռ��������û���߳���������Ҳ��û��wait���
		//�������������share countΪ0����Ϊ�Աȣ�ֻ�е���������ʱshare countΪ1��
		//һ�����������������߾ͻ�ȴ��������wait��ǵ����
		if ((OldStatus&SRWF_Hold) && ((OldStatus&SRWF_Wait) || ((OldStatus&SRWM_ITEM)==NULL)))
		{
			if (RtlpWaitCouldDeadlock())
				ZwTerminateProcess((HANDLE)0xFFFFFFFF,0xC000004B);

			item.attr=SYNC_Spinning;
			item.count=0;
			IsOptimize=FALSE;
			item.next=NULL;

			if (OldStatus&SRWF_Wait)
			{
				item.back=(SYNCITEM*)(OldStatus&SRWM_ITEM);
				//ԭ��������ôд�ģ����Ǹղ�SRWF_Hold�Ѿ���⵽��
				NewStatus=((SYNCSTATUS)&item)|(OldStatus&(SRWF_Many|SRWF_Hold))|(SRWF_Link|SRWF_Wait);
				item.first=NULL;

				if (!(OldStatus&SRWF_Link))
					IsOptimize=TRUE;
			}
			else
			{
				item.first=&item;
				//��ǰһ���Ƕ�ռ�������Բ��ÿ���SRWF_Many
				NewStatus=((SYNCSTATUS)&item)|(SRWF_Wait|SRWF_Hold);
			}

			CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)SRWLock,NewStatus,OldStatus);
			if (CurrStatus==OldStatus)
			{
				if (IsOptimize)
					RtlpOptimizeSRWLockList(SRWLock,NewStatus);

				for (int i=SRWLockSpinCount;i>0;i--)
				{
					if (!(item.attr&SYNC_Spinning))
						break;
					_mm_pause();
				}

				if (InterlockedBitTestAndReset((LONG*)&(item.attr),SYNC_SPIN_BIT))
					NtWaitForKeyedEvent(GlobalKeyedEventHandle,&item,FALSE,NULL);
				OldStatus=CurrStatus;
			}
			else
			{
				RtlBackoff(&dwBackOffCount);
				OldStatus=(SYNCSTATUS)SRWLock->Ptr;
			}
		}
		else
		{
			//ĳ���̸߳��ͷ������������Hold��ǣ������̻߳�û���ü���ȡ��
			//���߳̿��Գû���ȡ�������ñ�ǣ���Ѳ���ȡ�����Ѻ��ٴ���ռ��
			//�����е�С���⣬����ո��Ƕ�ռ���ͷţ���ʹ�����ǹ�������
			//Ҳ�п���ȡ�����Ѳ����������Ǻ͵�ǰ�Ĺ����߳�һ���ȡ��
			if (OldStatus&SRWF_Wait)
				NewStatus=OldStatus+SRWF_Hold;
			//��ǰ���ڹ���״̬�����Ի�ȡ�������ӹ������
			else
				NewStatus=(OldStatus+(1<<SRW_COUNT_BIT))|SRWF_Hold;
			CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)SRWLock,NewStatus,OldStatus);
			if (CurrStatus==OldStatus)
				return ;
			RtlBackoff(&dwBackOffCount);
			OldStatus=(SYNCSTATUS)SRWLock->Ptr;
		}
	}
}

void NTAPI RtlReleaseSRWLockExclusive(RTL_SRWLOCK* SRWLock)
{
	//ȥ��Hold���
	SYNCSTATUS OldStatus=InterlockedExchangeAdd((SYNCSTATUS*)SRWLock,-SRWF_Hold);
	if (!(OldStatus&SRWF_Hold))
		RtlRaiseStatus(0xC0000264);	//STATUS_RESOURCE_NOT_OWNED
	//���߳��ڵȴ�����û���߳����ڲ�������ִ�л��Ѳ���
	//����ǰ����������̼߳�⵽״̬�ı䣬ִ�л��Ѳ���
	if ((OldStatus&SRWF_Wait) && !(OldStatus&SRWF_Link))
	{
		OldStatus-=SRWF_Hold;
		SYNCSTATUS CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)SRWLock,OldStatus+SRWF_Link,OldStatus);
		if (OldStatus==CurrStatus)
			RtlpWakeSRWLock(SRWLock,OldStatus+SRWF_Link);
	}
}

void NTAPI RtlReleaseSRWLockShared(RTL_SRWLOCK* SRWLock)
{
	SYNCSTATUS CurrStatus,NewStatus;
	SYNCSTATUS OldStatus=InterlockedCompareExchange((SYNCSTATUS*)SRWLock,0,((1<<SRW_COUNT_BIT)|SRWF_Hold));
	//����������Ϊ1���ұ�ǽ�ΪHold
	//˵������һ�����������ָ�������״̬�Ϳ�����
	if (OldStatus==((1<<SRW_COUNT_BIT)|SRWF_Hold))
		return ;

	if (!(OldStatus&SRWF_Hold))
		RtlRaiseStatus(0xC0000264);

	//ֻ���ڹ�����
	if (!(OldStatus&SRWF_Wait))
	{
		do 
		{
			//�������Ϊ1�����Ϊ����״̬
			if ((OldStatus&SRWM_COUNT)<=(1<<SRW_COUNT_BIT))
				NewStatus=0;
			//�����������0������-1
			else
				NewStatus=OldStatus-(1<<SRW_COUNT_BIT);
			CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)SRWLock,NewStatus,OldStatus);
			if (CurrStatus==OldStatus)
				return ;
			OldStatus=CurrStatus;
		} while (!(OldStatus&SRWF_Wait));
	}

	//�ж�ռ����ȴ�ʱ
	//����ж��������������-1
	if (OldStatus&SRWF_Many)
	{
		SYNCITEM* curr=(SYNCITEM*)(OldStatus&SRWM_ITEM);
		//Ѱ�������first�ڵ㣬��ѯ�������
		//�������ӹ���������������Ҳ���������ȴ��ڵ�
		//�������Ӷ�ռ������ռ����ȴ���������item��¼�������
		//����ģ���ռ���Ӷ�ռ�������ռ���ӹ���������¼�Ĺ������Ϊ0
		while (curr->first==NULL)
			curr=curr->back;	
		curr=curr->first;

		//�������-1����������������0��˵�����������߳�ռ�й�����
		DWORD count=InterlockedDecrement(&curr->count);
		if (count>0)
			return ;
	}

	//��������ȫ�ͷţ������¸��ȴ���
	while (1)
	{
		NewStatus=OldStatus&(~(SRWF_Many|SRWF_Hold));
		//���߳��ڲ�����������ȥ���Ѱ�
		if (OldStatus&SRWF_Link)
		{
			CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)SRWLock,NewStatus,OldStatus);
			if (CurrStatus==OldStatus)
				return ;
		}
		else
		{
			NewStatus|=SRWF_Link;
			CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)SRWLock,NewStatus,OldStatus);
			if (CurrStatus==OldStatus)
			{
				RtlpWakeSRWLock(SRWLock,NewStatus);
				return ;
			}
		}
		OldStatus=CurrStatus;
	}
}

BOOL NTAPI RtlTryAcquireSRWLockExclusive(RTL_SRWLOCK* SRWLock)
{
	BOOL IsLocked=InterlockedBitTestAndSet((LONG*)SRWLock,SRW_HOLD_BIT);
	return !(IsLocked==TRUE);
}

BOOL NTAPI RtlTryAcquireSRWLockShared(RTL_SRWLOCK* SRWLock)
{
	DWORD dwBackOffCount=0;
	SYNCSTATUS OldStatus=InterlockedCompareExchange((SYNCSTATUS*)SRWLock,(1<<SRW_COUNT_BIT)|SRWF_Hold,0);
	if (OldStatus==0)
		return TRUE;
	while (1) 
	{
		if ((OldStatus&SRWF_Hold) && ((OldStatus&SRWF_Wait) || (OldStatus&SRWM_ITEM)==NULL))
			return FALSE;
		SYNCSTATUS NewStatus;
		if (OldStatus&SRWF_Wait)
			NewStatus=OldStatus+SRWF_Hold;
		else
			NewStatus=OldStatus+(1<<SRW_COUNT_BIT);
		SYNCSTATUS CurrStatus=InterlockedCompareExchange((SYNCSTATUS*)SRWLock,NewStatus,OldStatus);
		if (CurrStatus==OldStatus)
			return TRUE;
		RtlBackoff(&dwBackOffCount);
		OldStatus=(SYNCSTATUS)SRWLock->Ptr;
	}
}


