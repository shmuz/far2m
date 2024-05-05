#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 1,4,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xD2F36B62;
  aInfo->Version       = Version;
  aInfo->Title         = L"FarColorer";
  aInfo->Description   = L"Syntax highlighting in Far editor";
  aInfo->Author        = L"Igor Ruskih, Dobrunov Aleksey, Eugene Efremov, FAR People";
}
