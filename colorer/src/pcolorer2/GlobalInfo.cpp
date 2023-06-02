#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xD2F36B62;
  aInfo->MinFarVersion = 0x00020004;
  aInfo->Version       = 0;
  aInfo->Title         = L"FarColorer";
  aInfo->Description   = L"Syntax highlighting in Far editor";
  aInfo->Author        = L"Igor Ruskih, FAR People";
}
