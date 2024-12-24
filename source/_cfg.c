#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include <_cfg.h>

static Handle cfgHandle;
static int cfgRefCount;

Result _cfgInit(void)
{
	Result ret;

	if (AtomicPostIncrement(&cfgRefCount)) return 0;

	// cfg:i has the most commands, then cfg:s, then cfg:u
	ret = srvGetServiceHandle(&cfgHandle, "cfg:i");
	//if(R_FAILED(ret)) ret = srvGetServiceHandle(&cfgHandle, "cfg:s"); // need even higher for all
	//if(R_FAILED(ret)) ret = srvGetServiceHandle(&cfgHandle, "cfg:u"); // not useful here
	if(R_FAILED(ret)) AtomicDecrement(&cfgRefCount);

	return ret;
}

void _cfgExit(void)
{
	if (AtomicDecrement(&cfgRefCount)) return;
	svcCloseHandle(cfgHandle);
}

Result _CFGS_GetConfigInfoBlk4(u32 size, u32 blkID, volatile void* outData)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x401,2,2); // 0x4010082
	cmdbuf[1] = size;
	cmdbuf[2] = blkID;
	cmdbuf[3] = IPC_Desc_Buffer(size,IPC_BUFFER_W);
	cmdbuf[4] = (u32)outData;

	if(R_FAILED(ret = svcSendSyncRequest(cfgHandle)))return ret;

	return (Result)cmdbuf[1];
}

Result _CFGS_SetConfigInfoBlk4(u32 size, u32 blkID, volatile const void* inData)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x402,2,2); // 0x4020082
	cmdbuf[1] = blkID;
	cmdbuf[2] = size;
	cmdbuf[3] = IPC_Desc_Buffer(size,IPC_BUFFER_R);
	cmdbuf[4] = (u32)inData;

	if(R_FAILED(ret = svcSendSyncRequest(cfgHandle)))return ret;

	return (Result)cmdbuf[1];
}

Result _CFGI_CreateConfigInfoBlk(u32 size, u32 blkID, u16 blkFlags, const void* inData)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x804,3,2); // 0x80400C2
	cmdbuf[1] = blkID;
	cmdbuf[2] = size;
	cmdbuf[3] = blkFlags;
	cmdbuf[4] = IPC_Desc_Buffer(size,IPC_BUFFER_R);
	cmdbuf[5] = (u32)inData;

	if(R_FAILED(ret = svcSendSyncRequest(cfgHandle)))return ret;

	return (Result)cmdbuf[1];
}

Result _CFGU_SecureInfoGetRegion(u8* region)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x2,0,0); // 0x20000

	if(R_FAILED(ret = svcSendSyncRequest(cfgHandle)))return ret;

	*region = (u8)cmdbuf[2] & 0xFF;

	return (Result)cmdbuf[1];
}

Result _CFGI_SecureInfoSet(void* signature, size_t sigsize, void* data, size_t datasize)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x811,2,4); // 0x8110084
	cmdbuf[1] = datasize;
	cmdbuf[2] = sigsize;
	cmdbuf[3] = IPC_Desc_Buffer(datasize, IPC_BUFFER_R);
	cmdbuf[4] = (u32)data;
	cmdbuf[5] = IPC_Desc_Buffer(sigsize, IPC_BUFFER_R);
	cmdbuf[6] = (u32)signature;

	if(R_FAILED(ret = svcSendSyncRequest(cfgHandle)))return ret;

	return (Result)cmdbuf[1];
}

Result _CFGI_SecureInfoVerifySignature()
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x813,0,0); // 0x8130000

	if(R_FAILED(ret = svcSendSyncRequest(cfgHandle)))return ret;

	return (Result)cmdbuf[1];
}

Result _CFGI_SecureInfoGetData(void* data, size_t size)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x814,1,2); // 0x8140042
	cmdbuf[1] = size;
	cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_W);
	cmdbuf[3] = (u32)data;

	if(R_FAILED(ret = svcSendSyncRequest(cfgHandle)))return ret;

	return (Result)cmdbuf[1];
}

Result _CFGI_SecureInfoGetSignature(void* signature, size_t size)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x815,1,2); // 0x8150042
	cmdbuf[1] = size;
	cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_W);
	cmdbuf[3] = (u32)signature;

	if(R_FAILED(ret = svcSendSyncRequest(cfgHandle)))return ret;

	return (Result)cmdbuf[1];
}

Result _CFGI_SaveConfigToEvac(void)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x819,0,0); // 0x8190000

	if(R_FAILED(ret = svcSendSyncRequest(cfgHandle)))return ret;

	return (Result)cmdbuf[1];
}

Result _CFGU_GetCountryCodeID(u16 string, u16* code)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0xA,1,0); // 0xA0040
	cmdbuf[1] = (u32)string;

	if(R_FAILED(ret = svcSendSyncRequest(cfgHandle)))return ret;

	*code = (u16)cmdbuf[2] & 0xFFFF;

	return (Result)cmdbuf[1];
}

Result GetSetGuaranteeBlock(u32 size, u32 blkID, u16 blkFlags, const void* inData, void* outData) {
	Result res = _CFGS_GetConfigInfoBlk4(size, blkID, outData);

	if (res == CFG_BLKID_NOT_FOUND) {
		printf("Blk id 0x%08lx missing, creating!\n", blkID);
		res = _CFGI_CreateConfigInfoBlk(size, blkID, blkFlags, inData);
		if (!R_FAILED(res)) {
			memcpy(outData, inData, size);
			return 0;
		}
	}

	if (R_FAILED(res)) {
		printf("Failed to get or to create 0x%08lx. %08lX\n", blkID, res);
		return res;
	}

	if (!memcmp(outData, inData, size)) return 0;
	res = _CFGS_SetConfigInfoBlk4(size, blkID, inData);
	if (R_FAILED(res)) {
		printf("CFGS_SetConfigInfoBlk4 failed. 0x%08lx / %08lX\n", blkID, res);
		return res;
	}
	return 0;
}
