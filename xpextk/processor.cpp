
#include <ntddk.h>

/*
��Ҫ֪���̵߳�ǰ�������ĸ�CPU�����ϣ�ÿ�����ľ���Ҫһ������Ŀռ䣬������¼�Ͷ�ȡID
���õ�ѡ����ֻ��CPU��GDT��IDT��MSR���Լ�����ϵͳ��KPCR��64��������Ҫ6bit�洢
Win7��KPCR��¼��CPUÿ�����ĵı���Լ���ţ����ں����API��ѯ
����KCPR��Ring0��Ring3����ʾ�Ҫ�������ӵ�ϵͳ���ã�Ring3��ҪѰ�Ҹ���Ч�ķ���

���ں��е�������ͬ��Ring3����ͨ��lslָ���öν��ޣ�����Win7ѡ����GDT��¼ID
���һ���εĳ��Ⱥ̣ܶ��ν��޾��ò������ߵ�λ���ճ����Ĳ��ֿ���������¼CPU���
���TEB���Ǹ��ξ�����Ҫ����������������̶���Ring3���������ܷ��ʵ�
��ǰ�����Ӽ�λ���öν��ޱ�ʵ�ʴ�һЩ����û��ʲô���⣬�����޸����������Ŀ��
Ҫ���ľ�����Ring0��GDT�ĵ�8���14��19λ�����ϱ�ţ���Ring3��lslָ���ȡ

Win7�Ժ��64λϵͳ��CPU���飬����Ҫ��¼��ţ�32λ�����Ϊ0��
�����һ��WORD��Ӧ��Ϊ16bit��������CPU����Ҳ��WORD��ʾ�����=65536/64=1024������Ҫ10bit
Win7 x64����ż�¼�ڶν��޵ĵ�10λ���ټ��ϸ�6λ��CPU��ţ�ֻʣ���м�4λ����
����64λCPU��Ʒ����˱仯���ε�ַĬ�ϴ�0��ʼ������ȫ����ַ�ռ䣬���ټ��ν���
������������ƣ�Win7 x64��GDT�������٣�TEB���ڶ�Ҳ���е���������������10

��û���ҵ�Win7�ں�ʵ�ֵĻ����룬�����Ǵ�GetCurrentProcessorNumberEx�Ļ���Ʋ����
�������������ģ�����PCHunter��GDT��ʶ�������⣬��ʾ������
*/

#pragma pack(1)
typedef struct _GDTR
{
	USHORT Limit;
	ULONG Base;
} GDTR;

typedef struct _GDT
{
	USHORT Limit0_15;
	USHORT Base0_15;
	UCHAR Base16_23;
	UCHAR Type:4;
	UCHAR S:1;
	UCHAR DPL:2;
	UCHAR P:1;
	UCHAR Limit16_19:4;
	UCHAR AVL:1;
	UCHAR L:1;
	UCHAR D_B:1;
	UCHAR G:1;
	UCHAR Base24_31;
} GDT;

typedef struct _SELECTOR
{
	USHORT RPL:2;
	USHORT TI:1;
	USHORT Index:13;
} SELECTOR;
#pragma pack()

void InitProcessorIdHelper()
{
	ULONG core=KeNumberProcessors;
	if (core>32)
		core=32;
	for (ULONG i=0;i<core;i++)
	{
		KeSetSystemAffinityThread(1<<i);
		//KPCR��GDT�ֶ�Ҳ���Ի��
		GDTR gdtr={0,0};
		_asm sgdt gdtr;
		GDT* gdt=(GDT*)gdtr.Base;
		DbgPrint("core%d GDT at:0x%08X\n",i,gdtr.Base);
		//14λ��19λ�洢CPU���ĺţ�limit�������޴�1MB��Ϊ16KB
		gdt[7].Limit0_15=((i&3)<<14)|gdt[7].Limit0_15;
		gdt[7].Limit16_19=(i>>2)&0x0F;
	}
	KeRevertToUserAffinityThread();
}

void UninitProcessorIdHelper()
{
	ULONG core=KeNumberProcessors;
	if (core>32)
		core=32;
	for (ULONG i=0;i<core;i++)
	{
		KeSetSystemAffinityThread(1<<i);
		GDTR gdtr={0,0};
		_asm sgdt gdtr;
		GDT* gdt=(GDT*)gdtr.Base;
		gdt[7].Limit0_15=gdt[7].Limit0_15&0x3FFF;
		gdt[7].Limit16_19=0;
	}
	KeRevertToUserAffinityThread();
}