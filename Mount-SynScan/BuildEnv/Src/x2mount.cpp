#include <string.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "x2mount.h"
#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/basicstringinterface.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/basiciniutilinterface.h"
#include "../../licensedinterfaces/theskyxfacadefordriversinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/basiciniutilinterface.h"
#include "../../licensedinterfaces/mutexinterface.h"
#include "../../licensedinterfaces/tickcountinterface.h"


/////////////////////////////////////////////////////////////////////
/* Revision history
0.6     6/7/2018        Basically works, but only tested against one mount.
                        Goto works
                        Sync works
                        Is Slewing works
                        Abort slew works
                        Reports firmware on SyncScan controller
                        Makes sure mount is intialized before you can connect
                        Returns false from needsrefraction adjustments
0.7                     Logger Interface
                        Beyond the Pole
0.8                     Beyond the Pole, take 2
1.0                     General Release

*/
#define VERSION 1.0


#define DISPLAY_NAME "Software Bisque Native"


X2Mount::X2Mount(const char* pszDriverSelection,
				const int& nInstanceIndex,
				SerXInterface					* pSerX, 
				TheSkyXFacadeForDriversInterface	* pTheSkyX, 
				SleeperInterface					* pSleeper,
				BasicIniUtilInterface			* pIniUtil,
				LoggerInterface					* pLogger,
				MutexInterface					* pIOMutex,
				TickCountInterface				* pTickCount)
    {
	m_nPrivateMulitInstanceIndex	= nInstanceIndex;
	m_pSerX							= pSerX;		
	m_pTheSkyXForMounts				= pTheSkyX;
	m_pSleeper						= pSleeper;
	m_pIniUtil						= pIniUtil;
	m_pLogger						= pLogger;	
	m_pIOMutex						= pIOMutex;
	m_pTickCount					= pTickCount;
 
    strcpy(m_Firmware, "N/A");
    bEQMount = (strtok((char *)"ALT/AZ", pszDriverSelection) == NULL);
    bIsSynced = false;
    bSouthPole = false;
    }
    
X2Mount::~X2Mount()
    {
	if (m_pSerX)
		delete m_pSerX;
	if (m_pTheSkyXForMounts)
		delete m_pTheSkyXForMounts;
	if (m_pSleeper)
		delete m_pSleeper;
	if (m_pIniUtil)
		delete m_pIniUtil;
	if (m_pLogger)
		delete m_pLogger;
	if (m_pIOMutex)
		delete m_pIOMutex;
	if (m_pTickCount)
		delete m_pTickCount;
    }

int	X2Mount::queryAbstraction(const char* pszName, void** ppVal)
{
	*ppVal = NULL;

    if (!strcmp(pszName, SerialPortParams2Interface_Name))
        *ppVal = dynamic_cast<SerialPortParams2Interface*>(this);
	else if (!strcmp(pszName, SyncMountInterface_Name))
		*ppVal = dynamic_cast<SyncMountInterface*>(this);
	else if (!strcmp(pszName, SlewToInterface_Name))
		*ppVal = dynamic_cast<SlewToInterface*>(this);
    else if (!strcmp(pszName, NeedsRefractionInterface_Name))
        *ppVal = dynamic_cast<NeedsRefractionInterface*>(this);
    else if(!strcmp(pszName, LoggerInterface_Name))
        *ppVal = dynamic_cast<LoggerInterface*>(this);
//    else if(!strcmp(pszName, ParkInterface_Name))
//        *ppVal = dynamic_cast<ParkInterface*>(this);
     
    // Do not expose this interface if in Alt/Az mode
    if(bEQMount) {
        if (!strcmp(pszName, AsymmetricalEquatorialInterface2_Name))
		    *ppVal = dynamic_cast<AsymmetricalEquatorialInterface2*>(this);
        else
            if(!strcmp(pszName, BeyondThePoleInterface_Name))
            *ppVal = dynamic_cast<BeyondThePoleInterface*>(this);
        }
     
    //else if (!strcmp(pszName, OpenLoopMoveInterface_Name))
	//	*ppVal = dynamic_cast<OpenLoopMoveInterface*>(this);
	//else if (!strcmp(pszName, LinkFromUIThreadInterface_Name))
	//	*ppVal = dynamic_cast<LinkFromUIThreadInterface*>(this);
	//else if (!strcmp(pszName, TrackingRatesInterface_Name))
	//	*ppVal = dynamic_cast<TrackingRatesInterface*>(this);

	return SB_OK;
}

