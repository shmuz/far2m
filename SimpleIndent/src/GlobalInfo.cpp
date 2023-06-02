#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0X52D8EECB;
  aInfo->MinFarVersion = 0x00020004;
  aInfo->Version       = 0;
  aInfo->Title         = L"Simple Indent";
  aInfo->Description   = L"Simple Indent plugin for FAR Manager";
  aInfo->Author        = L"Alex Yaroslavsky, FAR People";
}
