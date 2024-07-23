#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 2,2,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0X52D8EECB;
  aInfo->Version       = Version;
  aInfo->Title         = L"Simple Indent";
  aInfo->Description   = L"Simple Indent plugin for FAR editor";
  aInfo->Author        = L"Vladimir Panteleev, Alex Yaroslavsky, FAR People";
}
