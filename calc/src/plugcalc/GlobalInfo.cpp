#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x894EAABB;
  aInfo->MinFarVersion = 0x00020004;
  aInfo->Version       = 0;
  aInfo->Title         = L"Calculator";
  aInfo->Description   = L"Calculator plugin for FAR manager";
  aInfo->Author        = L"Igor Ruskih, uncle-vunkis, FAR People";
}
