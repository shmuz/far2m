#include <farplug-wide.h>

#ifdef FAR_MANAGER_FAR2M
SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xD2F36B62;
  aInfo->Version       = MAKEPLUGVERSION(0,0,0,0);
  aInfo->Title         = L"FarColorer";
  aInfo->Description   = L"Syntax highlighting in Far editor";
  aInfo->Author        = L"Igor Ruskih, FAR People";
}
#endif
