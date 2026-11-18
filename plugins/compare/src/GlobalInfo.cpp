#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->MinFarVersion = MAKEFARVERSION2(2,9,0,0);
  aInfo->Version       = MAKEFARVERSION2(0,0,0,0);
  aInfo->SysID         = 0x4F6BDE22;
  aInfo->Title         = L"Compare";
  aInfo->Description   = L"Advanced File Compare plugin";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
  aInfo->UseMenuGuids  = 0;
}
