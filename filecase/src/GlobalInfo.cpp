#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xADAC3050;
  aInfo->MinFarVersion = 0x00020004;
  aInfo->Version       = 0;
  aInfo->Title         = L"FileCase";
  aInfo->Description   = L"File names case conversion for Far Manager";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
}
