
#include <ntddk.h>

/*
Windowsʱ�����С��Ԫ��100ns��һ��tick���������Ԫ��ʱ��
��ʱ���жϷ���ʱ�������һ���ж���������һЩʱ��
ϵͳ�ӵ�ǰtick��ȥ������ʱ�䣬ʣ���ʱ���¼��KiTickOffset
���ʣ��ʱ��С��0������ʱ�侭����һ��tick����ǰtick����
��ʱ��Ҫ����tick����ͳ���̹߳���ʱ�䣬�����߳�ʱ��Ƭ��quantum��
�������������£��������KiTimeUpdateNotifyRoutine��Win7����֧�֣�
���ΪKiTickOffset����KeMaximumIncrement��ʱ�䵥Ԫ�������¸�tick
��Win2003�Ĵ���������ʱ�����������˼���tick���������Ǽ�1����
���ʣ��ʱ�����0���ᴦ��һЩDPC�����ݣ�����Ͳ�����

����tickʱ�����ȸ���KeTickCount�����Ǹ�64λ��ֵ���㹻��¼������
��GetTickCount��ʹ�����ֵ������KUSER_SHARED_DATA���ֵ
Win7����ƫ��0x320��TickCount�ֶΣ�ͬ��Ҳ��64λ
XP����ƫ��0��TickCountLow��ֻ��32λ��ϵͳ����һ��ʱ����������
��ΪGetTickCount����Ҳ��32λ�ģ�һ������ֻ��49.7�죬ϵͳ�����������΢����

ֻҪ��KeUpdateSystemTime����TickCountLowʱ��ͬʱ����TickCount������ʵ�ֹ���
����1�Ǹ�ntoskrnl.exe���ļ���Ч���ȶ������鷳��ͨ���Բ�
����2������һ��Timer����ʱ��KeTickCount���ƹ�ȥ����򵥵��첽��������̫��
����3��ʹ��KiTimeUpdateNotifyRoutine������ֻ��һ��λ�ã����ܱ�ĳЩ�������ռ��
��ע�⣬�жϺ����ǿպ͵��ú����Ƿֿ��ģ�����û�м�������ջص�ָ���м��͸���������

����Windowsͨ��һЩ�㷨��ͬһʱ��ֻ����һ������ִ��KeUpdateSystemTime
���������첽����������������û��޸�ʱ�䣬Ҳ�����KeTickCount
����tickΪ��Ч��û��ʹ���������ܵ���KSYSTEM_TIME��Low��High1�����ֶβ�ͬ��
����Win7 32λ����High2Time��ֵ��High1Time��ͬ�������д��
�����ȡʱ��High2Time��High1Time��ͬ��˵��д��û����ɣ����̼߳串��д��
GetTickCount�Ե�һ�ᣬѭ���ٴζ�ȡ��������ͬʱ���ʹ�����������
XPʹ�õļ�ʱ������ʽ��RTC������Զ������ʽ��HPET��ʱ����û��Ҫ���������
����Win7 64λ��64λ�Ĳ���������һ����д�������ֶΣ�����Ҫ����ͬ����ʽ
*/

