#include <farplug-wide.h>

#ifdef FAR_MANAGER_FAR2M
SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 0,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xE873426D;
  aInfo->Version       = Version;
  aInfo->Title         = L"NetCfg";
  aInfo->Description   = L"NetCfg plugin for Far Manager";
  aInfo->Author        = L"VPROFi";
}
#endif
