#pragma once 
#include<ntdef.h>

struct FakePage
{
    PVOID GuestVA;//Ҫfake��guest���Ե�ַ
    PHYSICAL_ADDRESS GuestPA;
    PVOID PageContent;//�������ҳ�����Ϣ����vmlaunch֮ǰ����,Ҳ����guest�ܿ�����ҳ������
    PHYSICAL_ADDRESS PageContentPA;
};

struct ICFakePage
{
    virtual void Construct() = 0;
    virtual void Destruct() = 0;
    FakePage fp;
};
