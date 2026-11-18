#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->MinFarVersion = MAKEFARVERSION2(2,9,0,0);
  aInfo->Version       = MAKEFARVERSION2(0,0,0,0);
  aInfo->SysID         = 0xDEEC52C3;
  aInfo->Title         = L"AutoWrap";
  aInfo->Description   = L"Auto wrap for Far editor";
  aInfo->Author        = L"Eugene Roshal, FAR Group, FAR People";
  aInfo->UseMenuGuids  = 0;
}
