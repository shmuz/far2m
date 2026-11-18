#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->MinFarVersion = MAKEFARVERSION2(2,9,0,0);
  aInfo->Version       = MAKEFARVERSION2(0,7,1,20241005);
  aInfo->SysID         = 0xE873426D;
  aInfo->Title         = L"NetCfg";
  aInfo->Description   = L"NetCfg plugin";
  aInfo->Author        = L"VPROFi";
  aInfo->UseMenuGuids  = 0;
}
