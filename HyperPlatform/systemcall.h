#include"include/stdafx.h"
#include"include/PDBSDK.h"

typedef struct _SYSTEM_SERVICE_DESCRIPTOR_TABLE
{
	PULONG_PTR ServiceTableBase;
	PULONG ServiceCounterTableBase;
	ULONG_PTR NumberOfServices;
	PUCHAR ParamTableBase;
} SYSTEM_SERVICE_DESCRIPTOR_TABLE, * PSYSTEM_SERVICE_DESCRIPTOR_TABLE;


//��Ҫ��asm�ļ�ʹ��
extern "C"
{
	inline const ULONG KernelSize = 0xa6e000; //hard signature
	inline ULONG_PTR KernelBase = NULL;


	inline ULONG_PTR KiSystemServiceStart = NULL;
	inline ULONG_PTR PtrKiSystemServiceStart = NULL;
	inline PVOID OriKiSystemServiceStart = NULL;
	inline PSYSTEM_SERVICE_DESCRIPTOR_TABLE aSYSTEM_SERVICE_DESCRIPTOR_TABLE = NULL;

	void SystemCallHandler(KTRAP_FRAME* TrapFrame, ULONG SSDT_INDEX);
	ULONG_PTR GetKernelBase();
	const char* GetSyscallProcess();

	decltype(&SystemCallHandler) UserSystemCallHandler = NULL;

	void InitUserSystemCallHandler(decltype(&SystemCallHandler) UserHandler);
}


//
//��vm��ʼ��֮ǰ��ʼ����Ҫ�ı���
//

NTSTATUS InitSystemVar();

void DoSystemCallHook();

PVOID GetSSDTEntry(IN ULONG index);