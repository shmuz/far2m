#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->MinFarVersion = MAKEFARVERSION2(2,9,0,0);
  aInfo->Version       = MAKEFARVERSION2(1,4,0,0);
  aInfo->SysID         = 0xD2F36B62;
  aInfo->Title         = L"FarColorer";
  aInfo->Description   = L"Syntax highlighting in Far editor";
  aInfo->Author        = L"Igor Ruskih, Dobrunov Aleksey, Eugene Efremov, FAR People";
  aInfo->UseMenuGuids  = 0;
}
