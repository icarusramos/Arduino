#include "wifi_drv.h"

// arduino includes
#include "wl_definitions.h"
#include "wl_types.h"
#include "wiring.h"

#ifdef __cplusplus
extern "C" {

// RTK includes
#include "main.h"
#include "wifi_conf.h"
#include "wifi_constants.h"
#include "wifi_structures.h"
#include "lwip_netconf.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"

extern struct netif xnetif[NET_IF_NUM]; 

}
#endif

// Array of data to cache the information related to the networks discovered
uint8_t WiFiDrv::_networkCount = 0;
char 	WiFiDrv::_networkSsid[][WL_SSID_MAX_LENGTH] = {{"1"},{"2"},{"3"},{"4"},{"5"}};
int32_t WiFiDrv::_networkRssi[WL_NETWORKS_LIST_MAXNUM] = { 0 };
uint8_t WiFiDrv::_networkEncr[WL_NETWORKS_LIST_MAXNUM] = { 0 };

static bool init_wlan = false;

static rtw_network_info_t wifi = {0};
static rtw_ap_info_t ap = {0};
static unsigned char password[65] = {0};

rtw_wifi_setting_t WiFiDrv::wifi_setting;

static void init_wifi_struct(void)
{
    memset(wifi.ssid.val, 0, sizeof(wifi.ssid.val));
    memset(wifi.bssid.octet, 0, ETH_ALEN);	
    memset(password, 0, sizeof(password));
    wifi.ssid.len = 0;
    wifi.password = NULL;
    wifi.password_len = 0;
    wifi.key_id = -1;
    memset(ap.ssid.val, 0, sizeof(ap.ssid.val));
    ap.ssid.len = 0;
    ap.password = NULL;
    ap.password_len = 0;
    ap.channel = 1;
}

void WiFiDrv::wifiDriverInit()
{
    if (init_wlan == false) {
        init_wlan = true;
        LwIP_Init();
        wifi_on(RTW_MODE_STA);
    }
}

int8_t WiFiDrv::wifiSetNetwork(char* ssid, uint8_t ssid_len)
{
	int ret;

    memset(wifi.bssid.octet, 0, ETH_ALEN);
    memcpy(wifi.ssid.val, ssid, ssid_len);
    wifi.ssid.len = ssid_len;

	wifi.security_type = RTW_SECURITY_OPEN;
    wifi.password = NULL;
    wifi.password_len = 0;
    wifi.key_id = 0;

    ret = wifi_connect((char*)wifi.ssid.val, wifi.security_type, (char*)wifi.password, wifi.ssid.len,
                    wifi.password_len, wifi.key_id, NULL);

    if (ret == RTW_SUCCESS) {

        LwIP_DHCP(0, DHCP_START);

        init_wifi_struct();

        return WL_SUCCESS;

    } else {

        init_wifi_struct();
        
        return WL_FAILURE;
    }
}

int8_t WiFiDrv::wifiSetPassphrase(char* ssid, uint8_t ssid_len, const char *passphrase, const uint8_t len)
{
	int ret;

    memset(wifi.bssid.octet, 0, ETH_ALEN);
    memcpy(wifi.ssid.val, ssid, ssid_len);
    wifi.ssid.len = ssid_len;

	wifi.security_type = RTW_SECURITY_WPA2_AES_PSK;
    memset(password, 0, sizeof(password));
    memcpy(password, passphrase, len);
    wifi.password = password;
    wifi.password_len = len;
    wifi.key_id = 0;

    ret = wifi_connect((char*)wifi.ssid.val, wifi.security_type, (char*)wifi.password, wifi.ssid.len,
                    wifi.password_len, wifi.key_id, NULL);

    if (ret == RTW_SUCCESS) {

        LwIP_DHCP(0, DHCP_START);

        init_wifi_struct();

        return WL_SUCCESS;

    } else {

        init_wifi_struct();
        
        return WL_FAILURE;
    }
}

int8_t WiFiDrv::wifiSetKey(char* ssid, uint8_t ssid_len, uint8_t key_idx, const void *key, const uint8_t len)
{
	int ret;
    int i, idx;
    const unsigned char* k = (const unsigned char *)key;

    memset(wifi.bssid.octet, 0, ETH_ALEN);
    memcpy(wifi.ssid.val, ssid, ssid_len);
    wifi.ssid.len = ssid_len;

	wifi.security_type = RTW_SECURITY_WEP_PSK;
    memset(password, 0, sizeof(password));

    // convert hex sring to hex value
    for (i=0, idx=0; i<len; i++) {

        if ( k[i] >= '0' && k[i] <= '9' ) {
            password[idx] += (k[i] - '0');
        } else if ( k[i] >= 'a' && k[i] <= 'f' ) {
            password[idx] += (k[i] - 'a' + 10);
        } else if ( k[i] >= 'A' && k[i] <= 'F' ) {
            password[idx] += (k[i] - 'A' + 10);
        }

        if (i % 2 == 0) {
            password[idx] *= 16;
        } else {
            idx++;
        }
    }

    wifi.password = password;
    wifi.password_len = len/2;
    wifi.key_id = key_idx;

    ret = wifi_connect((char*)wifi.ssid.val, wifi.security_type, (char*)wifi.password, wifi.ssid.len,
                    wifi.password_len, wifi.key_id, NULL);

    if (ret == RTW_SUCCESS) {

        LwIP_DHCP(0, DHCP_START);

        init_wifi_struct();

        return WL_SUCCESS;

    } else {

        init_wifi_struct();
        
        return WL_FAILURE;
    }

}

