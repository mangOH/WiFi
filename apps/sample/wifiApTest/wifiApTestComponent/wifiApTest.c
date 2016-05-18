 /**
  * This module implements a test for Wifi Access Point
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

#include <signal.h>
#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
//                                       Test Function Variable defines
//--------------------------------------------------------------------------------------------------
#define TEST_SSID_STR "wifiApSSID"

#define TEST_SECU_PROTO  LE_WIFIAP_SECURITY_WPA2
#define TEST_PASSPHRASE "passphrase"



//--------------------------------------------------------------------------------------------------
// PREREQUISITE!!     IP HANDLING
// Please note that IP handling is not provided by the Wifi Service,
// but the following is provided as an example of how the user can setup IP.
// For the following to work the file :
//  /etc/dnsmasq.d/wifiAP.conf
// must contain the following:
// _________________
// dhcp-range=wlan0,192.168.10.10,192.168.10.199,24h
// EOF
// _________________
// Then reboot! Because this is read at startup.
//--------------------------------------------------------------------------------------------------

//# IP & mask of subnet created on the wlan
#define  SUBNET "192.168.0.0/24"
#define  HOST_IP "192.168.10.1"

#define IP_RANGE_START "192.168.10.10"
#define IP_RANGE_END "192.168.10.20"
#define MASK "255.255.255.0"



//--------------------------------------------------------------------------------------------------
//                                       Internal defines
//--------------------------------------------------------------------------------------------------

//# interface du the Access Point
#define INT_WIFI "wlan0"

// defines because the SSID is actually uint8 not char.
#define TEST_SSID     ((const uint8_t *) TEST_SSID_STR)
#define TEST_SSID_NBR_BYTES     (sizeof(TEST_SSID_STR)-1)

//# interface wlan to eth0 that has access to Internet
#define INT_NET "eth0"


//--------------------------------------------------------------------------------------------------
/**
 * Event handler reference.
 */
//--------------------------------------------------------------------------------------------------
static le_wifiAp_NewEventHandlerRef_t HdlrRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * runs the command
 */
//--------------------------------------------------------------------------------------------------
static void RunSystemCommand
(
    char * commandString
)
{
    int16_t systemResult;

    if( NULL == commandString )
    {
        LE_ERROR( "RunSystemCommand ERROR Parameter is NULL" );
        return;
    }

    systemResult = system( commandString );
    // Return value of -1 means that the fork() has failed (see man system).
    if ( 0 == WEXITSTATUS( systemResult ) )
    {
        LE_INFO("RunSystemCommand Success: %s", commandString);
    }
    else
    {
        LE_ERROR( "RunSystemCommand Error %s Failed: (%d)", commandString, systemResult );
    }

}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the credentials
 * and bridge the wlan0 to ethernet
 */
//--------------------------------------------------------------------------------------------------
static void Testle_startDhcpServerAndbridgeConnection
(
    void
)
{
    RunSystemCommand("/sbin/ifconfig " INT_WIFI " " HOST_IP " up");

    //# DNSMASQ Startup
//    RunSystemCommand( "/usr/bin/dnsmasq --interface=" INT_WIFI
//                     " --dhcp-range=" IP_RANGE_START "," IP_RANGE_END ",12h -d --bogus-priv&");

    //# Turn on IP forwarding (faire suivre les paquets d'une interface à l'autre)
    RunSystemCommand("echo 1 > /proc/sys/net/ipv4/ip_forward");
    //# load masquerade module
    RunSystemCommand("/sbin/modprobe ipt_MASQUERADE");

    RunSystemCommand("/usr/sbin/iptables -A POSTROUTING -t nat -o " INT_NET " -j MASQUERADE");
    RunSystemCommand("/usr/sbin/iptables -A FORWARD --match state "
                      "--state RELATED,ESTABLISHED --jump ACCEPT");
    RunSystemCommand("/usr/sbin/iptables -A FORWARD -i " INT_WIFI " --destination " SUBNET
                    " --match state --state NEW --jump ACCEPT");


}

