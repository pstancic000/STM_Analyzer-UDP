#include "SerialAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "SerialAnalyzer.h"
#include "SerialAnalyzerSettings.h" // FOr UDP getters
#include <iostream>
#include <sstream>
#include <string> // For std::string
#include <stdio.h>

#include <cstring> // For std::strlen, std::memcpy
#include <sys/types.h>
// WINDOWS SPECIFIC

#ifdef _MSC_VER
#include <winsock2.h>
#include <ws2tcpip.h>

#endif

// LINUX SPECIFIC
#ifdef __GNUC__

#include <netinet/in.h>
#include <arpa/inet.h>  // For sockaddr_in and inet_addr
#include <sys/socket.h> // For socket functions
#include <unistd.h>     // For close

#endif

#include <vector>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <cstdlib>


// Added for windows
#pragma comment( lib, "ws2_32.lib" ) // Link with ws2_32.lib for Winsock

// void SerialAnalyzerResults::


void SerialAnalyzerResults::ConvertHexStringToBytes( const char* hexString, std::vector<uint8_t>& byteArray )
{
    size_t length = std::strlen( hexString );
    if( length % 2 != 0 )
    {
        // std::cerr << "Hex string length must be even." << std::endl;
        return;
    }

    for( size_t i = 0; i < length; i += 2 )
    {
        std::string byteString( hexString + i, 2 ); // Create a string from two characters
        uint8_t byte = static_cast<uint8_t>( std::stoul( byteString, nullptr, 16 ) );
        byteArray.push_back( byte );
    }
}

void SerialAnalyzerResults::ConvertHexStringToBytes3( const char* hexString, std::vector<uint8_t>& byteArray )
{
    size_t length = std::strlen( hexString );
    if( length % 2 != 0 )
    {
        std::cerr << "Hex string length must be even." << std::endl;
        return;
    }

    auto HexCharToByte = []( char c ) -> uint8_t
    {
        if( c >= '0' && c <= '9' )
            return c - '0';
        else if( c >= 'A' && c <= 'F' )
            return c - 'A' + 10;
        else if( c >= 'a' && c <= 'f' )
            return c - 'a' + 10;
        else
            throw std::invalid_argument( "Invalid hex character" );
    };

    byteArray.clear();               // Clear the vector before filling it
    byteArray.reserve( length / 2 ); // Reserve space for the resulting bytes

    for( size_t i = 0; i < length; i += 2 )
    {
        try
        {
            uint8_t highNibble = HexCharToByte( hexString[ i ] );
            uint8_t lowNibble = HexCharToByte( hexString[ i + 1 ] );
            uint8_t byte = ( highNibble << 4 ) | lowNibble;
            byteArray.push_back( byte );
        }
        catch( const std::exception& e )
        {
            std::cerr << "Error converting hex string: " << e.what() << std::endl;
            return;
        }
    }
}

