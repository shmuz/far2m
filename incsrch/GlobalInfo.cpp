#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x84C0C745;
  aInfo->MinFarVersion = 0x00020004;
  aInfo->Version       = 0;
  aInfo->Title         = L"Incremental Search";
  aInfo->Description   = L"Incremental Search";
  aInfo->Author        = L"Stanislav Mekhanoshin, FAR People";
}
