
#include "common.h"

/*
�ִ�����ϵͳӵ���豸�Ȳ�ε�������plug and play��������������ʱ��̬����Ӳ��
���оͰ���CPU���ĵ����Ӻ��Ƴ������ú�����
��ˣ�ϵͳ����ʱ��⵽�����CPU����������һ���Ǿ��Ե�����
Ҳ��һ�����ִ��CPU������Ҳ��һ���ǿ��õ�CPU��������Щ������������ʱ�仯
Win7�Դ��ṩ��һ����֧�֣�Ԥ��һЩ�ռ䣬��CPU���ú�������ʱ����ActiveCount
��XP��ȫ��֧�֣�����CPU��������=�������=��������

����ͨ������CPU�׺��ԣ�affinity�����߳�ֻ������ָ���ĺ�����
Windowsʹ��һ��DWORD_PTR��ʾ���ĵ����룬�������00000010�����������ڵ�1��������
��00001101��ʾ���������ں���0������2�ͺ���3�ϣ�����һ�ָ�Чʵ�õķ���
����32λϵͳ��DWORD_PTR���ܱ�ʾ32�����ģ�64λϵͳ��Ҳֻ�ܱ�ʾ64������
Ϊ�˽����һ���⣬������CPU����趨����64��CPU���ı��һ�飬�����+����ȷ��CPU
32λ�ĺ�Win7��ǰ�Ĳ���ϵͳ��֧��CPU�飬Ĭ��ʹ�õ�0�飬���32����64������
�ʹ�������һ����ָ��CPU���API������API��Ȼ���ݣ�ֻ�ܲ���Ĭ�ϵĵ�0��

SetThreadGroupAffinity 
GetThreadGroupAffinity 
GetProcessGroupAffinity 
GetLogicalProcessorInformation 
GetLogicalProcessorInformationEx 
NtQuerySystemInformationEx
...
*/

WORD WINAPI K32GetActiveProcessorGroupCount()
{
	return 1;
}

WORD WINAPI K32GetMaximumProcessorGroupCount()
{
	return 1;
}

DWORD WINAPI K32GetActiveProcessorCount(WORD GroupNumber)
{
	if (GroupNumber!=0)
		return 0;
	SYSTEM_BASIC_INFORMATION SysInfo;
	NTSTATUS Result=NtQuerySystemInformation(SystemBasicInformation,&SysInfo,sizeof(SysInfo),NULL);
	if (!NT_SUCCESS(Result))
		return 0;
	DWORD CoreCount=0;
	for (int i=0;i<32;i++)
	{
		CoreCount+=(SysInfo.ActiveProcessorsAffinityMask&1);
		SysInfo.ActiveProcessorsAffinityMask>>=1;
	}
	return CoreCount;
}

DWORD WINAPI K32GetMaximumProcessorCount(WORD GroupNumber)
{
	if (GroupNumber!=0)
		return 0;
	SYSTEM_BASIC_INFORMATION SysInfo;
	NTSTATUS Result=NtQuerySystemInformation(SystemBasicInformation,&SysInfo,sizeof(SysInfo),NULL);
	if (!NT_SUCCESS(Result))
		return 0;
	return SysInfo.NumberOfProcessors;
}

//��Ҫxpextk.sys֧��
//��ű�������GDT�ĵ�8���14��19λ�ϣ���Ring3��lslָ���ȡ
//0x3B������=7����=GDT��RPL=Ring3
_declspec(naked)
	DWORD WINAPI RtlGetCurrentProcessorNumber()
{
	_asm 
	{
		mov  ecx, 0x3B;
		lsl  eax, ecx;
		shr  eax, 0x0E;
		retn ;
	}
}

_declspec(naked)
	void WINAPI RtlGetCurrentProcessorNumberEx(PPROCESSOR_NUMBER ProcNumber)
{
	_asm
	{
		mov  edi, edi;
		push  ebp;
		mov  ebp, esp;
		mov  edx, dword ptr ss:[ebp+8];  //ProcNumber
		xor  eax, eax;
		mov  [edx], ax;  //ProcNumber->Group
		mov  ecx, 0x3B;
		lsl  eax, ecx;
		shr  eax, 0x0E;
		mov  [edx+2], al;  //ProcNumber->Number
		mov  byte ptr [edx+3], 0;  //ProcNumber->Reserved
		pop  ebp;
		retn 4;
	}
}