//LinkInterface
int	X2Mount::establishLink(void)					
    {
    int nRet = SB_OK;
    
    if (m_pIniUtil)
        m_pIniUtil->readString(PARENT_KEY, CHILD_KEY_PORT_NAME, NULL, (char*)m_PortName, MAX_PORT_NAME_SIZE);
    else
        return ERR_MEMORY;
        
    if((nRet = m_pSerX->open(m_PortName, 9600, SerXInterface::B_NOPARITY, "-DTR_CONTROL 1")) != SB_OK)
        return nRet;
        
    char cResponseString[64];
    
    // Hey, are we aligned?
    if((nRet = SendCommand("J", cResponseString, 2)))
        return nRet;
    
    if(cResponseString[0] == 0)
        return ERR_NOTINITIALIZED;

    // Save the current tracking mode
    if((nRet = SendCommand("t", cResponseString, 2)))
        return nRet;
        
//    printf("Tracking mode: %d\n", cResponseString[0]);
    modeTracking = cResponseString[0];

    // Get firmware version
    int nMajor, nMinor, nSub;
    if((nRet = SendCommand("V", cResponseString, 7)))
      return nRet;
      
    sscanf(cResponseString+4, "%X", &nSub);
    cResponseString[4] = NULL;
    sscanf(cResponseString+2, "%X", &nMinor);
    cResponseString[2] = NULL;
    sscanf(cResponseString, "%X", &nMajor);
    sprintf(m_Firmware, "%02d.%02d.%02d", nMajor, nMinor, nSub);
    
    // Are we in the southern hemisphere?
    if((nRet = SendCommand("w", cResponseString, 9)))
        return nRet;

    if(cResponseString[3] == 1) 
        bSouthPole = true;
    
    // We are ready...
	
    return SB_OK;
    }
    
int	X2Mount::terminateLink(void)						
    {
    X2MutexLocker ml(GetMutex());
    m_pSerX->close();
	return SB_OK;
    }
    
bool X2Mount::isLinked(void) const					
    {
    //X2MutexLocker ml(GetMutex());

	return m_pSerX->isConnected();
    }
    
bool X2Mount::isEstablishLinkAbortable(void) const	{return false;}

//AbstractDriverInfo
    
int	X2Mount::abort(void)												
    {
    X2MutexLocker ml(GetMutex());

    char cNota[8];
	return SendCommand("M", cNota, 1);
    }

/////////////////////////////////////////////////////////////////////////////////////////////
// DONE DONE DONE DONE DONE DONE DONE
// Well, at least working ;-)
/////////////////////////////////////////////////////////////////////////////////////////////