//! [SetCred]
//--------------------------------------------------------------------------------------------------
/**
 * Sets the credentials
 */
//--------------------------------------------------------------------------------------------------
static void Testle_setCredentials
(
    void
)
{
    LE_ASSERT(LE_OK == le_wifiAp_SetPassPhrase ( TEST_PASSPHRASE ));

    LE_ASSERT(LE_OK == le_wifiAp_SetSecurityProtocol ( TEST_SECU_PROTO ));

    LE_ASSERT(LE_OK == le_wifiAp_SetDiscoverable ( true ));
}
//! [SetCred]

//! [Subscribe]
//--------------------------------------------------------------------------------------------------
/**
 * Handler for Wifi Client changes
 *
 * @param event
 *        Handles the wifi events
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
static void myMsgHandler
(
    le_wifiAp_Event_t event,
    void* contextPtr
)
{
    LE_INFO( "Wifi Ap event received");
    switch( event )
    {
        case LE_WIFIAP_EVENT_CLIENT_CONNECTED:
        {
            ///< A client connect to AP
            LE_INFO( "LE_WIFIAP_EVENT_CLIENT_CONNECTED");
        }
        break;

        case LE_WIFIAP_EVENT_CLIENT_DISCONNECTED:
        {
            ///< A client connect to AP
            LE_INFO( "LE_WIFICLIENT_EVENT_DISCONNECTED");
        }
        break;


        default:
            LE_ERROR( "ERROR Unknown event %d", event);
        break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Tests the Wifi Access Point.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_wifiApStart
(
    void
)
{
    LE_INFO( "Start Test Wifi Access Point");

    // Add an handler function to handle message reception
    HdlrRef=le_wifiAp_AddNewEventHandler( myMsgHandler, NULL );

    LE_ASSERT(HdlrRef != NULL);

    LE_ASSERT(LE_OK == le_wifiAp_SetSsid( TEST_SSID, TEST_SSID_NBR_BYTES ));

    Testle_setCredentials();

    if( LE_OK == le_wifiAp_Start() )
    {
        LE_INFO( "le_wifiAp_Start OK");

        Testle_startDhcpServerAndbridgeConnection();
    }
    else
    {
        LE_ERROR( "le_wifiAp_Start ERROR");
    }

    LE_ASSERT(LE_OK == le_wifiAp_SetIpRange("192.168.10.1",
                                            "192.168.10.10",
                                            "192.168.10.100"));
}
//! [Subscribe]


//--------------------------------------------------------------------------------------------------
/**
 * Stop the WiFi AP
 */
//--------------------------------------------------------------------------------------------------
static void Testle_wifiApStop
(
    int signalId
)
{
    LE_INFO( "WIFI AP STOP : Received signal %d", signalId );

    // Stop the AP
    le_wifiAp_Stop();

    // Turn off IP forwarding
    LE_INFO( "WIFI AP STOP - Disabling IP forwarding" );
    RunSystemCommand("echo 0 > /proc/sys/net/ipv4/ip_forward");
    // Removing masquerade modules
    LE_INFO( "WIFI AP STOP - Removing the masquerading module..." );
    RunSystemCommand("modprobe ipt_MASQUERADE");

    // Turn off the iptables
    RunSystemCommand( "iptables -t nat -f" );
    RunSystemCommand( "iptables -t mangle -F" );
    RunSystemCommand( "iptables -F" );
    RunSystemCommand( "iptables -X" );

    // Flush the IP address of the wlan0 interface
    RunSystemCommand( "ip addr flush dev " INT_WIFI );
}

//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Wifi Init
    LE_INFO( "======== Wifi Ap Test  ========");

    putenv("PATH=/legato/systems/current/bin:/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin");

    signal(SIGINT, Testle_wifiApStop);
    signal(SIGTERM, Testle_wifiApStop);

    Testle_wifiApStart();
}
