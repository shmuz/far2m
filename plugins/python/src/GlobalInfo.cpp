#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->MinFarVersion = MAKEFARVERSION2(2,9,0,0);
  aInfo->Version       = MAKEFARVERSION2(0,0,0,0);
  aInfo->SysID         = 0x7E9585C2;
  aInfo->Title         = L"Python";
  aInfo->Description   = L"Python plugin";
  aInfo->Author        = L"Grzegorz Makarewicz";
  aInfo->UseMenuGuids  = 0;
}
