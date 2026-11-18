#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->MinFarVersion = MAKEFARVERSION2(2,9,0,0);
  aInfo->Version       = MAKEFARVERSION2(0,0,0,0);
  aInfo->SysID         = 0x65642111;
  aInfo->Title         = L"ArcLite";
  aInfo->Description   = L"Archive support for Far Manager (based on 7-Zip project)";
  aInfo->Author        = L"Eugene Roshal & Far Group, anta999";
  aInfo->UseMenuGuids  = 0;
}