int8_t WiFiDrv::disconnect()
{
    wifi_disconnect();

    return WL_DISCONNECTED;
}

uint8_t WiFiDrv::getConnectionStatus()
{
    wifiDriverInit();

	if (wifi_is_connected_to_ap() == 0) {
		return WL_CONNECTED;
	} else {
		return WL_DISCONNECTED;
	}
}

uint8_t* WiFiDrv::getMacAddress()
{
    return LwIP_GetMAC(&xnetif[0]);
}

void WiFiDrv::getIpAddress(IPAddress& ip)
{
    ip = LwIP_GetIP(&xnetif[0]);
}

void WiFiDrv::getSubnetMask(IPAddress& mask)
{
    mask = LwIP_GetMASK(&xnetif[0]);
}

void WiFiDrv::getGatewayIP(IPAddress& ip)
{
    ip = LwIP_GetGW(&xnetif[0]);
}

char* WiFiDrv::getCurrentSSID()
{
    wifi_get_setting(WLAN0_NAME, &wifi_setting);
    
    return (char *)(wifi_setting.ssid);
}

uint8_t* WiFiDrv::getCurrentBSSID()
{
    uint8_t bssid[ETH_ALEN];
    wext_get_bssid(WLAN0_NAME, bssid);
    return bssid;
}

int32_t WiFiDrv::getCurrentRSSI()
{
	int rssi = 0;

	wifi_get_rssi(&rssi);

    return rssi;
}

uint8_t WiFiDrv::getCurrentEncryptionType()
{
    wifi_get_setting(WLAN0_NAME, &wifi_setting);

    return (wifi_setting.security_type);
}

rtw_result_t WiFiDrv::wifidrv_scan_result_handler( rtw_scan_handler_result_t* malloced_scan_result )
{
    rtw_scan_result_t* record;

	if (malloced_scan_result->scan_complete != RTW_TRUE) {
		record = &malloced_scan_result->ap_details;
		record->SSID.val[record->SSID.len] = 0; /* Ensure the SSID is null terminated */

        if ( _networkCount < WL_NETWORKS_LIST_MAXNUM ) {
            memcpy( _networkSsid[_networkCount], record->SSID.val, record->SSID.len );
            _networkRssi[_networkCount] = record->signal_strength;
            _networkEncr[_networkCount] = record->security;
            _networkCount++;
        }
	}

	return RTW_SUCCESS;
}

int8_t WiFiDrv::startScanNetworks()
{
    _networkCount = 0;
	if( wifi_scan_networks(wifidrv_scan_result_handler, NULL ) != RTW_SUCCESS ){
		return WL_FAILURE;
	}

    return WL_SUCCESS;
}

uint8_t WiFiDrv::getScanNetworks()
{
    return _networkCount;
}

char* WiFiDrv::getSSIDNetoworks(uint8_t networkItem)
{
	if (networkItem >= WL_NETWORKS_LIST_MAXNUM)
		return NULL;

	return _networkSsid[networkItem];
}

uint8_t WiFiDrv::getEncTypeNetowrks(uint8_t networkItem)
{
	if (networkItem >= WL_NETWORKS_LIST_MAXNUM)
		return NULL;

    uint8_t encType = 0;

    if ( _networkEncr[networkItem] == RTW_SECURITY_OPEN ) {
        encType = ENC_TYPE_NONE;
    } else if ( _networkEncr[networkItem] | AES_ENABLED ) {
        encType = ENC_TYPE_CCMP;
    } else if ( _networkEncr[networkItem] | TKIP_ENABLED ) {
        encType = ENC_TYPE_TKIP;
    } else if ( _networkEncr[networkItem] == RTW_SECURITY_WEP_PSK ) {
        encType = ENC_TYPE_WEP;
    }

    return encType;
}

int32_t WiFiDrv::getRSSINetoworks(uint8_t networkItem)
{
	if (networkItem >= WL_NETWORKS_LIST_MAXNUM)
		return NULL;

	return _networkRssi[networkItem];
}

char*  WiFiDrv::getFwVersion()
{
	// The version is for compatible to arduino example code
    return "1.1.0";
}

int WiFiDrv::getHostByName(const char* aHostname, IPAddress& aResult)
{
	ip_addr_t ip_addr;
	err_t err;
	err = netconn_gethostbyname(aHostname, &ip_addr);

	if (err != ERR_OK) {
	  	return WL_FAILURE;
	}
		
	return WL_SUCCESS;
}