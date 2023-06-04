#include <farplug-wide.h>

#ifdef FAR_MANAGER_FAR2M
SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xB076F0B0;
  aInfo->Version       = MAKEPLUGVERSION(0,0,0,0);
  aInfo->Title         = L"Align";
  aInfo->Description   = L"Align block for Far Manager";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
}
#endif

