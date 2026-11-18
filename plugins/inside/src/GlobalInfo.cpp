#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->MinFarVersion = MAKEFARVERSION2(2,9,0,0);
  aInfo->Version       = MAKEFARVERSION2(0,0,0,0);
  aInfo->SysID         = 0xFF5449D0;
  aInfo->Title         = L"Inside";
  aInfo->Description   = L"Shows what is inside ELF and some other file formats";
  aInfo->Author        = L"elfmz";
  aInfo->UseMenuGuids  = 0;
}
