#include <farplug-wide.h>

#ifdef FAR_MANAGER_FAR2M
SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x14CA31E6;
  aInfo->Version       = MAKEPLUGVERSION(0,0,0,0);
  aInfo->Title         = L"MultiArc";
  aInfo->Description   = L"Archive support plugin for FAR Manager";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
}
#endif
