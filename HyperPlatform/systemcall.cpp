#include"systemcall.h"
#include"util.h"
#include"ia32_type.h"
#include"log.h"
#include"include/exclusivity.h"
#include"ept.h"

extern "C"
{
#include"kernel-hook/khook/khook/hk.h"
extern "C" void DetourKiSystemCall64Shadow();
extern "C" void DetourKiSystemServiceCopyEnd();
extern "C" void DetourOtherKiSystemServiceCopyEnd();
extern "C" void DetourKiSystemServiceCopyStart();
extern "C" void DetourKiSystemServiceStart();
NTSYSAPI const char* PsGetProcessImageFileName(PEPROCESS Process);

}

FakePage SystemFakePage;

ULONG_PTR
fulsh_insn_cache(
	_In_ ULONG_PTR Argument
)
{
#if 1
	Log("flush insn cache!\n");
#endif
	//aZwFlushInstructionCache();
	return true;
}

const char* GetSyscallProcess()
{
	return PsGetProcessImageFileName(IoGetCurrentProcess());
}

NTSTATUS InitSystemVar()
{
	//
	//��ʼ���ں˻�ַ
	//
	KernelBase = GetKernelBase();
	
	KiSystemCall64Shadow = UtilReadMsr64(Msr::kIa32Lstar);
	PtrDetourKiSystemCall64Shadow = (ULONG_PTR)&DetourKiSystemCall64Shadow;
	//PtrKiSystemServiceCopyEnd = (ULONG_PTR)&DetourKiSystemServiceCopyEnd;
	//PtrKiSystemServiceCopyStart = (ULONG_PTR)&DetourKiSystemServiceCopyStart;
	OtherPtrKiSystemServiceCopyEnd = (ULONG_PTR)&DetourOtherKiSystemServiceCopyEnd;
	PtrKiSystemServiceStart = (ULONG_PTR)&DetourKiSystemServiceStart;

	//KiSystemServiceCopyEnd = OffsetKiSystemServiceCopyEnd + KernelBase;
	OtherKiSystemServiceCopyEnd = OffsetKiSystemServiceCopyEnd + KernelBase + 0x20;
	//KiSystemServiceCopyStart = OffsetKiSystemServiceCopyStart + KernelBase;
	KiSystemServiceStart = OffsetKiSystemServiceStart + KernelBase;

	aSYSTEM_SERVICE_DESCRIPTOR_TABLE = 
	(SYSTEM_SERVICE_DESCRIPTOR_TABLE*)(OffsetKeServiceDescriptorTable + KernelBase);

#ifdef DBG
	Log("KiSystemCall64Shadow at %llx\n", KiSystemCall64Shadow);
#endif // DEBUG

	if (!KiSystemCall64Shadow)
		return STATUS_UNSUCCESSFUL;

	KiSystemCall64ShadowCommon = KiSystemCall64Shadow + 0x2D;

	SystemFakePage.GuestVA = (PVOID)((KiSystemServiceStart >>12) << 12);
	SystemFakePage.PageContent = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, 'a');
	if (!SystemFakePage.PageContent)
		return STATUS_UNSUCCESSFUL;
	memcpy(SystemFakePage.PageContent, SystemFakePage.GuestVA,PAGE_SIZE);
	SystemFakePage.GuestPA = MmGetPhysicalAddress(SystemFakePage.GuestVA);
	SystemFakePage.PageContentPA = MmGetPhysicalAddress(SystemFakePage.PageContent);

	return STATUS_SUCCESS;
}

void DoSystemCallHook()
{

#if 0 //������KPTI֮����������ˣ���Ϊ�û����̲���������Ĵ����ӳ�䣬����ִ�в���
	UtilWriteMsr64(Msr::kIa32Lstar, (ULONG64)DetourKiSystemCall64Shadow);
	DbgBreakPoint();
#endif

	//
	//���Բ���ֱ��hook KiSystemCall64 Ȼ����ept�����ڴ�
	//
	//һ���ĵ���hook��ͷ����ȡ��ֻ�ܲ�ȡhook����
#if 0
	auto exclusivity = ExclGainExclusivity();
	HkDetourFunction((PVOID)KiSystemCall64Shadow, (PVOID)PtrDetourKiSystemCall64Shadow, NULL);
	ExclReleaseExclusivity(exclusivity);
#endif

	//�漰��ȫ�ֱ������ض�λ���Ƚ��鷳
#if 0
	auto exclusivity = ExclGainExclusivity();
	HkDetourFunction((PVOID)KiSystemServiceCopyEnd, (PVOID)PtrKiSystemServiceCopyEnd, &OriKiSystemServiceCopyEnd);
	ExclReleaseExclusivity(exclusivity);
#endif

	//����Ҳ�е�ɵ��
#if 0
	auto exclusivity = ExclGainExclusivity();
	HkDetourFunction((PVOID)
		KiSystemServiceCopyStart, 
		(PVOID)PtrKiSystemServiceCopyStart, 
		&OriKiSystemServiceCopyStart);
	ExclReleaseExclusivity(exclusivity);
#endif

	//����ط�Ҳ���У���call�ĵط�������߳��л��ѷ��ص�ַ���ڶ�ջ�����Ƿ��ص�ַ�����Ѿ���hook������
	//�ٻ���ִ�оͱ��ˣ��ر��Ǹ�Ƶ�����ĵط�
#if 0
	auto exclusivity = ExclGainExclusivity();
	HkDetourFunction(
		(PVOID)OtherKiSystemServiceCopyEnd, 
		(PVOID)OtherPtrKiSystemServiceCopyEnd,
		&OriOtherKiSystemServiceCopyEnd);
	KeIpiGenericCall(fulsh_insn_cache, NULL);
	ExclReleaseExclusivity(exclusivity);
#endif

	auto exclusivity = ExclGainExclusivity();
	HkDetourFunction((PVOID)
		KiSystemServiceStart,
		(PVOID)PtrKiSystemServiceStart,
		&OriKiSystemServiceStart);
	ExclReleaseExclusivity(exclusivity);
}

//ֻ����SSDT����������ShadowSSDT
PVOID GetSSDTEntry(IN ULONG index)
{
	ULONG size = 0;
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

#if 1
	//������¼�����˶��ٴ�ϵͳ���ã�����debug��ֻ�е�һ�ε�ʱ������
	static ULONG64 SysCallCount = 0;
	if (!SysCallCount) {
		Log("[SysCallCount]at %p\n", &SysCallCount);
		Log("[SYSCALL]%s\nIndex %x\nTarget %llx\n", GetSyscallProcess(), SSDT_INDEX, GetSSDTEntry(SSDT_INDEX));
	}
	SysCallCount++;
#endif

	//Ȼ��Ӧ�õ����û����Ĵ����������û���ṩ����ʹ��Ĭ�ϵ�

	if (UserSystemCallHandler)
	{
		UserSystemCallHandler(TrapFrame, SSDT_INDEX);
	}
}
