#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0X52D8EECB;
  aInfo->Version       = MAKEPLUGVERSION(2,2,0,0);
  aInfo->Title         = L"Simple Indent";
  aInfo->Description   = L"Simple Indent plugin for FAR Manager";
  aInfo->Author        = L"Vladimir Panteleev, Alex Yaroslavsky, FAR People";
}
