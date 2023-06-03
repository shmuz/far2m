#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x84C0C745;
  aInfo->MinFarVersion = MAKEFARVERSION(2,4);
  aInfo->Version       = MAKEPLUGVERSION(2,1,0,0);
  aInfo->Title         = L"Incremental Search";
  aInfo->Description   = L"Incremental Search";
  aInfo->Author        = L"Stanislav Mekhanoshin, FAR People";
}
