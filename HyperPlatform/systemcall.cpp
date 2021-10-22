#include"include/exclusivity.h"
#include"ia32_type.h"
#include"systemcall.h"
#include"include/write_protect.h"

extern "C"
{
#include"kernel-hook/khook/khook/hk.h"
extern "C" void DetourKiSystemServiceStart();
NTSYSAPI const char* PsGetProcessImageFileName(PEPROCESS Process);

}


fpSystemCall SystemCallFake;
char SystemCallRecoverCode[15] = {};
NTSTATUS HookStatus = STATUS_UNSUCCESSFUL;


const char* GetSyscallProcess()
{
	return PsGetProcessImageFileName(IoGetCurrentProcess());
}


NTSTATUS InitSystemVar()
{
	/*
	* �������������vmlaunch֮ǰ��ʼ��һЩȫ�ֱ���
	* 
	* 1.����ں˻�ַ
	* 
	* 2.���KiSystemServiceStart�ĵ�ַ
	* 
	* 3.���SSDT Table�ĵ�ַ
	* 
	* 4.��ʼ��һ��α���guestҳ��
	*/


	KernelBase = GetKernelBase();
	
	PtrKiSystemServiceStart = (ULONG_PTR)&DetourKiSystemServiceStart;

	//KiSystemServiceCopyStart = OffsetKiSystemServiceCopyStart + KernelBase;
	KiSystemServiceStart = OffsetKiSystemServiceStart + KernelBase;

	aSYSTEM_SERVICE_DESCRIPTOR_TABLE = 
	(SYSTEM_SERVICE_DESCRIPTOR_TABLE*)(OffsetKeServiceDescriptorTable + KernelBase);

	SystemCallFake.Construct();

	return STATUS_SUCCESS;
}

void DoSystemCallHook()
{
	OriKiSystemServiceStart = (PVOID)((ULONG_PTR)KiSystemServiceStart + 0x14);
	auto exclusivity = ExclGainExclusivity();
	//
	//hook��ʱ������˾޴�bug��ept hide hook��ʱ��ǧ������jmp [prt]����Ȼ�ڶ�vm-exit��дvm-exit֮����ѭ��
	//
#if 0
	HookStatus = HkDetourFunction((PVOID)
		KiSystemServiceStart,
		(PVOID)PtrKiSystemServiceStart,
		&OriKiSystemServiceStart);
#endif
	//
	//push r15
	//mov r15,xx
	//jmp r15
	// 
	//r15:pop r15
	//
	char hook[] = { 0x41,0x57,0x49,0xBF,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x41,0xFF,0xE7 };
	memcpy(SystemCallRecoverCode, (PVOID)KiSystemServiceStart, sizeof(SystemCallRecoverCode));
	memcpy(hook + 4, &PtrKiSystemServiceStart, sizeof(PtrKiSystemServiceStart));
	auto irql = WPOFFx64();
	memcpy((PVOID)KiSystemServiceStart, hook, sizeof(hook));
	WPONx64(irql);

	ExclReleaseExclusivity(exclusivity);
}

//ֻ����SSDT����������ShadowSSDT
PVOID GetSSDTEntry(IN ULONG index)
{
	PSYSTEM_SERVICE_DESCRIPTOR_TABLE pSSDT = aSYSTEM_SERVICE_DESCRIPTOR_TABLE;
	PVOID pBase = (PVOID)KernelBase;

	if (pSSDT && pBase)
	{
		// Index range check ��shadowssdt��Ļ�����0
		if (index > pSSDT->NumberOfServices)
			return NULL;

		return (PUCHAR)pSSDT->ServiceTableBase + (((PLONG)pSSDT->ServiceTableBase)[index] >> 4);
	}

	return NULL;
}

void InitUserSystemCallHandler(decltype(&SystemCallHandler) UserHandler)
{
	UserSystemCallHandler = UserHandler;
}

void SystemCallHandler(KTRAP_FRAME * TrapFrame,ULONG SSDT_INDEX)
{

	//������¼�����˶��ٴ�ϵͳ���ã�����debug��ֻ�е�һ�ε�ʱ������
	static ULONG64 SysCallCount = 0;
	if (!SysCallCount) {
		Log("[SysCallCount]at %p\n", &SysCallCount);
		Log("[SYSCALL]%s\nIndex %x\nTarget %llx\n", GetSyscallProcess(), SSDT_INDEX, GetSSDTEntry(SSDT_INDEX));
	}
	SysCallCount++;

	//Ȼ��Ӧ�õ����û����Ĵ����������û���ṩ����ʹ��Ĭ�ϵ�

	if (UserSystemCallHandler)
	{
		UserSystemCallHandler(TrapFrame, SSDT_INDEX);
	}
}
