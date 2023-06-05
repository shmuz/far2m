#include <farplug-wide.h>

#ifdef FAR_MANAGER_FAR2M
SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 0,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xD2F36B62;
  aInfo->Version       = Version;
  aInfo->Title         = L"FarColorer";
  aInfo->Description   = L"Syntax highlighting in Far editor";
  aInfo->Author        = L"Igor Ruskih, FAR People";
}
#endif
