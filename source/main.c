#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <3ds.h>
#include <_cfg.h>
#include <_nim.h>

// only if reboot fails and config saved to evac
static void destroy_wifi_configs() {
	static u8 dummy1[0xC00] = {0};
	static u8 dummy2[0xC00] = {0};
	GetSetGuaranteeBlock(0xC00, 0x80000, 0xC, &dummy1, &dummy2);
	GetSetGuaranteeBlock(0xC00, 0x80001, 0xC, &dummy1, &dummy2);
	GetSetGuaranteeBlock(0xC00, 0x80002, 0xC, &dummy1, &dummy2);
}

#define SECINFOSIZE 273

typedef struct SecInfo {
	u8 Signature[256];
	union {
		struct {
			u8 Region;
			u8 Byte0x101;
			char Serial[15];
		};
		u8 Data[17];
	};
} SecInfo;

_Static_assert(sizeof(SecInfo) == SECINFOSIZE, "SecInfo bad size build");

static void print_account_info(NIM_AccountInformation* acc) {
	printf("Account ID: %s\n", acc->AccountId);
	printf("Account Status: %s\n", acc->AccountStatus);
	printf("Account Country: %s\n", acc->Country);
	printf("Service Standby Mode: %i\n", acc->ServiceStandbyMode);
}

typedef union {
	char str[2];
	u16 str16;
} IsoStr;

const s8 country_to_region_table[] = {
	-1,  0, -1, -1, -1, -1, -1, -1,  1,  1,  1,  1,  1,  1,  1,  1, 
	 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	 1,  1,  1,  1,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	 6, -1, -1, -1, -1, -1, -1, -1,  5, -1, -1, -1, -1, -1, -1, -1,
	 6, -1, -1, -1, -1, -1, -1, -1, -1,  1, -1, -1,  1, -1, -1, -1,
	 4, -1, -1, -1, -1, -1, -1, -1,  1,  2, -1, -1, -1, -1,  1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,  2,  2,  1
};

static const u8 default_languages[7] = {
	0, // Japanese
	1, // English
	1, // English
	1, // English
	6, // Simplified Chinese
	7, // Korean
	11 // Traditional Chinese
};

// 0  Japanese
// 1  English
// 2  French
// 3  German
// 4  Italian
// 5  Spanish
// 6  Simplified Chinese
// 7  Korean
// 8  Dutch
// 9  Portuguese
// 10 Russian
// 11 Traditional Chinese

static const u8 language_table[7][12] = {
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // JPN
	{0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0}, // USA
	{0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0}, // EUR
	{0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // AUS but, mostly a stub
	{0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0}, // CHN
	{0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0}, // KOR
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}  // TWN
};

static Result fix_language(u8 region) {
	if (region >= 7)
		region = CFG_REGION_JPN;

	u8 default_language = default_languages[region];
	u8 lang, dummy1;

	Result res = GetSetGuaranteeBlock(1, 0xA0002, 0xE, &default_language, &lang);
	if (R_FAILED(res)) return res;

	// at this point, default language was set to config blk
	// if returned language was invalid, just return
	// otherwise let it flow and set back the previous language
	if (lang >= 12 || !language_table[region][lang])
		return res;

	res = GetSetGuaranteeBlock(1, 0xA0002, 0xE, &lang, &dummy1);
	return res;
}

static bool evac_saved = false;

