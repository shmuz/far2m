#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->MinFarVersion = MAKEFARVERSION2(2,9,0,0);
  aInfo->Version       = MAKEFARVERSION2(2,3,0,0);
  aInfo->SysID         = 0x84C0C745;
  aInfo->Title         = L"Incremental Search";
  aInfo->Description   = L"Incremental Search in the editor";
  aInfo->Author        = L"Stanislav Mekhanoshin, FAR People";
  aInfo->UseMenuGuids  = 0;
}