void SerialAnalyzerResults::AppendToBuffer( const char* source, char* destination, size_t& currentLength, size_t maxLength )
{
    size_t sourceLength = std::strlen( source );

    // Check if there is enough space left in the buffer
    if( currentLength + sourceLength < maxLength )
    {
        std::memcpy( destination + currentLength, source, sourceLength );
        currentLength += sourceLength;
        destination[ currentLength ] = '\0'; // Null-terminate the buffer
    }
    else
    {
        // Handle the overflow case
        // std::cerr << "Buffer overflow, cannot append data." << std::endl;
        return;
    }
}
// LINUX/UNIX SPECIFIC FUNCTION FOR SENDING WITH UDP
// Creating socket and sending with UDP
#ifdef __GNUC__
void SerialAnalyzerResults::SendUDPMessage( const void* data, size_t length, const char* ip, int port )
{
    int sockfd;
    struct sockaddr_in ServerAddr;

    if( ( sockfd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
    {
        std::cerr << "Socket creation failed." << std::endl;
        exit( EXIT_FAILURE );
    }

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons( port );
    ServerAddr.sin_addr.s_addr = inet_addr( ip );

    int n = sendto( sockfd, data, length, MSG_CONFIRM, ( const struct sockaddr* )&ServerAddr, sizeof( ServerAddr ) );

    if( n < 0 )
    {
        std::cerr << "Sending UDP message failed." << std::endl;
        close( sockfd );
        exit( EXIT_FAILURE );
    }

    close( sockfd );
}
#endif

// WINDOWS SPECIFIC FUNCTION FOR SENDING WITH UDP
#ifdef _MSC_VER
void SerialAnalyzerResults::SendUDPMessage( const void* data, size_t length, const char* ip, int port )
{
    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in ServerAddr;
    int n;

    // Initialize Winsock
    if( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 )
    {
        // std::cerr << "WSAStartup failed." << std::endl;
        // exit(EXIT_FAILURE);
        return;
    }

    // Create a UDP socket
    if( ( sockfd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) == INVALID_SOCKET )
    {
        // std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        // exit(EXIT_FAILURE);
        return;
    }

    // Set up the server address structure
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons( port );
    InetPton( AF_INET, ip, &ServerAddr.sin_addr ); // Convert IP address from string to binary form

    // Send the UDP message
    n = sendto( sockfd, static_cast<const char*>( data ), static_cast<int>( length ), 0, reinterpret_cast<const sockaddr*>( &ServerAddr ),
                sizeof( ServerAddr ) );

    if( n == SOCKET_ERROR )
    {
        // std::cerr << "Sending UDP message failed: " << WSAGetLastError() << std::endl;
        closesocket( sockfd );
        WSACleanup();
        // exit(EXIT_FAILURE);
        return;
    }

    // Clean up
    closesocket( sockfd );
    WSACleanup();
}
#endif
void SerialAnalyzerResults::ConcatBuffers( const char* SourceBuffer, char* DestinationBuffer )
{
    const int BufferSize = 2000;
    // Bilo je u kodu zbog razmaka
    // std::strncat( DestinationBuffer, " ", BufferSize - std::strlen( DestinationBuffer ) - 1 );
    std::strncat( DestinationBuffer, SourceBuffer, BufferSize - std::strlen( DestinationBuffer ) - 1 );
}

void SerialAnalyzerResults::CleanBuffer( char* Buffer )
{
    std::memset( Buffer, 0, sizeof( Buffer ) );
}


void SerialAnalyzerResults::CopyStringCommand_STM( int CMD_ID_Value, char* Buffer )
{
    switch( CMD_ID_Value )
    {
    case 0:
        strcpy( Buffer, "\n\r HOSTIF-HWRESET.Request, Perform a hardware reset of the platform. Reset can be performed "
                        "also through the RESET pin" );
        break;

    case 1:
        strcpy( Buffer, "\n\r HOSTIF-HWRESET.Confirm, Perform a hardware reset of the platform. Reset can be performed "
                        "also through the RESET pin" );
        break;

    case 2:
        strcpy( Buffer, "\n\r HOSTIF-MODESET.Request, Change the mode of the platform" );
        break;

    case 3:
        strcpy( Buffer, "\n\r HOSTIF-MODESET.Confirm, Change the mode of the platform" );
        break;

    case 4:
        strcpy( Buffer, "\n\r HOSTIF-MODEGET.Request, Get the current operating mode of the platform" );
        break;

    case 5:
        strcpy( Buffer, "\n\r HOSTIF-MODEGET.Confirm, Get the current operating mode of the platform" );
        break;

    case 6:
        strcpy( Buffer, "\n\r HOSTIF-BAUDRATESET.Request, Change the UART host interface baudrate of the platform" );
        break;

    case 7:
        strcpy( Buffer, "\n\r HOSTIF-BAUDRATESET.Confirm, Change the UART host interface baudrate of the platform" );
        break;

    case 8:
        strcpy( Buffer, "\n\r HOSTIF-RESETSTATE.Request, Reset the host interface state flushing all the messages in the "
                        "internal queue" );
        break;

    case 9:
        strcpy( Buffer, "\n\r HOSTIF-RESETSTATE.Confirm, Reset the host interface state flushing all the messages in the "
                        "internal queue" );
        break;

    case 10:
        strcpy( Buffer, "\n\r HOSTIF-TESTSET.Request, Request to enter in a PLC test or to exit" );
        break;

    case 11:
        strcpy( Buffer, "\n\r HOSTIF-TESTSET.Confirm, Request to enter in a PLC test or to exit" );
        break;

    case 12:
        strcpy( Buffer, "\n\r HOSTIF-TESTGET.Request, Request to get the status of the current PLC test" );
        break;

    case 13:
        strcpy( Buffer, "\n\r HOSTIF-TESTGET.Confirm, Request to get the status of the current PLC test" );

    case 14:
        strcpy( Buffer, "\n\r HOSTIF-SFLASH.Request, Perform an operation on the external SPI Flash if connected to the G3 platform" );
        break;

    case 15:
        strcpy( Buffer, "\n\r HOSTIF-SFLASH.Confirm, Perform an operation on the external SPI Flash if connected to the G3 platform" );
        break;

    case 16:
        strcpy( Buffer, "\n\r HOSTIF-NVM.Request, Perform an operation on the external NVM if an external SPI Flash is "
                        "connected to the G3 platform" );
        break;

    case 17:
        strcpy( Buffer, "\n\r HOSTIF-NVM.Confirm, Perform an operation on the external NVM if an external SPI Flash is "
                        "connected to the G3 platform" );
        break;

    case 18:
        strcpy( Buffer, "\n\r HOSTIF-TRACE.Request, Change the configuration of the trace module" );
        break;

    case 19:
        strcpy( Buffer, "\n\r HOSTIF-TRACE.Confirm, Change the configuration of the trace module" );
        break;

    case 20:
        strcpy( Buffer, "\n\r HOSTIF-DBGTOOL.Request, Perform information and debug operation on the G3 platform" );
        break;

    case 21:
        strcpy( Buffer, "\n\r HOSTIF-DBGTOOL.Confirm, Perform information and debug operation on the G3 platform" );
        break;

    case 22:
        strcpy( Buffer, "\n\r HOSTIF-SEC-INIT.Request, Request operation for the secure host interface" );
        break;

    case 23:
        strcpy( Buffer, "\n\r HOSTIF-SEC-INIT.Confirm, Request operation for the secure host interface" );
        break;

    case 24:
        strcpy( Buffer, "\n\r HOSTIF-RFCONFIGSET.Request, Configure RF device parameters (Radio, Packet format, CSMA, Power" );
        break;

    case 25:
        strcpy( Buffer, "\n\r HOSTIF-RFCONFIGSET.Confirm, Configure RF device parameters (Radio, Packet format, CSMA, Power" );
        break;

    case 26:
        strcpy( Buffer, "\n\r HOSTIF-RFCONFIGGET.Request, Get current RF device parameters (Radio, Packet format, CSMA, Power" );
        break;

    case 27:
        strcpy( Buffer, "\n\r HOSTIF-RFCONFIGGET.Confirm, Get current RF device parameters (Radio, Packet format, CSMA, Power" );
        break;

    case 28:
        strcpy( Buffer, "\n\r HOSTIF-HWCONFIG.Request, Request G3 platform HW configuration" );
        break;

    case 29:
        strcpy( Buffer, "\n\r HOSTIF-OTP.Request, Perform an operation on the OTP device area" );
        break;

    case 30:
        strcpy( Buffer, "\n\r HOSTIF-OTP.Confirm, Perform an operation on the OTP device area" );
        break;

    case 31:
        strcpy( Buffer, "\n\r HOSTIF-HWCONFIG.Confirm, Request G3 platform HW configuration" );
        break;

    case 32:
        strcpy( Buffer, "\n\r G3LIB-GET.Request, Get the value of one attribute" );
        break;

    case 33:
        strcpy( Buffer, "\n\r G3LIB-GET.Confirm, Get the value of one attribute" );
        break;

    case 34:
        strcpy( Buffer, "\n\r G3LIB-SET.Request, Set the value of one attribute" );
        break;

    case 35:
        strcpy( Buffer, "\n\r G3LIB-SET.Request, Set the value of one attribute" );
        break;

    case 36:
        strcpy( Buffer, "\n\r G3LIB-SWRESET.Request, Perform a software reset of the platform and change the PLC configuration" );
        break;

    case 37:
        strcpy( Buffer, "\n\r G3LIB-SWRESET.Confirm, Perform a software reset of the platform and change the PLC configuration" );
        break;

    case 38:
        strcpy( Buffer, "\n\r G3LIB-TESTMODEENABLE.Request, Enable or disable a testmode" );
        break;

    case 39:
        strcpy( Buffer, "\n\r G3LIB-TESTMODEENABLE.Confirm, Enable or disable a testmode" );
        break;

    case 40:
        strcpy( Buffer, "\n\r G3LIB-EVENTNOTIFICATION.Indication, Indication of a particular event occurred in the G3 "
                        "platform that requires the host to be informed" );
        break;

    case 41:
        strcpy( Buffer, "\n\r G3LIB-MULTIPLEGET.Request, Request the read of multiple entries from one table" );
        break;

    case 42:
        strcpy( Buffer, "\n\r G3LIB-MULTIPLEGET.Confirm, Request the read of multiple entries from one table" );
        break;

    case 43:
        strcpy( Buffer, "\n\r G3LIB-SNIFFER.Indication, Frame detected in Sniffer Combined mode" );
        break;

    case 64:
        strcpy( Buffer, "\n\r G3PHY-DATA.Request, Data transmission at PHY Layer" );
        break;

    case 65:
        strcpy( Buffer, "\n\r G3PHY-DATA.Confirm, Data transmission at PHY Layer" );
        break;

    case 66:
        strcpy( Buffer, "\n\r G3PHY-DATA.Indication, Data reception at PHY Layer" );
        break;

    case 67:
        strcpy( Buffer, "\n\r G3PHY-ACK.Request, ACK transmission at PHY Layer" );
        break;

    case 68:
        strcpy( Buffer, "\n\r G3PHY-ACK.Confirm, ACK transmission at PHY Layer" );
        break;

    case 69:
        strcpy( Buffer, "\n\r G3PHY-ACK.Indication, ACK reception at PHY Layer" );
        break;

    case 70:
        strcpy( Buffer, "\n\r G3PHY-RF.Request, Raw data transmission on RF interface" );
        break;

    case 71:
        strcpy( Buffer, "\n\r G3PHY-RF.Confirm, Raw data transmission on RF interface" );
        break;

    case 72:
        strcpy( Buffer, "\n\r G3PHY-RF.Indication, Raw data reception on RF interface" );
        break;

    case 74:
        strcpy( Buffer, "\n\r G3PHY-SETTRXSTATE.Request, Put the PHY in transmission or reception state" );
        break;

    case 75:
        strcpy( Buffer, "\n\r G3PHY-SETTRXSTATE.Confirm, Put the PHY in transmission or reception state" );
        break;

    case 76:
        strcpy( Buffer, "\n\r G3PHY-CS.Request, Get the status of the channel (busy/free)" );
        break;

    case 77:
        strcpy( Buffer, "\n\r G3PHY-CS.Confirm, Get the status of the channel (busy/free)" );
        break;

    case 96:
        strcpy( Buffer, "\n\r G3MAC-DATA.Request, Data transmission at MAC Layer" );
        break;

    case 97:
        strcpy( Buffer, "\n\r G3MAC-DATA.Confirm, Data transmission at MAC Layer" );
        break;

    case 98:
        strcpy( Buffer, "\n\r G3MAC-DATA.Indication, Data reception at MAC Layer" );
        break;

    case 99:
        strcpy( Buffer, "\n\r G3MAC-BCNNOTIFY.Indication, Beacon reception at MAC Layer" );
        break;

    case 104:
        strcpy( Buffer, "\n\r G3MAC-RESET.Request, Reset the MAC Layer" );
        break;

    case 105:
        strcpy( Buffer, "\n\r G3MAC-RESET.Confirm, Reset the MAC Layer" );
        break;

    case 106:
        strcpy( Buffer, "\n\r G3MAC-SCAN.Request, Perform an active scan" );
        break;

    case 107:
        strcpy( Buffer, "\n\r G3MAC-SCAN.Confirm, Perform an active scan" );
        break;

    case 108:
        strcpy( Buffer, "\n\r G3MAC-COMMSTATUS.Indication, Indicate a wrong/unexpected data reception" );
        break;

    case 109:
        strcpy( Buffer, "\n\r G3MAC-START.Request, Start a PAN" );
        break;

    case 110:
        strcpy( Buffer, "\n\r G3MAC-START.Confirm, Start a PAN" );
        break;

    case 128:
        strcpy( Buffer, "\n\r G3ADP-DATA.Request, Data transmission at ADP Layer" );
        break;

    case 129:
        strcpy( Buffer, "\n\r G3ADP-DATA.Confirm, Data transmission at ADP Layer" );
        break;

    case 130:
        strcpy( Buffer, "\n\r G3ADP-DATA.Indication, Data reception at ADP Layer" );
        break;

    case 131:
        strcpy( Buffer, "\n\r G3ADP-DISCOVERY.Request, Perform a PAN discovery procedure" );
        break;

    case 132:
        strcpy( Buffer, "\n\r G3ADP-DISCOVERY.Confirm, Perform a PAN discovery procedure" );
        break;

    case 133:
        strcpy( Buffer, "\n\r G3ADP-NETSTART.Request, Start a PAN (coordinator only)" );
        break;

    case 134:
        strcpy( Buffer, "\n\r G3ADP-NETSTART.Confirm, Start a PAN (coordinator only)" );
        break;

    case 135:
        strcpy( Buffer, "\n\r G3ADP-NETJOIN.Request, Join a PAN (device only)" );
        break;

    case 136:
        strcpy( Buffer, "\n\r G3ADP-NETJOIN.Confirm, Join a PAN (device only)" );
        break;

    case 137:
        strcpy( Buffer, "\n\r G3ADP-NETLEAVE.Request, Leave the current PAN (device only)" );
        break;

    case 138:
        strcpy( Buffer, "\n\r G3ADP-NETLEAVE.Confirm, Leave the current PAN (device only)" );
        break;

    case 139:
        strcpy( Buffer, "\n\r G3ADP-NETLEAVE.Indication, Indicate that the node has left the PAN after a kick from "
                        "cooridantor (devide only)" );
        break;

    case 140:
        strcpy( Buffer, "\n\r G3ADP-RESET.Request, Reset the ADP Layer" );
        break;

    case 141:
        strcpy( Buffer, "\n\r G3ADP-RESET.Confirm, Reset the ADP Layer" );
        break;

    case 146:
        strcpy( Buffer, "\n\r G3ADP-NETSTATUS.Indication, Indicate a wrong/unexpected data reception" );
        break;

    case 147:
        strcpy( Buffer, "\n\r G3ADP-ROUTEDISCOVERY.Request, Perform a manual route discovery" );
        break;

    case 148:
        strcpy( Buffer, "\n\r G3ADP-ROUTEDISCOVERY.Confirm, Perform a manual route discovery" );
        break;

    case 149:
        strcpy( Buffer, "\n\r G3ADP-PATHDISCOVERY.Request, Perform a path discovery" );
        break;

    case 150:
        strcpy( Buffer, "\n\r G3ADP-PATHDISCOVERY.Confirm, Perform a path discovery" );
        break;

    case 151:
        strcpy( Buffer, "\n\r G3ADP-LBP.Request, LBP data transmission at ADP Layer (coordinator only)" );
        break;

    case 152:
        strcpy( Buffer, "\n\r G3ADP-LBP.Confirm, LBP data transmission at ADP Layer (coordinator only)" );
        break;

    case 153:
        strcpy( Buffer, "\n\r G3ADP-LBP.Indication, LBP data reception at ADP Layer (coordinator only)" );
        break;

    case 154:
        strcpy( Buffer, "\n\r G3ADP-ROUTEOVER-DATA.Request, Data transmission at ADP without the default routing" );
        break;

    case 155:
        strcpy( Buffer, "\n\r G3ADP-ROUTEDELETE.Request, Delete a route" );
        break;

    case 156:
        strcpy( Buffer, "\n\r G3ADP-ROUTEDELETE.Confirm, Delete a route" );
        break;

    case 176:
        strcpy( Buffer, "\n\r G3BOOT-SRV-START.Request, Start the bootstrap server (coordinator only)" );
        break;

    case 177:
        strcpy( Buffer, "\n\r G3BOOT-SRV-START.Confirm, Start the bootstrap server (coordinator only)" );
        break;

    case 178:
        strcpy( Buffer, "\n\r G3BOOT-SRV-STOP.Request, Stop the bootstrap server (coordinator only)" );
        break;

    case 179:
        strcpy( Buffer, "\n\r G3BOOT-SRV-STOP.Confirm, Stop the bootstrap server (coordinator only)" );
        break;

    case 180:
        strcpy( Buffer, "\n\r G3BOOT-SRV-LEAVE.Indication, Indicate a node has left the PAN (coordinator only)" );
        break;

    case 181:
        strcpy( Buffer, "\n\r G3BOOT-SRV-KICK.Request, Kick a device from the network (coordinator only)" );
        break;

    case 182:
        strcpy( Buffer, "\n\r G3BOOT-SRV-KICK.Confirm, Kick a device from the network (coordinator only)" );
        break;

    case 183:
        strcpy( Buffer, "\n\r G3BOOT-SRV-JOIN.Indication, Indicate a node has joint the PAN (coordinator only)" );
        break;

    case 184:
        strcpy( Buffer, "\n\r G3BOOT-DEV-START.Request, Start a bootstrap client (device only)" );
        break;

    case 185:
        strcpy( Buffer, "\n\r G3BOOT-DEV-START.Confirm, Start a bootstrap client (device only)" );
        break;

    case 186:
        strcpy( Buffer, "\n\r G3BOOT-DEV-LEAVE.Request, Ask the node to leave the current PAN (device only)" );
        break;

    case 187:
        strcpy( Buffer, "\n\r G3BOOT-DEV-LEAVE.Confirm, Ask the node to leave the current PAN (device only)" );
        break;

    case 188:
        strcpy( Buffer, "\n\r G3BOOT-DEV-LEAVE.Indication, Indicate the node has left the PAN (device only)" );
        break;

    case 193:
        strcpy( Buffer, "\n\r G3BOOT-DEV-PANSORT.Indication, Get the list of LBA (device only)" );
        break;

    case 194:
        strcpy( Buffer, "\n\r G3BOOT-DEV-PANSORT.Request, Provide the ordered list of LBAs to be used to bootstrap(device only)" );
        break;

    case 195:
        strcpy( Buffer, "\n\r G3BOOT-DEV-PANSORT.Confirm, Provide the ordered list of LBAs to be used to bootstrap(device only)" );
        break;

    case 196:
        strcpy( Buffer, "\n\r G3BOOT-SRV-GETPSK.Indication, Get IdP and EUI64 of the device joining the PAN (coordinator only)" );
        break;

    case 197:
        strcpy( Buffer, "\n\r G3BOOT-SRV-GETPSK.Request, Provide the PSK and the short address to be assigned to the "
                        "device (coordinator only)" );
        break;

    case 198:
        strcpy( Buffer, "\n\r G3BOOT-SRV-GETPSK.Confirm, Provide the PSK and the short address to be assigned to the "
                        "device (coordinator only)" );
        break;

    case 208:
        strcpy( Buffer, "\n\r G3UDP-DATA.Request, Transmit data at UDP Layer" );
        break;

    case 209:
        strcpy( Buffer, "\n\r G3UDP-DATA.Confirm, Transmit data at UDP Layer" );
        break;

    case 210:
        strcpy( Buffer, "\n\r G3UDP-DATA.Indication, Receive data at UDP Layer" );
        break;

    case 211:
        strcpy( Buffer, "\n\r G3UDP-CONN-SET.Request, Register a new communication socket handler over UDP" );
        break;

    case 212:
        strcpy( Buffer, "\n\r G3UDP-CONN-SET.Confirm, Register a new communication socket handler over UDP" );
        break;

    case 213:
        strcpy( Buffer, "\n\r G3UDP-CONN-GET.Request, Get the UDP communication socket handler" );
        break;

    case 214:
        strcpy( Buffer, "\n\r G3UDP-CONN-GET.Conffirm, Get the UDP communication socket handler" );
        break;

    case 215:
        strcpy( Buffer, "\n\r G3ICMP-ECHO.Request, Transmit ECHO request data at ICMPv6 layer" );
        break;

    case 216:
        strcpy( Buffer, "\n\r G3ICMP-ECHO.Confirm, Confirm the ECHO request ICMPv6 data transmission" );
        break;

    case 217:
        strcpy( Buffer, "\n\r G3ICMP-ECHOREP.Indication, Receive the ECHO reply data at ICMPv6 Layer" );
        break;

    case 223:
        strcpy( Buffer, "\n\r G3ICMP-ECHOREQ.Indication, Receive the ECHO request data from ICMPv6 Layer" );
        break;

    case 255:
        strcpy( Buffer, "\n\r ERROR.Indication, Notification of one Host Interface error" );
        break;

    default:
        strcpy( Buffer, "\n\r Error with CMD_ID value" );
        break;
    } // end of switch
    strcpy( Buffer, "\n\r" );
}

void SerialAnalyzerResults::Print_Command_STM( int CMD_ID_Value )
{
    switch( CMD_ID_Value )
    {
    case 0:
        AddTabularText( "\n\r HOSTIF-HWRESET.Request, Perform a hardware reset of the platform. Reset can be performed "
                        "also through the RESET pin" );
        break;

    case 1:
        AddTabularText( "\n\r HOSTIF-HWRESET.Confirm, Perform a hardware reset of the platform. Reset can be performed "
                        "also through the RESET pin" );
        break;

    case 2:
        AddTabularText( "\n\r HOSTIF-MODESET.Request, Change the mode of the platform" );
        break;

    case 3:
        AddTabularText( "\n\r HOSTIF-MODESET.Confirm, Change the mode of the platform" );
        break;

    case 4:
        AddTabularText( "\n\r HOSTIF-MODEGET.Request, Get the current operating mode of the platform" );
        break;

    case 5:
        AddTabularText( "\n\r HOSTIF-MODEGET.Confirm, Get the current operating mode of the platform" );
        break;

    case 6:
        AddTabularText( "\n\r HOSTIF-BAUDRATESET.Request, Change the UART host interface baudrate of the platform" );
        break;

    case 7:
        AddTabularText( "\n\r HOSTIF-BAUDRATESET.Confirm, Change the UART host interface baudrate of the platform" );
        break;

    case 8:
        AddTabularText( "\n\r HOSTIF-RESETSTATE.Request, Reset the host interface state flushing all the messages in the "
                        "internal queue" );
        break;

    case 9:
        AddTabularText( "\n\r HOSTIF-RESETSTATE.Confirm, Reset the host interface state flushing all the messages in the "
                        "internal queue" );
        break;

    case 10:
        AddTabularText( "\n\r HOSTIF-TESTSET.Request, Request to enter in a PLC test or to exit" );
        break;

    case 11:
        AddTabularText( "\n\r HOSTIF-TESTSET.Confirm, Request to enter in a PLC test or to exit" );
        break;

    case 12:
        AddTabularText( "\n\r HOSTIF-TESTGET.Request, Request to get the status of the current PLC test" );
        break;

    case 13:
        AddTabularText( "\n\r HOSTIF-TESTGET.Confirm, Request to get the status of the current PLC test" );

    case 14:
        AddTabularText( "\n\r HOSTIF-SFLASH.Request, Perform an operation on the external SPI Flash if connected to the G3 platform" );
        break;

    case 15:
        AddTabularText( "\n\r HOSTIF-SFLASH.Confirm, Perform an operation on the external SPI Flash if connected to the G3 platform" );
        break;

    case 16:
        AddTabularText( "\n\r HOSTIF-NVM.Request, Perform an operation on the external NVM if an external SPI Flash is "
                        "connected to the G3 platform" );
        break;

    case 17:
        AddTabularText( "\n\r HOSTIF-NVM.Confirm, Perform an operation on the external NVM if an external SPI Flash is "
                        "connected to the G3 platform" );
        break;

    case 18:
        AddTabularText( "\n\r HOSTIF-TRACE.Request, Change the configuration of the trace module" );
        break;

    case 19:
        AddTabularText( "\n\r HOSTIF-TRACE.Confirm, Change the configuration of the trace module" );
        break;

    case 20:
        AddTabularText( "\n\r HOSTIF-DBGTOOL.Request, Perform information and debug operation on the G3 platform" );
        break;

    case 21:
        AddTabularText( "\n\r HOSTIF-DBGTOOL.Confirm, Perform information and debug operation on the G3 platform" );
        break;

    case 22:
        AddTabularText( "\n\r HOSTIF-SEC-INIT.Request, Request operation for the secure host interface" );
        break;

    case 23:
        AddTabularText( "\n\r HOSTIF-SEC-INIT.Confirm, Request operation for the secure host interface" );
        break;

    case 24:
        AddTabularText( "\n\r HOSTIF-RFCONFIGSET.Request, Configure RF device parameters (Radio, Packet format, CSMA, Power" );
        break;

    case 25:
        AddTabularText( "\n\r HOSTIF-RFCONFIGSET.Confirm, Configure RF device parameters (Radio, Packet format, CSMA, Power" );
        break;

    case 26:
        AddTabularText( "\n\r HOSTIF-RFCONFIGGET.Request, Get current RF device parameters (Radio, Packet format, CSMA, Power" );
        break;

    case 27:
        AddTabularText( "\n\r HOSTIF-RFCONFIGGET.Confirm, Get current RF device parameters (Radio, Packet format, CSMA, Power" );
        break;

    case 28:
        AddTabularText( "\n\r HOSTIF-HWCONFIG.Request, Request G3 platform HW configuration" );
        break;

    case 29:
        AddTabularText( "\n\r HOSTIF-OTP.Request, Perform an operation on the OTP device area" );
        break;

    case 30:
        AddTabularText( "\n\r HOSTIF-OTP.Confirm, Perform an operation on the OTP device area" );
        break;

    case 31:
        AddTabularText( "\n\r HOSTIF-HWCONFIG.Confirm, Request G3 platform HW configuration" );
        break;

    case 32:
        AddTabularText( "\n\r G3LIB-GET.Request, Get the value of one attribute" );
        break;

    case 33:
        AddTabularText( "\n\r G3LIB-GET.Confirm, Get the value of one attribute" );
        break;

    case 34:
        AddTabularText( "\n\r G3LIB-SET.Request, Set the value of one attribute" );
        break;

    case 35:
        AddTabularText( "\n\r G3LIB-SET.Request, Set the value of one attribute" );
        break;

    case 36:
        AddTabularText( "\n\r G3LIB-SWRESET.Request, Perform a software reset of the platform and change the PLC configuration" );
        break;

    case 37:
        AddTabularText( "\n\r G3LIB-SWRESET.Confirm, Perform a software reset of the platform and change the PLC configuration" );
        break;

    case 38:
        AddTabularText( "\n\r G3LIB-TESTMODEENABLE.Request, Enable or disable a testmode" );
        break;

    case 39:
        AddTabularText( "\n\r G3LIB-TESTMODEENABLE.Confirm, Enable or disable a testmode" );
        break;

    case 40:
        AddTabularText( "\n\r G3LIB-EVENTNOTIFICATION.Indication, Indication of a particular event occurred in the G3 "
                        "platform that requires the host to be informed" );
        break;

    case 41:
        AddTabularText( "\n\r G3LIB-MULTIPLEGET.Request, Request the read of multiple entries from one table" );
        break;

    case 42:
        AddTabularText( "\n\r G3LIB-MULTIPLEGET.Confirm, Request the read of multiple entries from one table" );
        break;

    case 43:
        AddTabularText( "\n\r G3LIB-SNIFFER.Indication, Frame detected in Sniffer Combined mode" );
        break;

    case 64:
        AddTabularText( "\n\r G3PHY-DATA.Request, Data transmission at PHY Layer" );
        break;

    case 65:
        AddTabularText( "\n\r G3PHY-DATA.Confirm, Data transmission at PHY Layer" );
        break;

    case 66:
        AddTabularText( "\n\r G3PHY-DATA.Indication, Data reception at PHY Layer" );
        break;

    case 67:
        AddTabularText( "\n\r G3PHY-ACK.Request, ACK transmission at PHY Layer" );
        break;

    case 68:
        AddTabularText( "\n\r G3PHY-ACK.Confirm, ACK transmission at PHY Layer" );
        break;

    case 69:
        AddTabularText( "\n\r G3PHY-ACK.Indication, ACK reception at PHY Layer" );
        break;

    case 70:
        AddTabularText( "\n\r G3PHY-RF.Request, Raw data transmission on RF interface" );
        break;

    case 71:
        AddTabularText( "\n\r G3PHY-RF.Confirm, Raw data transmission on RF interface" );
        break;

    case 72:
        AddTabularText( "\n\r G3PHY-RF.Indication, Raw data reception on RF interface" );
        break;

    case 74:
        AddTabularText( "\n\r G3PHY-SETTRXSTATE.Request, Put the PHY in transmission or reception state" );
        break;

    case 75:
        AddTabularText( "\n\r G3PHY-SETTRXSTATE.Confirm, Put the PHY in transmission or reception state" );
        break;

    case 76:
        AddTabularText( "\n\r G3PHY-CS.Request, Get the status of the channel (busy/free)" );
        break;

    case 77:
        AddTabularText( "\n\r G3PHY-CS.Confirm, Get the status of the channel (busy/free)" );
        break;

    case 96:
        AddTabularText( "\n\r G3MAC-DATA.Request, Data transmission at MAC Layer" );
        break;

    case 97:
        AddTabularText( "\n\r G3MAC-DATA.Confirm, Data transmission at MAC Layer" );
        break;

    case 98:
        AddTabularText( "\n\r G3MAC-DATA.Indication, Data reception at MAC Layer" );
        break;

    case 99:
        AddTabularText( "\n\r G3MAC-BCNNOTIFY.Indication, Beacon reception at MAC Layer" );
        break;

    case 104:
        AddTabularText( "\n\r G3MAC-RESET.Request, Reset the MAC Layer" );
        break;

    case 105:
        AddTabularText( "\n\r G3MAC-RESET.Confirm, Reset the MAC Layer" );
        break;

    case 106:
        AddTabularText( "\n\r G3MAC-SCAN.Request, Perform an active scan" );
        break;

    case 107:
        AddTabularText( "\n\r G3MAC-SCAN.Confirm, Perform an active scan" );
        break;

    case 108:
        AddTabularText( "\n\r G3MAC-COMMSTATUS.Indication, Indicate a wrong/unexpected data reception" );
        break;

    case 109:
        AddTabularText( "\n\r G3MAC-START.Request, Start a PAN" );
        break;

    case 110:
        AddTabularText( "\n\r G3MAC-START.Confirm, Start a PAN" );
        break;

    case 128:
        AddTabularText( "\n\r G3ADP-DATA.Request, Data transmission at ADP Layer" );
        break;

    case 129:
        AddTabularText( "\n\r G3ADP-DATA.Confirm, Data transmission at ADP Layer" );
        break;

    case 130:
        AddTabularText( "\n\r G3ADP-DATA.Indication, Data reception at ADP Layer" );
        break;

    case 131:
        AddTabularText( "\n\r G3ADP-DISCOVERY.Request, Perform a PAN discovery procedure" );
        break;

    case 132:
        AddTabularText( "\n\r G3ADP-DISCOVERY.Confirm, Perform a PAN discovery procedure" );
        break;

    case 133:
        AddTabularText( "\n\r G3ADP-NETSTART.Request, Start a PAN (coordinator only)" );
        break;

    case 134:
        AddTabularText( "\n\r G3ADP-NETSTART.Confirm, Start a PAN (coordinator only)" );
        break;

    case 135:
        AddTabularText( "\n\r G3ADP-NETJOIN.Request, Join a PAN (device only)" );
        break;

    case 136:
        AddTabularText( "\n\r G3ADP-NETJOIN.Confirm, Join a PAN (device only)" );
        break;

    case 137:
        AddTabularText( "\n\r G3ADP-NETLEAVE.Request, Leave the current PAN (device only)" );
        break;

    case 138:
        AddTabularText( "\n\r G3ADP-NETLEAVE.Confirm, Leave the current PAN (device only)" );
        break;

    case 139:
        AddTabularText( "\n\r G3ADP-NETLEAVE.Indication, Indicate that the node has left the PAN after a kick from "
                        "cooridantor (devide only)" );
        break;

    case 140:
        AddTabularText( "\n\r G3ADP-RESET.Request, Reset the ADP Layer" );
        break;

    case 141:
        AddTabularText( "\n\r G3ADP-RESET.Confirm, Reset the ADP Layer" );
        break;

    case 146:
        AddTabularText( "\n\r G3ADP-NETSTATUS.Indication, Indicate a wrong/unexpected data reception" );
        break;

    case 147:
        AddTabularText( "\n\r G3ADP-ROUTEDISCOVERY.Request, Perform a manual route discovery" );
        break;

    case 148:
        AddTabularText( "\n\r G3ADP-ROUTEDISCOVERY.Confirm, Perform a manual route discovery" );
        break;

    case 149:
        AddTabularText( "\n\r G3ADP-PATHDISCOVERY.Request, Perform a path discovery" );
        break;

    case 150:
        AddTabularText( "\n\r G3ADP-PATHDISCOVERY.Confirm, Perform a path discovery" );
        break;

    case 151:
        AddTabularText( "\n\r G3ADP-LBP.Request, LBP data transmission at ADP Layer (coordinator only)" );
        break;

    case 152:
        AddTabularText( "\n\r G3ADP-LBP.Confirm, LBP data transmission at ADP Layer (coordinator only)" );
        break;

    case 153:
        AddTabularText( "\n\r G3ADP-LBP.Indication, LBP data reception at ADP Layer (coordinator only)" );
        break;

    case 154:
        AddTabularText( "\n\r G3ADP-ROUTEOVER-DATA.Request, Data transmission at ADP without the default routing" );
        break;

    case 155:
        AddTabularText( "\n\r G3ADP-ROUTEDELETE.Request, Delete a route" );
        break;

    case 156:
        AddTabularText( "\n\r G3ADP-ROUTEDELETE.Confirm, Delete a route" );
        break;

    case 176:
        AddTabularText( "\n\r G3BOOT-SRV-START.Request, Start the bootstrap server (coordinator only)" );
        break;

    case 177:
        AddTabularText( "\n\r G3BOOT-SRV-START.Confirm, Start the bootstrap server (coordinator only)" );
        break;

    case 178:
        AddTabularText( "\n\r G3BOOT-SRV-STOP.Request, Stop the bootstrap server (coordinator only)" );
        break;

    case 179:
        AddTabularText( "\n\r G3BOOT-SRV-STOP.Confirm, Stop the bootstrap server (coordinator only)" );
        break;

    case 180:
        AddTabularText( "\n\r G3BOOT-SRV-LEAVE.Indication, Indicate a node has left the PAN (coordinator only)" );
        break;

    case 181:
        AddTabularText( "\n\r G3BOOT-SRV-KICK.Request, Kick a device from the network (coordinator only)" );
        break;

    case 182:
        AddTabularText( "\n\r G3BOOT-SRV-KICK.Confirm, Kick a device from the network (coordinator only)" );
        break;

    case 183:
        AddTabularText( "\n\r G3BOOT-SRV-JOIN.Indication, Indicate a node has joint the PAN (coordinator only)" );
        break;

    case 184:
        AddTabularText( "\n\r G3BOOT-DEV-START.Request, Start a bootstrap client (device only)" );
        break;

    case 185:
        AddTabularText( "\n\r G3BOOT-DEV-START.Confirm, Start a bootstrap client (device only)" );
        break;

    case 186:
        AddTabularText( "\n\r G3BOOT-DEV-LEAVE.Request, Ask the node to leave the current PAN (device only)" );
        break;

    case 187:
        AddTabularText( "\n\r G3BOOT-DEV-LEAVE.Confirm, Ask the node to leave the current PAN (device only)" );
        break;

    case 188:
        AddTabularText( "\n\r G3BOOT-DEV-LEAVE.Indication, Indicate the node has left the PAN (device only)" );
        break;

    case 193:
        AddTabularText( "\n\r G3BOOT-DEV-PANSORT.Indication, Get the list of LBA (device only)" );
        break;

    case 194:
        AddTabularText( "\n\r G3BOOT-DEV-PANSORT.Request, Provide the ordered list of LBAs to be used to bootstrap(device only)" );
        break;

    case 195:
        AddTabularText( "\n\r G3BOOT-DEV-PANSORT.Confirm, Provide the ordered list of LBAs to be used to bootstrap(device only)" );
        break;

    case 196:
        AddTabularText( "\n\r G3BOOT-SRV-GETPSK.Indication, Get IdP and EUI64 of the device joining the PAN (coordinator only)" );
        break;

    case 197:
        AddTabularText( "\n\r G3BOOT-SRV-GETPSK.Request, Provide the PSK and the short address to be assigned to the "
                        "device (coordinator only)" );
        break;

    case 198:
        AddTabularText( "\n\r G3BOOT-SRV-GETPSK.Confirm, Provide the PSK and the short address to be assigned to the "
                        "device (coordinator only)" );
        break;

    case 208:
        AddTabularText( "\n\r G3UDP-DATA.Request, Transmit data at UDP Layer" );
        break;

    case 209:
        AddTabularText( "\n\r G3UDP-DATA.Confirm, Transmit data at UDP Layer" );
        break;

    case 210:
        AddTabularText( "\n\r G3UDP-DATA.Indication, Receive data at UDP Layer" );
        break;

    case 211:
        AddTabularText( "\n\r G3UDP-CONN-SET.Request, Register a new communication socket handler over UDP" );
        break;

    case 212:
        AddTabularText( "\n\r G3UDP-CONN-SET.Confirm, Register a new communication socket handler over UDP" );
        break;

    case 213:
        AddTabularText( "\n\r G3UDP-CONN-GET.Request, Get the UDP communication socket handler" );
        break;

    case 214:
        AddTabularText( "\n\r G3UDP-CONN-GET.Conffirm, Get the UDP communication socket handler" );
        break;

    case 215:
        AddTabularText( "\n\r G3ICMP-ECHO.Request, Transmit ECHO request data at ICMPv6 layer" );
        break;

    case 216:
        AddTabularText( "\n\r G3ICMP-ECHO.Confirm, Confirm the ECHO request ICMPv6 data transmission" );
        break;

    case 217:
        AddTabularText( "\n\r G3ICMP-ECHOREP.Indication, Receive the ECHO reply data at ICMPv6 Layer" );
        break;

    case 223:
        AddTabularText( "\n\r G3ICMP-ECHOREQ.Indication, Receive the ECHO request data from ICMPv6 Layer" );
        break;

    case 255:
        AddTabularText( "\n\r ERROR.Indication, Notification of one Host Interface error" );
        break;

    default:
        AddTabularText( "\n\r Error with CMD_ID value" );
        break;
    } // end of switch
    AddTabularText( "\n\r" );
}

SerialAnalyzerResults::SerialAnalyzerResults( SerialAnalyzer* analyzer, SerialAnalyzerSettings* settings )
    : AnalyzerResults(), mSettings( settings ), mAnalyzer( analyzer )
{
}

SerialAnalyzerResults::~SerialAnalyzerResults() = default;

void SerialAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& /*channel*/,
                                                DisplayBase display_base ) // unreferenced vars commented out to remove warnings.
{
    // we only need to pay attention to 'channel' if we're making bubbles for more than one channel (as set by
    // AddChannelBubblesWillAppearOn)
    ClearResultStrings();
    Frame frame = GetFrame( frame_index );

    bool framing_error = false;
    if( ( frame.mFlags & FRAMING_ERROR_FLAG ) != 0 )
        framing_error = true;

    bool parity_error = false;
    if( ( frame.mFlags & PARITY_ERROR_FLAG ) != 0 )
        parity_error = true;

    U32 bits_per_transfer = mSettings->mBitsPerTransfer;
    if( mSettings->mSerialMode != SerialAnalyzerEnums::Normal )
        bits_per_transfer--;

    char number_str[ 128 ];
    AnalyzerHelpers::GetNumberString( frame.mData1, display_base, bits_per_transfer, number_str, 128 );

    char result_str[ 128 ];

    // MP mode address case:
    if( ( frame.mFlags & MP_MODE_ADDRESS_FLAG ) != 0 )
    {
        AddResultString( "A" );
        AddResultString( "Addr" );
        // result_str[128] = ("H");

        if( framing_error == false )
        {
            snprintf( result_str, sizeof( result_str ), "Addr: %s", number_str );
            AddResultString( result_str );

            snprintf( result_str, sizeof( result_str ), "Address: %s", number_str );
            AddResultString( result_str );
        }
        else
        {
            snprintf( result_str, sizeof( result_str ), "Addr: %s (framing error)", number_str );
            AddResultString( result_str );

            snprintf( result_str, sizeof( result_str ), "Address: %s (framing error)", number_str );
            AddResultString( result_str );
        }
        return;
    }

    // normal case:
    if( ( parity_error == true ) || ( framing_error == true ) )
    {
        AddResultString( "!" );

        snprintf( result_str, sizeof( result_str ), "%s (error)", number_str );
        AddResultString( result_str );

        if( framing_error == false )
            snprintf( result_str, sizeof( result_str ), "%s (parity error)", number_str );
        else if( parity_error == false )
            snprintf( result_str, sizeof( result_str ), "%s (framing error)", number_str );
        else
            snprintf( result_str, sizeof( result_str ), "%s (framing error & parity error)", number_str );

        AddResultString( result_str );
    }
    else
    {
        AddResultString( number_str );
    }
}

void SerialAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 /*export_type_user_id*/ )
{
    // export_type_user_id is only important if we have more than one export type.
    std::stringstream ss;

    U64 trigger_sample = mAnalyzer->GetTriggerSample();
    U32 sample_rate = mAnalyzer->GetSampleRate();
    U64 num_frames = GetNumFrames();

    void* f = AnalyzerHelpers::StartFile( file );


    if( mSettings->mSerialMode == SerialAnalyzerEnums::Normal )
    {
        // Normal case -- not MP mode.
        ss << "Time [s],Value,Parity Error,Framing Error" << std::endl;

        for( U32 i = 0; i < num_frames; i++ )
        {
            Frame frame = GetFrame( i );

            // static void GetTimeString( U64 sample, U64 trigger_sample, U32 sample_rate_hz, char* result_string, U32
            // result_string_max_length );
            char time_str[ 128 ];
            AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

            char number_str[ 128 ];
            AnalyzerHelpers::GetNumberString( frame.mData1, display_base, mSettings->mBitsPerTransfer, number_str, 128 );

            ss << time_str << "," << number_str;


            if( ( frame.mFlags & PARITY_ERROR_FLAG ) != 0 )
                ss << ",Error,";

            else

                ss << ", ,";

            if( ( frame.mFlags & FRAMING_ERROR_FLAG ) != 0 )
                ss << "Error";


            ss << std::endl;

            AnalyzerHelpers::AppendToFile( ( U8* )ss.str().c_str(), static_cast<U32>( ss.str().length() ), f );
            ss.str( std::string() );

            if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
            {
                AnalyzerHelpers::EndFile( f );
                return;
            }
        }
    }
    else
    {
        // MP mode.
        ss << "Time [s],Packet ID,Address,Data,Framing Error" << std::endl;
        U64 address = 0;

        for( U32 i = 0; i < num_frames; i++ )
        {
            Frame frame = GetFrame( i );

            if( ( frame.mFlags & MP_MODE_ADDRESS_FLAG ) != 0 )
            {
                address = frame.mData1;
                continue;
            }

            U64 packet_id = GetPacketContainingFrameSequential( i );

            // static void GetTimeString( U64 sample, U64 trigger_sample, U32 sample_rate_hz, char* result_string, U32
            // result_string_max_length );
            char time_str[ 128 ];
            AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

            char address_str[ 128 ];
            AnalyzerHelpers::GetNumberString( address, display_base, mSettings->mBitsPerTransfer - 1, address_str, 128 );

            char number_str[ 128 ];
            AnalyzerHelpers::GetNumberString( frame.mData1, display_base, mSettings->mBitsPerTransfer - 1, number_str, 128 );
            if( packet_id == INVALID_RESULT_INDEX )
                ss << time_str << "," << "" << "," << address_str << "," << number_str << ",";
            else
                ss << time_str << "," << packet_id << "," << address_str << "," << number_str << ",";

            if( ( frame.mFlags & FRAMING_ERROR_FLAG ) != 0 )
                ss << "Error";

            ss << std::endl;

            AnalyzerHelpers::AppendToFile( ( U8* )ss.str().c_str(), static_cast<U32>( ss.str().length() ), f );
            ss.str( std::string() );


            if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
            {
                AnalyzerHelpers::EndFile( f );
                return;
            }
        }
    }

    UpdateExportProgressAndCheckForCancel( num_frames, num_frames );
    AnalyzerHelpers::EndFile( f );
}
void SerialAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
#if 1
    ClearTabularText();
    Frame frame = GetFrame( frame_index );

    bool framing_error = false;

    if( ( frame.mFlags & FRAMING_ERROR_FLAG ) != 0 )
        framing_error = true;

    bool parity_error = false;
    if( ( frame.mFlags & PARITY_ERROR_FLAG ) != 0 )
        parity_error = true;

    U32 bits_per_transfer = mSettings->mBitsPerTransfer;
    if( mSettings->mSerialMode != SerialAnalyzerEnums::Normal )
        bits_per_transfer--;

    char number_str[ 128 ];
    AnalyzerHelpers::GetNumberString( frame.mData1, display_base, bits_per_transfer, number_str, 128 );


    char result_str[ 128 ];

    // MP mode address case:
    if( ( frame.mFlags & MP_MODE_ADDRESS_FLAG ) != 0 )
    {
        if( framing_error == false )
        {
            snprintf( result_str, sizeof( result_str ), "Address: %s", number_str );
            AddTabularText( result_str );
        }
        else
        {
            snprintf( result_str, sizeof( result_str ), "Address: %s (framing error)", number_str );
            AddTabularText( result_str );
        }
        return;
    }

    // normal case:


    if( ( parity_error == true ) || ( framing_error == true ) )
    {
        if( framing_error == false )
            snprintf( result_str, sizeof( result_str ), "%s (parity error)\n", number_str );
        else if( parity_error == false )
            snprintf( result_str, sizeof( result_str ), "%s (framing error)\n", number_str );
        else
            snprintf( result_str, sizeof( result_str ), "%s (framing error & parity error)\n", number_str );

        AddTabularText( result_str );
    }
    else
    {
        Parse_Data_STM( number_str );
        //  AddTabularText(" Bla bla truc /n/r");

#endif
    }
}
void SerialAnalyzerResults::Parse_Data_STM( char number_str[] )
{
    // vars
    char CleanData[ 3 ];
    char ascii_buffer[ 5 ];         // For visible ascii characters nad "\t, \n, \r"
    char LEN_buff[ 128 ];           // Stores first, then second byte value of payload length
    char LSB_LEN_buff[ 128 ];       // hex in string form
    char LSB_LEN_buff_value[ 128 ]; // value in string form

    static unsigned int lsb_LEN = 0;   // LSB LEN byte
    static unsigned int msb_LEN = 0;   // MSB LEN byte
    static unsigned int LEN_value = 0; // Number of bytes containing payload length

    static int synchro1 = 0; // For checking synchro byte 1
    static int synchro2 = 0; // For checking synchro byte 2


    char CMD_ID_buff[ 10 ]; // Buffer for storing incoming CMD_ID string (3rd byte)
    int CMD_ID_value;       // Value of CMD_ID

    char zero_buff[ 5 ] = "0x00"; // For setting some integer values to 0
// static int load_payload = 0;  // Index for incoming paylaod

// Time variables:
// Creating custom epoch for LINUX/UNIX
#ifdef __GNUC__
    std::chrono::time_point<std::chrono::high_resolution_clock> custom_epoch =
        std::chrono::high_resolution_clock::from_time_t( 1723462354 ); // Number of seconds passed from unix epoch

#endif
// Creating custom epoch for WINDOWS
#ifdef _MSC_VER
    const std::chrono::seconds custom_epoch_seconds( 1723702957 );
    const std::chrono::high_resolution_clock::time_point custom_epoch( custom_epoch_seconds );

// std::chrono::time_point<std::chrono::high_resolution_clock> custom_epoch =
// std::chrono::time_point<std::chrono::high_resolution_clock>(
//     std::chrono::duration_cast<std::chrono::nanoseconds>(custom_epoch_seconds));
#endif

    static long start_time_miliseconds_number;    // Used for calculating time difference between two incoming bytes
    static long previous_time_miliseconds_number; // Used for calculating time difference between two incoming bytes

    // counters
    // static int Synchro_Check_Flag = 0; // Checks if first two bytes are 1616 and if payload is in correct format

    static int count_to = 0;      // Value of whole package length ( Payload + 10 Header Bytes + 2 CRC Bytes)
    static int general_count = 1; // Global counter that resets for each new package
    static int payload_counter;   // Counter for coutning incoming payload bytes (Should go to LEN_value)

    // UDP vars

    // Buffer for concating strings
    // Data buffer with full msg
    static char DataToSendUDP[ 2000 ];
    size_t CurrentDataLength;

    // Port and IP address values - From LLA interface
    const char* server_ip = mSettings->GetIPAddressString();
    U32 server_port = mSettings->GetPortNumber();
    
    // Dynamic byte array - storing data for sending with UDP
    std::vector<uint8_t> byteArray;

    // Header vars - Band, Version, Hardwere Type, Packet type
    double Band_Select = mSettings->GetBand();
    double Packet_Type_Get = mSettings->GetPacketType();

    uint8_t Packet_Type =
        static_cast<uint8_t>( Packet_Type_Get );        // defines the paket type contained in SnifferPayload, 0- sniffer, 1- IPV6
    uint8_t Band = static_cast<uint8_t>( Band_Select ); // active plc band. 0- CENA, 1-CENB, 2-FCC
    uint8_t Hardwere_Type =
        1; // defines wich g3 hardware is used for sniffin (g3 modem vendor). At the moment 0 value is assigned to Microchip hardware.
    uint8_t Hardwere_Version = 1; // verson of the hardware
    std::vector<uint8_t> Reserved_Bytes( 4, 0 );

    // Code
    // Modifying data to be in format "0x hex value"

    if( number_str[ 0 ] == '\\' )
    {
        if( number_str[ 1 ] == '0' )
        {
            if( number_str[ 2 ] == '\0' )
            {
                number_str[ 0 ] = '0';
                number_str[ 1 ] = 'x';
                number_str[ 2 ] = '0';
                number_str[ 3 ] = '0';
                // number_str[ 4 ] = '\0';
            }
        }
    }
    if( number_str[ 0 ] == '\\' )
    {
        if( number_str[ 1 ] == 't' )
        {
            if( number_str[ 2 ] == '\0' )
            {
                number_str[ 0 ] = '0';
                number_str[ 1 ] = 'x';
                number_str[ 2 ] = '0';
                number_str[ 3 ] = '9';
            }
        }
    }
    if( number_str[ 0 ] == '\\' )
    {
        if( number_str[ 1 ] == 'r' )
        {
            if( number_str[ 2 ] == '\0' )
            {
                number_str[ 0 ] = '0';
                number_str[ 1 ] = 'x';
                number_str[ 2 ] = '0';
                number_str[ 3 ] = 'D';
            }
        }
    }
    if( number_str[ 0 ] == '\\' )
    {
        if( number_str[ 1 ] == 'n' )
        {
            if( number_str[ 2 ] == '\0' )
            {
                number_str[ 0 ] = '0';
                number_str[ 1 ] = 'x';
                number_str[ 2 ] = '0';
                number_str[ 3 ] = 'A';
            }
        }
    }
    if( number_str[ 0 ] == '\\' )
    {
        if( number_str[ 1 ] == 'x' )
        {
            if( number_str[ 4 ] == '\0' )
            {
                number_str[ 0 ] = '0';
            }
        }
    }
    // time calculations
    previous_time_miliseconds_number = start_time_miliseconds_number;

    auto start_time = std::chrono::high_resolution_clock::now();

    auto duration = start_time - custom_epoch; // time from 20th May 2024 to now

    // cast duration time to miliseconds
    auto duration_miliseconds = std::chrono::duration_cast<std::chrono::milliseconds>( duration ).count();

    // cast duration in ms to integer/number to save data from previous iteration
    start_time_miliseconds_number = static_cast<int>( duration_miliseconds );

    // Cleaning the buffer we are stroing data for UDP sending - dodato
    if( general_count == 1 )
    {
        // CleanBuffer( DataToSendUDP );
        byteArray.clear();
        std::memset( DataToSendUDP, 0, sizeof( DataToSendUDP ) );

        // HexToByteArray.clear();
    }
    // PRINTING
    // If incoming info is in visible ascii range its the only character in buffer
    // if thats the case print hex value of the ascii symbol
    if( number_str[ 1 ] == '\0' )
    {
        std::snprintf( ascii_buffer, sizeof( ascii_buffer ), "0x%02X", number_str[ 0 ] );

        // Extracting two Data nibbles from formated number_str
        ascii_buffer[ 4 ] = '\0';
        CleanData[ 0 ] = ascii_buffer[ 2 ];
        CleanData[ 1 ] = ascii_buffer[ 3 ];
        CleanData[ 2 ] = '\0';
        CurrentDataLength = std::strlen( DataToSendUDP );

        AppendToBuffer( CleanData, DataToSendUDP, CurrentDataLength, sizeof( DataToSendUDP ) );

        // checking time difference between two incoming data and printing
        // that either new data arrived or an error occured
        if( start_time_miliseconds_number - previous_time_miliseconds_number < 500 )
        {
            AddTabularText( ascii_buffer, " " );
        }
        else
        {
            AddTabularText( "Large time difference between data" );
            AddTabularText( ascii_buffer, " " );
            // AddTabularText("JEDAN");
        }
        if( general_count == 3 )
        {
            CMD_ID_value = std::stoi( ascii_buffer + 2, nullptr, 16 ); // Getting CMD_ID
            sprintf( CMD_ID_buff, "%d", CMD_ID_value, " " );           // converting decimal version of hex CMD value into a char buffer
            // AddTabularText( "Decimalna vrednost: ", CMD_ID_buff );     // Printing decimal value
            synchro1 = std::stoi( zero_buff + 2, nullptr, 16 );
            Print_Command_STM( CMD_ID_value ); // Switch function for printing message based on CMD_ID
        }
    }
    else
    {
        // Extracting two Data nibbles from formated number_str
        CleanData[ 0 ] = number_str[ 2 ];
        CleanData[ 1 ] = number_str[ 3 ];
        CleanData[ 2 ] = '\0';
        CurrentDataLength = std::strlen( DataToSendUDP );
        AppendToBuffer( CleanData, DataToSendUDP, CurrentDataLength, sizeof( DataToSendUDP ) );

        // checking time difference between two incoming data and printing
        // that either new data arrived or an error occured

        number_str[ 4 ] = '\0';
        if( start_time_miliseconds_number - previous_time_miliseconds_number < 500 )
        {
            if( ( std::stoi( number_str + 2, nullptr, 16 ) == 22 ) && ( synchro1 != 22 ) && ( general_count == 1 ) )
            {
                AddTabularText( "\n\r New message:",
                                "\n\r" ); // Pritning only after general_count reset and before new message ( First synchro byte )
            }
            AddTabularText( number_str, " " );
        }
        else
        {
            AddTabularText( "Large time difference between data" );
            if( ( std::stoi( number_str + 2, nullptr, 16 ) == 22 ) && ( synchro1 != 22 ) && ( general_count == 1 ) )
            {
                AddTabularText( "\n\rNew message:",
                                "\n\r" ); // Pritning only after general_count reset and before new message ( First synchro byte )
            }
            AddTabularText( number_str, " " );
        }
        if( general_count == 3 )
        {
            CMD_ID_value = std::stoi( number_str + 2, nullptr, 16 ); // Geting CMD_ID
            sprintf( CMD_ID_buff, "%d", CMD_ID_value );              // Printing decimal version of hex CMD value into a char buffer
            // AddTabularText( "Decimalna vrednost: ", CMD_ID_buff );   // Printing decimal value
            Print_Command_STM( CMD_ID_value );                       // function for printing message based on CMD_ID
            synchro1 = std::stoi( zero_buff + 2, nullptr, 16 );
        }
    }

    // Working with incoming and previous bytes
    // Extracting two Data nibbles from formated number_str


    // synchro bytes

    // if( synchro1 == 22 )
    // {
    //     // synchro2 = std::stoi( number_str + 2, nullptr, 16 ); // synchro 2 will only have value  22 on 2nd iteration
    // }
    // else
    // {
    //     // synchro2 = std::stoi( zero_buff + 2, nullptr, 16 );
    // }
    // // synchro1 = std::stoi( number_str + 2, nullptr, 16 );


    // LEN bytes (4th and 5th)
    // 2nd LEN byte
    if( general_count == 5 ) // was k == 1
    {
        msb_LEN = std::stoi( number_str + 2, nullptr, 16 );
        msb_LEN = msb_LEN << 8; // Shifting second byte (LEN value is in little endian format)
        LEN_value = msb_LEN | lsb_LEN;

        sprintf( LEN_buff, "%d", LEN_value );
        AddTabularText( "\n\rPayload Length: ", LEN_buff, "\n\r" );

        count_to = LEN_value + 12;
    }

    // 1st LEN byte
    if( general_count == 4 ) // was if i == 4 ;
    {
        // AddTabularText("\n\rGeneral count = 4, prvi LEN byte: ", number_str , "\n\r");

        if( number_str[ 1 ] == '\0' ) // Converting LSB byte to a hex value if its in visible ASCII range
        {
            std::snprintf( LSB_LEN_buff, sizeof( LSB_LEN_buff ), "0x%02X", number_str[ 0 ] );
            lsb_LEN = std::stoi( LSB_LEN_buff + 2, nullptr, 16 );
        }
        else
        {
            lsb_LEN = std::stoi( number_str + 2, nullptr, 16 );
        }
        sprintf( LSB_LEN_buff_value, "%d", lsb_LEN );

        // AddTabularText( "LSB Byte Value: ", LSB_LEN_buff_value, "\n\r" );

        // Dodato
        // CleanData[ 0 ] = number_str[ 2 ];
        // CleanData[ 1 ] = number_str[ 3 ];
        // CleanData[ 2 ] = '\0';
        // CurrentDataLength = std::strlen( DataToSendUDP );
        // AppendToBuffer( CleanData, DataToSendUDP, CurrentDataLength, sizeof( DataToSendUDP ) );

        general_count = general_count + 1;
        return;
    }

    // new line on message end only

    // Storing buffer for sending data with UDP
    // ConcatBuffers( CleanData, DataToSendUDP );


    if( general_count == count_to )
    {
        // Converting to Bytes
        // ConvertHexStringToBytes( DataToSendUDP, HexToByteArray );
        // Adding Header Before dataso fir
        // byteArray.push_back( Hardwere_Type );
        // byteArray.push_back( Band );
        // byteArray.push_back( Hardwere_Version );
        // byteArray.push_back( Packet_Type );
        // byteArray.insert( byteArray.end(), Reserved_Bytes.begin(), Reserved_Bytes.end() );

        // Original version no header
        ConvertHexStringToBytes3( DataToSendUDP, byteArray );

        byteArray.insert(byteArray.begin(), Reserved_Bytes.begin(), Reserved_Bytes.end() );
        byteArray.insert(byteArray.begin(), Packet_Type);
        byteArray.insert(byteArray.begin(), Hardwere_Version);
        byteArray.insert(byteArray.begin(), Band);
        byteArray.insert(byteArray.begin(), Hardwere_Type);

        // Sending data with UDP

        SendUDPMessage( byteArray.data(), byteArray.size(), server_ip, server_port );
        AddTabularText( "\n\r" );

        general_count = 0;
        // payload_counter = 0;
        count_to = 0;
        // new_message_flag = 1;
        // load_payload = 0;
    }


    general_count = general_count + 1;
}

void SerialAnalyzerResults::GeneratePacketTabularText( U64 /*packet_id*/,
                                                       DisplayBase /*display_base*/ ) // unreferenced vars commented out to remove warnings.
{
    ClearResultStrings();

    AddResultString( "not supported" );
}

void SerialAnalyzerResults::GenerateTransactionTabularText(
    U64 /*transaction_id*/, DisplayBase /*display_base*/ ) // unreferenced vars commented out to remove warnings.
{
    ClearResultStrings();

    AddResultString( "not supported" );
}
