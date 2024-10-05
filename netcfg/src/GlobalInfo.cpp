#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 0,7,1,20241005 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xE873426D;
  aInfo->Version       = Version;
  aInfo->Title         = L"NetCfg";
  aInfo->Description   = L"NetCfg plugin";
  aInfo->Author        = L"VPROFi";
}