// Send a command... get a response... maybe time out...
// Note, return length should include the # terminator, which is replaced by NULL
int X2Mount::SendCommand(const char *szCommandString, char *szResponsString, int nRespLength)
    {
    int nRet = SB_OK;
    
    unsigned long toWrite = strlen(szCommandString);
    unsigned long wrote = 0;
    
    m_pSerX->purgeTxRx();   // Clear everything from the buffers
    
    // Write the file
    sprintf(szLogString, "Command to mount: %s", szCommandString);
    out(szLogString);
    if((nRet = m_pSerX->writeFile((void*)szCommandString, toWrite, wrote)))
        return nRet;
        
    m_pSerX->flushTx();     // Force flush to device in case OS is being a PIA
    
    // Wait up to 5 seconds for a response (specified by SkyWatcher docs)
    if((nRet = m_pSerX->waitForBytesRx(nRespLength, 5000))) {
        out((char *)"Timed out waiting for bytes.");
        return nRet;
        }
        
    // Read the response
    memset(szResponsString, 0, nRespLength+1);    // For DEBUGG...
    if((nRet = m_pSerX->readFile((void*)szResponsString, nRespLength, wrote)))
        return nRet;
        
    // Some responses are ASCI, some are straight values. Display both...
    sprintf(szLogString, "Response string: %s\n", szResponsString);
    out(szLogString);
    sprintf(szLogString, "Response values: %d", szResponsString[0]);
    for(unsigned int i = 1; i < strlen(szResponsString); i++) {
        char cValue[8];
        sprintf(cValue, ", %d", szResponsString[i]);
        strcat(szLogString, cValue);
        }
    out(szLogString);
    
    // Last character should be a '#'. Check and replace with NULL
    if(szResponsString[nRespLength-1] != '#') {
        out((char *)"Invalid response string.");
        return ERR_CMDFAILED;
        }
        
    szResponsString[nRespLength-1] = NULL;        
    return SB_OK;
    }

#pragma mark - SlewInterfaces
///////////////////////////////////////////////////////////////////////
// Report RA/Dec from mount
int X2Mount::raDec(double& ra, double& dec, const bool& bCached)
    {
    X2MutexLocker ml(GetMutex());

    int nRet = SB_OK;
    char cResponseString[64];
    
    // Get position
    if((nRet = SendCommand("e", cResponseString, 18)))
        return nRet;
        
    // Parse RA/Dec
    unsigned int rawRA, rawDEC;
    cResponseString[6] = NULL;
    sscanf(cResponseString, "%x", &rawRA);
    cResponseString[15] = NULL;
    sscanf(cResponseString+9, "%x", &rawDEC);

    ra = (rawRA/16777216.0) * 24.0;
    dec = (rawDEC/16777216.0) * 360.0;
    if(dec > 180.0)
        dec-= 360.0;
    
    // Ensure in range
    if (ra < 0) {
        ra += 24.0;
        }
    else if (ra > 24.0) 
        ra -= 24.0;
        
    return SB_OK;
    }


/////////////////////////////////////////////////////////
int X2Mount::startSlewTo(const double& dRa, const double& dDec)
    {
    X2MutexLocker ml(GetMutex());
    double dc = dDec;
    if(dc < 0.0)
        dc = 360.0 + dc;
        
    unsigned int rawRA = floor(dRa/24.0 * 16777216.0) + 0.5;
    unsigned int rawDC = floor(dc/360.0 * 16777216.0) + 0.5;
    
    char cCommand[32], cNone[32];
    sprintf(cCommand, "r%06X00,%06X00#", rawRA, rawDC);
    return SendCommand(cCommand, cNone, 1);    
    }


int X2Mount::startSlewToAltAz(const double& dAlt, const double& dAz)
    {
    X2MutexLocker ml(GetMutex());
    
    
    return SB_OK;
    }


/////////////////////////////////////////////////////////
int X2Mount::isCompleteSlewTo(bool& bComplete) const
    {
    X2MutexLocker ml(GetMutex()); // I marked this const so I still get thread protection
    
    char cResponse[8];
    X2Mount *P = (X2Mount *)this;
    P->SendCommand("L", cResponse, 2);
    if(cResponse[0] == '1')
        bComplete = false;
    else    
        bComplete = true;
        
    return SB_OK;
    }

/////////////////////////////////////////////////////////
int X2Mount::endSlewTo(void)
    {
    return SB_OK;   // Whatever...
    }