void unregister_outofregion() {
	// before starting, save config to /fixdata
	// this will make our changes to config blks non-permanent on reboot
	Result res = _CFGI_SaveConfigToEvac();
	if (R_FAILED(res)) {
		puts("Failed to save to evac");
		return;
	}

	evac_saved = true;

	void* mem = linearAlloc(0x20000);
	if (!mem) {
		puts("Failed to allocate buffer");
		return;
	}

	SecInfo secinfobak;
	bool secinfoset = false, passed = false;

	do {
		NIM_AccountInformation* account_info = NULL;

		res = _nimsInitConnect(mem, 0x20000, &account_info, 0x0004013000002C02LLU, 1234);
		if (R_FAILED(res)) {
			printf("Failed to connect: %08lx\n", res);
			u32 supportcode;
			Result res2 = _NIMS_GetSupportCode(&supportcode);
			if (R_FAILED(res2))
				printf("Failed to get support code: %08lx\n", res2);
			else
				printf("Support Code: %03lu-%04lu\n", supportcode / 10000u, supportcode % 10000u);
			break;
		}

		print_account_info(account_info);

		if (!account_info->AccountStatus) {
			puts("Account status null, stopping");
			break;
		}

		if (!strcmp(account_info->AccountStatus, "U")) {
			puts("Account already unregistered.");
			break;
		}

		if (!account_info->Country) {
			puts("Country null, stopping");
			break;
		}

		IsoStr country;
		country.str[0] = account_info->Country[0];
		country.str[1] = account_info->Country[1];

		u16 countrycode;
		res = _CFGU_GetCountryCodeID(country.str16, &countrycode);
		if (R_FAILED(res)) {
			printf("Failed to convert country: %08lx\n", res);
			break;
		}

		if (countrycode > sizeof(country_to_region_table) || country_to_region_table[countrycode] == -1) {
			puts("Unknown region for country");
			break;
		}

		u8 countryregion = country_to_region_table[countrycode];
		u8 consoleregion = 0xFF;
		res = _CFGU_SecureInfoGetRegion(&consoleregion);
		if (R_FAILED(res)) {
			printf("Failed to get console region: %08lx\n", res);
			break;
		}

		if (consoleregion == countryregion) {
			puts("Registry country region already same as console region");
			break;
		}

		SecInfo secinfo;
		res = _CFGI_SecureInfoGetSignature(&secinfo.Signature, sizeof(secinfo.Signature));
		if (!R_FAILED(res)) res = _CFGI_SecureInfoGetData(&secinfo.Data, sizeof(secinfo.Data));
		if (R_FAILED(res)) {
			printf("Failed to get secureinfo from config: %08lx\n", res);
			break;
		}

		memcpy(&secinfobak, &secinfo, sizeof(secinfo));

		if (secinfo.Region != consoleregion) {
			puts("Secinfo region doesn't match config region");
			break;
		}

		secinfo.Region = countryregion;

		res = _CFGI_SecureInfoSet(&secinfo.Signature, sizeof(secinfo.Signature), &secinfo.Data, sizeof(secinfo.Data));
		if (R_FAILED(res)) {
			printf("Failed to set secureinfo: %08lx\n", res);
			break;
		}

		secinfoset = true;

		// we didnt fix any signatures, but this app should be running under a hacked environment
		// the final secinfo will not be saved
		// once the secinfo was set, signature check variable was invalidated
		// config only does the check by itself once since boot, after that you'll need to be explicit
		// without running the check, operations with secinfo will fail
		res = _CFGI_SecureInfoVerifySignature();
		if (R_FAILED(res)) {
			printf("Signature check failed: %08lx\n", res);
			break;
		}

		res = _CFGU_SecureInfoGetRegion(&consoleregion);
		if (R_FAILED(res)) {
			printf("Failed to get console region: %08lx\n", res);
			break;
		}

		if (consoleregion != countryregion) {
			puts("Console region didnt change!");
			break;
		}

		u8 prevcountrycode[4];
		u8 _countrycode[4] = {0, 0, 0, countrycode & 0xFF};
		res = GetSetGuaranteeBlock(4, 0xB0000, 0xE, &_countrycode, &prevcountrycode);
		if (R_FAILED(res)) break;

		res = fix_language(countryregion);
		if (R_FAILED(res)) {
			printf("Failed to fix language: %08lx\n", res);
			break;
		}

		// because yes, we need to re-connect to update system information regards to region and language
		res = _nimsInitConnect(mem, 0x20000, &account_info, 0x0004013000002C02LLU, 1234);
		if (R_FAILED(res)) {
			printf("Failed to connect: %08lx\n", res);
			u32 supportcode;
			Result res2 = _NIMS_GetSupportCode(&supportcode);
			if (R_FAILED(res2))
				printf("Failed to get support code: %08lx\n", res2);
			else
				printf("Support Code: %03lu-%04lu\n", supportcode / 10000u, supportcode % 10000u);
			break;
		}

		res = _NIMS_Unregister();
		if (R_FAILED(res)) {
			printf("Failed to unregister: %08lx\n", res);
			u32 supportcode;
			Result res2 = _NIMS_GetSupportCode(&supportcode);
			if (R_FAILED(res2))
				printf("Failed to get support code: %08lx\n", res2);
			else
				printf("Support Code: %03lu-%04lu\n", supportcode / 10000u, supportcode % 10000u);
			break;
		}

		passed = true;
	} while(0);

	if (passed) {
		puts("Console unregistered!");
	}

	if (secinfoset) {
		// after this point we just hope for no errors
		// should be safe if we dont *explicitly* tell config to not save secinfo anyway
		// but I sleep easier knowing I tried to keep original data just in case
		res = _CFGI_SecureInfoSet(&secinfobak.Signature, sizeof(secinfobak.Signature), &secinfobak.Data, sizeof(secinfobak.Data));
		if (R_FAILED(res)) {
			printf("Failed to restore secureinfo: %08lx\n", res);
		} else {
			res = _CFGI_SecureInfoVerifySignature();
			if (R_FAILED(res)) {
				printf("Restored signature check failed: %08lx\n", res);
			}
		}
	}

	linearFree(mem);

	_nimsExit();
	return;
}

int main(int argc, char **argv)
{
	_cfgInit();
	nsInit();
	nwmExtInit();
	acInit();
	gfxInitDefault();

	PrintConsole bottomScreen;

	consoleInit(GFX_BOTTOM, &bottomScreen);
	consoleSelect(&bottomScreen);

	aptSetHomeAllowed(false);
	aptSetSleepAllowed(false);

	printf("nimregion\n\n");

	NWMEXT_ControlWirelessEnabled(true);

	bool executed_run = false;

	printf("Press the START button to exit.\n");
	printf("Press the X button to fix and reboot.\n");
	while (aptMainLoop())
	{
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();

		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START && !executed_run)
			break; // break in order to return to hbmenu

		if (kDown & KEY_X && !executed_run) {
			executed_run = true;
			unregister_outofregion();

			gfxFlushBuffers();
			gfxSwapBuffers();
			gspWaitForVBlank();

			sleep(5);

			if(R_FAILED(NS_RebootSystem())) {
				if (evac_saved) {
					NWMEXT_ControlWirelessEnabled(false);
					destroy_wifi_configs();
					ACI_LoadNetworkSetting(0);
					NWMEXT_ControlWirelessEnabled(true);
				}
				puts("Reboot failed, please force reboot");
			}
		}
	}

	gfxExit();
	acExit();
	nwmExtExit();
	nsExit();
	_cfgExit();
	return 0;
}
