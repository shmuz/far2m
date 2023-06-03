#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xFF5449D0;
  aInfo->MinFarVersion = 0x00020004;
  aInfo->Version       = 0;
  aInfo->Title         = L"Inside";
  aInfo->Description   = L"Shows what is inside ELF and some other file formats";
  aInfo->Author        = L"elfmz";
}
