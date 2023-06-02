#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xAE8CE351;
  aInfo->MinFarVersion = 0x00020004;
  aInfo->Version       = 0;
  aInfo->Title         = L"NetRocks";
  aInfo->Description   = L"Adds SFTP/SCP/NFS/SMB/WebDAV connectivity to far2l";
  aInfo->Author        = L"elfmz";
}
