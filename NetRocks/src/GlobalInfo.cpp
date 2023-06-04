#include <farplug-wide.h>

#ifdef FAR_MANAGER_FAR2M
SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xAE8CE351;
  aInfo->Version       = MAKEPLUGVERSION(0,0,0,0);
  aInfo->Title         = L"NetRocks";
  aInfo->Description   = L"Adds SFTP/SCP/NFS/SMB/WebDAV connectivity to far2l";
  aInfo->Author        = L"elfmz";
}
#endif
