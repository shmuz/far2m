#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xC941E865;
  aInfo->MinFarVersion = 0x00020004;
  aInfo->Version       = 0;
  aInfo->Title         = L"DrawLine";
  aInfo->Description   = L"Draw lines for Far Manager";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
}
