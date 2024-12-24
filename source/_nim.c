#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include <_nim.h>

static Handle nimsHandle = 0;
static int nimsRefCount = 0;

Result _nimsInit() {
	Result res;
	if (AtomicPostIncrement(&nimsRefCount)) return 0;
	res = srvGetServiceHandle(&nimsHandle, "nim:s");
	if (R_FAILED(res)) AtomicDecrement(&nimsRefCount);

	return res;
}

void _nimsExit(void) {
	if (AtomicDecrement(&nimsRefCount)) return;
	svcCloseHandle(nimsHandle);
}

Result _NIMS_GetInitializeResult(void)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x3f, 0, 0); // 0x003f0000

	if (R_FAILED(ret = svcSendSyncRequest(nimsHandle))) return ret;

	return (Result)cmdbuf[1];
}

Result _NIMS_GetSupportCode(u32* supportcode)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x31, 0, 0); // 0x00310000

	if (R_FAILED(ret = svcSendSyncRequest(nimsHandle))) return ret;

	if (supportcode) *supportcode = cmdbuf[2];

	return (Result)cmdbuf[1];
}

Result _NIMS_Unregister(void)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0xe, 0, 0); // 0x000e0000

	if (R_FAILED(ret = svcSendSyncRequest(nimsHandle))) return ret;

	return (Result)cmdbuf[1];
}

Result _NIMS_RegisterSelf(void)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x3c, 0, 2); // 0x003c0002
	cmdbuf[1] = IPC_Desc_CurProcessId();
	cmdbuf[2] = IPC_Desc_MoveHandles(1);

	if (R_FAILED(ret = svcSendSyncRequest(nimsHandle))) return ret;

	return (Result)cmdbuf[1];
}

Result _NIMS_SetAttribute(const char *attr, const char *val)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0xb, 2, 4); // 0x000b0084
	cmdbuf[1] = strlen(attr) + 1;
	cmdbuf[2] = strlen(val) + 1;
	cmdbuf[3] = IPC_Desc_StaticBuffer(cmdbuf[1], 0);
	cmdbuf[4] = (u32)attr;
	cmdbuf[5] = IPC_Desc_StaticBuffer(cmdbuf[2], 1);
	cmdbuf[6] = (u32)val;

	if (R_FAILED(ret = svcSendSyncRequest(nimsHandle))) return ret;

	return (Result)cmdbuf[1];
}

Result _NIMS_ConnectNoTicketDownload(void *buffer, size_t buffer_len, NIM_AccountInformation** account_info)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x57, 2, 2); // 0x00570082
	cmdbuf[1] = (u32)buffer; // buffer used internally like a heap, pointers fixed on return with this as reference
	cmdbuf[2] = buffer_len;
	cmdbuf[3] = IPC_Desc_Buffer(buffer_len, IPC_BUFFER_W);
	cmdbuf[4] = (u32)buffer;

	if (R_FAILED(ret = svcSendSyncRequest(nimsHandle))) return ret;

	// based on request's cmdbuf[1], all pointers are fixed to point into the buffer we passed
	// this pointer is within buffer and mustn't be used if buffer is freed
	// also nim doesn't guarantee a NULL ptr if it fails, it may be garbage data on error
	if (account_info) 
		*account_info = R_FAILED((Result)cmdbuf[1]) ? NULL : (NIM_AccountInformation*)cmdbuf[2];

	return (Result)cmdbuf[1];
}

Result _nimsInitConnect(void *buffer, size_t buffer_len, NIM_AccountInformation** account_info, u64 appid, s32 tin)
{
	Result res;
	
	if (R_FAILED(res = _nimsInit())) {
		return res;
	}

	if (R_FAILED(res = _NIMS_GetInitializeResult())) {
		return res;
	}

	if (appid) {
		// NIM always converts with "%016llx"
		char _appid[17];
		snprintf(_appid, 17, "%016llx", appid);
		if (R_FAILED(res = _NIMS_SetAttribute("AppId", _appid))) {
			return res;
		}
	} else {
		if (R_FAILED(res = _NIMS_RegisterSelf())) {
			return res;
		}
	}

	if (tin) { // not sure if tin can be 0, I'll assume not, that'd be only server check
		// NIM always converts with "%d", max size 11 (counting terminator byte)
		char _tin[11];
		snprintf(_tin, 11, "%ld", tin);
		if (R_FAILED(res = _NIMS_SetAttribute("TIN", _tin))) {
			return res;
		}
	} else {
		// default nim tin
		if (R_FAILED(res = _NIMS_SetAttribute("TIN", "1234"))) {
			return res;
		}
	}

	if (R_FAILED(res = _NIMS_ConnectNoTicketDownload(buffer, buffer_len, account_info))) {
		return res;
	}

	return res;
}
