#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xDEEC52C3;
  aInfo->MinFarVersion = 0x00020004;
  aInfo->Version       = 0;
  aInfo->Title         = L"AutoWrap";
  aInfo->Description   = L"Auto wrap for Far Manager";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
}