#pragma mark SyncMountInterface
//////////////////////////////////////////////////////////
// Sync to this location
int X2Mount::syncMount(const double& ra, const double& dec)
    {
    X2MutexLocker ml(GetMutex());
    
    double dc = dec;
    if(dc < 0.0)
        dc = 360.0 + dc;
        
    unsigned int rawRA = floor(ra/24.0 * 16777216.0) + 0.5;
    unsigned int rawDC = floor(dc/360.0 * 16777216.0) + 0.5;
    
    char cCommand[32], cNone[32];
    sprintf(cCommand, "s%06X00,%06X00#", rawRA, rawDC);
        
    int nRet = SendCommand(cCommand, cNone, 1);
    if(nRet) {
        bIsSynced = false;
        return nRet;
        }
        
    bIsSynced = true;
    return SB_OK;
    }
    
// Are you synced?
bool X2Mount::isSynced()             
    {
    return bIsSynced;
    }


#pragma mark - SerialPortParams2Interface
void X2Mount::portName(BasicStringInterface& str) const
    {
    if (m_pIniUtil)
        m_pIniUtil->readString(PARENT_KEY, CHILD_KEY_PORT_NAME, NULL, (char*)m_PortName, MAX_PORT_NAME_SIZE);

    str = m_PortName;
    }

void X2Mount::setPortName(const char* pszPort)
    {
    if (m_pIniUtil)
        m_pIniUtil->writeString(PARENT_KEY, CHILD_KEY_PORT_NAME, pszPort);
    }

#pragma mark - Detailed Device information
void    X2Mount::driverInfoDetailedInfo(BasicStringInterface& str) const
{
    str = DISPLAY_NAME;
}
double    X2Mount::driverInfoVersion(void) const                
{
    return VERSION;
}

//AbstractDeviceInfo
void X2Mount::deviceInfoNameShort(BasicStringInterface& str) const                
{
    str = "Sky-Watcher";
}
void X2Mount::deviceInfoNameLong(BasicStringInterface& str) const                
{
    str = "Sky-Watcher Mount";

}
void X2Mount::deviceInfoDetailedDescription(BasicStringInterface& str) const    
{
    if(bEQMount)
        str = "Equatorial Mount";
    else
        str = "Alt/Az Mount";
}

void X2Mount::deviceInfoFirmwareVersion(BasicStringInterface& str)        
{
    str = m_Firmware;
}
void X2Mount::deviceInfoModel(BasicStringInterface& str)                
{
    str = "SyncScan Controller";
}

#pragma mark LoggerInterface

/*! Have a string logged in TheSkyX's Communication Log window.*/
int X2Mount::out(const char* szLogThis)
    {
    GetLogger()->out(szLogThis);
    return SB_OK;
    }
    
/*! Return the number of packets, retries and failures associated with device io if appropriate.*/
void X2Mount::packetsRetriesFailuresChanged(const int& p, const int& r, const int& f)
    {
    // N/A
    }
    
#pragma mark AsymmetricalEquatorialInterface
bool X2Mount::knowsBeyondThePole() { return true; }
int X2Mount::beyondThePole(bool& bYes)
    {
    // I think this is for all intents and purposes, pier west...
    char szResponse[16];
    int nRet = SB_OK;
    if((nRet = SendCommand("p", szResponse, 2)))
        return nRet;
        
    if(szResponse[0] == 'W')
        bYes = true;
    else
        bYes = false;
        
    // Flipped for southern hemisphere
    if(bSouthPole) 
        bYes = !bYes;
    
    return SB_OK;
    }
/*
//!Return true if the device is parked.
bool X2Mount::isParked(void)
    {
    return false;
    }
    
//!Initiate the park process.
int  X2Mount::startPark(const double& dAz, const double& dAlt)
    {
    
    return SB_OK;
    }
    
    
//!Called to monitor the park process.  \param bComplete Set to true if the park is complete, otherwise set to false.
int  X2Mount::isCompletePark(bool& bComplete) const
    {
    return isCompleteSlewTo(bComplete);
    }
    
//!Called once the park is complete.  This is called once for every corresponding startPark() allowing software implementations of park.
int X2Mount::endPark(void)
    {
    
    
    return SB_OK;
    }
    
*/
