#pragma once
#include <3ds/types.h>

typedef struct NIM_MoneyCurrencyPair {
	char Amount[32];
	char Currency[32];
} NIM_MoneyCurrencyPair;

typedef struct NIM_TicketIdVersionPair {
	s64 TicketID;
	s32 Version;
} NIM_TicketIdVersionPair;

typedef struct NIM_TicketVersionList {
	NIM_TicketIdVersionPair *Entries;
	u32 EntryCount;
} NIM_TicketVersionList;

typedef struct NIM_AccountInformation {
	char *AccountId;
	char *AccountStatus;
	NIM_MoneyCurrencyPair *Balance;
	s64 *EulaVersion;
	s64 *LatestEulaVersion;
	char *Country;
	char *LoyaltyLoginNameOrExtAccountId;
	char *STDeviceToken;
	char *WTDeviceToken;
	int ServiceStandbyMode;
	int IVSSyncFlag;
	NIM_TicketVersionList *OwnedTickets;
} NIM_AccountInformation;

Result _nimsInit();
void _nimsExit(void);
Result _NIMS_GetInitializeResult(void);
Result _NIMS_GetSupportCode(u32* supportcode);
Result _NIMS_Unregister(void);
Result _NIMS_RegisterSelf(void);
Result _NIMS_SetAttribute(const char *attr, const char *val);
Result _NIMS_ConnectNoTicketDownload(void *buffer, size_t buffer_len, NIM_AccountInformation** account_info);
Result _nimsInitConnect(void *buffer, size_t buffer_len, NIM_AccountInformation** account_info, u64 appid, s32 tin);

