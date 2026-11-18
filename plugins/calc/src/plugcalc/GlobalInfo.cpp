#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->MinFarVersion = MAKEFARVERSION2(2,9,0,0);
  aInfo->Version       = MAKEFARVERSION2(3,25,0,0);
  aInfo->SysID         = 0x894EAABB;
  aInfo->Title         = L"Calculator";
  aInfo->Description   = L"Calculator plugin";
  aInfo->Author        = L"Igor Ruskih, uncle-vunkis, FAR People";
  aInfo->UseMenuGuids  = 0;
}
