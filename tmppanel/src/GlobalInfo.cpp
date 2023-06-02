#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xB77C964B;
  aInfo->MinFarVersion = 0x00020004;
  aInfo->Version       = 0;
  aInfo->Title         = L"TmpPanel";
  aInfo->Description   = L"Temporary Panel for Far Manager";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
}
