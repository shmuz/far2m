#include <farplug-wide.h>

#ifdef FAR_MANAGER_FAR2M
SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 0,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xC941E865;
  aInfo->Version       = Version;
  aInfo->Title         = L"DrawLine";
  aInfo->Description   = L"Draw lines for Far Manager";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
}
#endif
