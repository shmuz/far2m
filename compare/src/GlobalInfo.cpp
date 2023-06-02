#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x4F6BDE22;
  aInfo->MinFarVersion = 0x00020004;
  aInfo->Version       = 0;
  aInfo->Title         = L"Compare";
  aInfo->Description   = L"Advanced File Compare for Far Manager";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
}
