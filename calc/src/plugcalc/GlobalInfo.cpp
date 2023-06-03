#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x894EAABB;
  aInfo->MinFarVersion = MAKEFARVERSION(2,4);
  aInfo->Version       = MAKEPLUGVERSION(3,25,0,0);
  aInfo->Title         = L"Calculator";
  aInfo->Description   = L"Calculator plugin for FAR manager";
  aInfo->Author        = L"Igor Ruskih, uncle-vunkis, FAR People";
}
