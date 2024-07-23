#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 3,25,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x894EAABB;
  aInfo->Version       = Version;
  aInfo->Title         = L"Calculator";
  aInfo->Description   = L"Calculator plugin";
  aInfo->Author        = L"Igor Ruskih, uncle-vunkis, FAR People";
}
