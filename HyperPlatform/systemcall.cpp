#include"systemcall.h"
#include"util.h"
#include"ia32_type.h"
#include"log.h"
#include"include/exclusivity.h"

extern "C"
{
#include"kernel-hook/khook/khook/hk.h"
extern "C" void DetourKiSystemCall64Shadow();
extern "C" void DetourKiSystemServiceCopyEnd();
extern "C" void DetourOtherKiSystemServiceCopyEnd();
extern "C" void DetourKiSystemServiceCopyStart();
extern "C" void DetourKiSystemServiceStart();
}

void ZwFlushInstructionCache();

using ZwFlushInstructionCacheType = decltype(&ZwFlushInstructionCache);
ZwFlushInstructionCacheType aZwFlushInstructionCache = NULL;


ULONG_PTR
fulsh_insn_cache(
	_In_ ULONG_PTR Argument
)
{
#if 1
	Log("flush insn cache!\n");
#endif
	aZwFlushInstructionCache();
	return true;
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

#ifdef DBG
	Log("KiSystemCall64Shadow at %llx\n", KiSystemCall64Shadow);
#endif // DEBUG

	if (!KiSystemCall64Shadow)
		return STATUS_UNSUCCESSFUL;

	KiSystemCall64ShadowCommon = KiSystemCall64Shadow + 0x2D;

	return STATUS_SUCCESS;
}

void DoSystemCallHook()
{

	aZwFlushInstructionCache = (ZwFlushInstructionCacheType)(KernelBase + OffsetZwFlushInstructionCache);

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

void SystemCallHandler(ULONG64 ssdt_func_index)
{
	Log("hello world\n");
}