/*
VOID __fastcall KeUpdateSystemTime_Win7_32(KIRQL OldIrql,LONG Increment,KTRAP_FRAME* TrFrame)
{
	//...
	LONG MasterOffset=InterlockedExchangeAdd(&Kprcb->MasterOffset,-Increment);
	MasterOffset=MasterOffset-Increment;
	if (MasterOffset<=0)
	{
		ULONG SpinCount=0;
		while (KeTickCount.High1Time!=KeTickCount.High2Time)
		{
			SpinCount++;
			//HvlLongSpinCountMask=0xFFFFFFFF
			if ((SpinCount&HvlLongSpinCountMask)==0 && (HvlEnlightenments&0x40))
				HvlNotifyLongSpinWait(SpinCount);
			_mm_pause();
		}
		ULONGLONG TickCount=*(ULONGLONG*)&KeTickCount;
		TickCount++;
		KeTickCount.High2Time=(ULONG)(TickCount>>32);
		*(ULONGLONG*)&KeTickCount=TickCount;

		KUSER_SHARED_DATA* KUserSharedData=(KUSER_SHARED_DATA*)0xFFDF0000;
		KUserSharedData->TickCount.High2Time=(ULONG)(TickCount>>32);
		*(ULONGLONG*)&KUserSharedData->TickCount=TickCount;

		Kprcb->MasterOffset=KeMaximumIncrement+MasterOffset;
	}
	//...
}

VOID __stdcall KeUpdateSystemTime_XP(KIRQL OldIrql,ULONG Vector,KTRAP_FRAME* TrFramePtr)
{
	//XPSP1\NT\base\hals\halacpi\i386\ixclock.asm
	LONG TimeIncrement=eax;
	KTRAP_FRAME* TrFrame=ebp;
	LONG Zero=ebx;

	KUSER_SHARED_DATA* KUserSharedData=(KUSER_SHARED_DATA*)0xFFDF0000;
	ULONGLONG InterruptTime=*(ULONGLONG*)&KUserSharedData->InterruptTime;
	InterruptTime=InterruptTime+TimeIncrement;
	KUserSharedData->InterruptTime.High2Time=(ULONG)(InterruptTime>>32);
	*(ULONGLONG*)&KUserSharedData->InterruptTime=InterruptTime;

	ULONG OldTime=KeTickCount->LowPart;
	KiTickOffset=KiTickOffset-TimeIncrement;
	if (KiTickOffset<=0)
	{
		ULONGLONG SystemTime=*(ULONGLONG*)&KUserSharedData->SystemTime;
		SystemTime=SystemTime+KeTimeAdjustment;
		KUserSharedData->SystemTime.High2Time=(ULONG)(SystemTime>>32);
		*(ULONGLONG*)&KUserSharedData->SystemTime=SystemTime;

		ULONGLONG TickCount=*(ULONGLONG*)&KeTickCount;
		TickCount++;
		KeTickCount.High2Time=(ULONG)(TickCount>>32);
		*(ULONGLONG*)&KeTickCount=TickCount;

		if (KUserSharedData->TickCountLowDeprecated+1==0)	//TickCountLow���
			ExpTickCountAdjustmentCount++;

		KUserSharedData->TickCountLowDeprecated=KeTickCount.LowPart+
			ExpTickCountAdjustment*ExpTickCountAdjustmentCount;
		//...
	}

	//�м�Ĳ�������һЩDPC��صĲ������ɲο�NT5���룬���ﲻд��
	//Win2K3\NT\base\ntos\ke\ia64\clock.c
	//XPSP1\NT\base\ntos\ke\ia64\clock.c

	if (KiTickOffset<=0)
	{
		KiTickOffset=KiTickOffset+KeMaximumIncrement;
		KeUpdateRunTime(OldIrql);
		_asm cli;
		HalEndSystemInterrupt();
	}
	else
	{
		KeGetCurrentPrcb()->InterruptCount++;
		_asm cli;
		HalEndSystemInterrupt();
	}
}

VOID __stdcall KeUpdateRunTime_XP(KIRQL OldIrql)
{
	KTRAP_FRAME* TrFrame=ebp;
	//KPRCB��KPCRƫ��0x120��������ֱ����KPRCB
	KPRCB* Prcb=KeGetCurrentPrcb();
	Prcb->InterruptCount++;
	KTHREAD* Thread=Prcb->CurrentThread;
	KPROCESS* Process=Thread->ApcState.Process;
	if (TrFrame->EFlags&0x00020000 //Virtual 8086 Mode
		|| TrFrame->SegCs&1)	
	{
		//edx=1
		Prcb->UserTime++;
		Thread->UserTime++;
		InterlockedIncrement((LONG*)&Process->UserTime);
		//lea     ecx, [ecx+0]
		if (KiTimeUpdateNotifyRoutine!=NULL)
			KiTimeUpdateNotifyRoutine(PsGetCurrentThreadId());	//ecx=ETHREAD::UniqueThread
	}
	else
	{
		//edx=0
		Prcb->KernelTime++;
		if (OldIrql==DISPATCH_LEVEL && Prcb->DpcRoutineActive!=0)
		{
			Prcb->DpcTime++;
		}
		else if (OldIrql>DISPATCH_LEVEL)
		{
			Prcb->InterruptTime++;
		}
		else	//OldIrql<DISPATCH_LEVEL || (OldIrql==DISPATCH_LEVEL && Prcb->DpcRoutineActive==0)
		{
			Thread->KernelTime++;
			InterlockedIncrement((LONG*)&Process->KernelTime);
			if (KiTimeUpdateNotifyRoutine!=NULL)
				KiTimeUpdateNotifyRoutine(PsGetCurrentThreadId());
		}
	}
	//...
}
*/

typedef VOID (__fastcall*FNTIMEUPDATECALLBACK)(HANDLE ThreadId);
extern "C" NTSYSAPI VOID __fastcall KeSetTimeUpdateNotifyRoutine(FNTIMEUPDATECALLBACK TimeUpdateCallback);
KSYSTEM_TIME* KeTickCountAddr=NULL;

//���Խ׶Σ��Ȳ���KiTimeUpdateNotifyRoutine
VOID __fastcall KeTimeUpdateCallback(HANDLE ThreadId)
{
	KUSER_SHARED_DATA* KUserSharedData=(KUSER_SHARED_DATA*)0xFFDF0000;
	ULONGLONG TickCount=*(ULONGLONG*)KeTickCountAddr;
	KUserSharedData->TickCount.High2Time=(ULONG)(TickCount>>32);
	*(ULONGLONG*)&KUserSharedData->TickCount=TickCount;
}

void InitTickCount64Helper()
{
	UNICODE_STRING SymbolName;
	RtlInitUnicodeString(&SymbolName,L"KeTickCount");
	KeTickCountAddr=(KSYSTEM_TIME*)MmGetSystemRoutineAddress(&SymbolName);
	DbgPrint("KeTickCount Address:%08X\n",KeTickCountAddr);
	KeSetTimeUpdateNotifyRoutine(KeTimeUpdateCallback);
}

void UninitTickCount64Helper()
{
	ULONG* AsmAddr=(ULONG*)((ULONG)KeSetTimeUpdateNotifyRoutine+2);
	ULONG* KiTimeUpdateNotifyRoutine=(ULONG*)*AsmAddr;
	DbgPrint("KiTimeUpdateNotifyRoutine:%08X\n",KiTimeUpdateNotifyRoutine);
	*KiTimeUpdateNotifyRoutine=NULL;
}