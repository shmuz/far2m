#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 2,1,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x84C0C745;
  aInfo->Version       = Version;
  aInfo->Title         = L"Incremental Search";
  aInfo->Description   = L"Incremental Search in the editor";
  aInfo->Author        = L"Stanislav Mekhanoshin, FAR People";
}
