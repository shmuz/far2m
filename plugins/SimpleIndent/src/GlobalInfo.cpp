#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->MinFarVersion = MAKEFARVERSION2(2,9,0,0);
  aInfo->Version       = MAKEFARVERSION2(2,2,0,0);
  aInfo->SysID         = 0X52D8EECB;
  aInfo->Title         = L"Simple Indent";
  aInfo->Description   = L"Simple Indent plugin for FAR editor";
  aInfo->Author        = L"Vladimir Panteleev, Alex Yaroslavsky, FAR People";
  aInfo->UseMenuGuids  = 0;
}
