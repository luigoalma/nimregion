#pragma once
#include <3ds/types.h>

#define CFG_BLKID_NOT_FOUND MAKERESULT(RL_PERMANENT, RS_WRONGARG, RM_CONFIG, RD_NOT_FOUND)

Result _cfgInit(void);
void _cfgExit(void);
Result _CFGS_GetConfigInfoBlk4(u32 size, u32 blkID, volatile void* outData);
Result _CFGS_SetConfigInfoBlk4(u32 size, u32 blkID, volatile const void* inData);
Result _CFGI_CreateConfigInfoBlk(u32 size, u32 blkID, u16 blkFlags, const void* inData);
Result _CFGU_SecureInfoGetRegion(u8* region);
Result _CFGI_SecureInfoSet(void* signature, size_t sigsize, void* data, size_t datasize);
Result _CFGI_SecureInfoVerifySignature();
Result _CFGI_SecureInfoGetData(void* data, size_t size);
Result _CFGI_SecureInfoGetSignature(void* signature, size_t size);
Result _CFGI_SaveConfigToEvac(void);
Result _CFGU_GetCountryCodeID(u16 string, u16* code);

Result GetSetGuaranteeBlock(u32 size, u32 blkID, u16 blkFlags, const void* inData, void* outData);

