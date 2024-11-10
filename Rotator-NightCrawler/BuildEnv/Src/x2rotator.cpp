#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "x2rotator.h"

#include "../../../../licensedinterfaces/serxinterface.h"
#include "../../../../licensedinterfaces/theskyxfacadefordriversinterface.h"
#include "../../../../licensedinterfaces/sleeperinterface.h"
#include "../../../../licensedinterfaces/loggerinterface.h"
#include "../../../../licensedinterfaces/basiciniutilinterface.h"
#include "../../../../licensedinterfaces/mutexinterface.h"
#include "../../../../licensedinterfaces/basicstringinterface.h"
#include "../../../../licensedinterfaces/tickcountinterface.h"
#include "../../../../licensedinterfaces/x2guiinterface.h"



// 1.01, fixed incorrect rotator steps conversion
#define DRIVER_VERSION      1.02
// 1.02 Rebuild since shared header with focuser has been changed.




NightCrawlerRotatorPlugIn::NightCrawlerRotatorPlugIn(const char* pszDriverSelection,
						const int& nInstanceIndex,
						SerXInterface					* pSerX, 
						TheSkyXFacadeForDriversInterface	* pTheSkyX, 
						SleeperInterface					* pSleeper,
						BasicIniUtilInterface			* pIniUtil,
						LoggerInterface					* pLogger,
						MutexInterface					* pIOMutex,
						TickCountInterface				* pTickCount)
: m_nPrivateISIndex(nInstanceIndex)
{
	m_nInstanceIndex				= nInstanceIndex;
	m_pSerX							= pSerX;		
	m_pTheSkyXForMounts				= pTheSkyX;
	m_pSleeper						= pSleeper;
	m_pIniUtil						= pIniUtil;
	m_pLogger						= pLogger;	
	m_pIOMutex						= pIOMutex;
	m_pTickCount					= pTickCount;
	m_pSavedMutex					= pIOMutex;
	m_pSavedSerX					= pSerX;


	m_nInstanceIndex = nInstanceIndex;
	m_bLinked = false;
	m_dPosition = 0;
	m_bDoingGoto = false;
	m_dTargetPosition = 0;
	m_nGotoStartStamp = 0;
    nStepsCount = 0;
    
       // Read in settings
    if (m_pIniUtil) {
		m_pIniUtil->readString(PARENT_KEY_STRING, SERIAL_NAME, "", serialName, MAX_SERIAL_PATH);
        }

 
	
}

NightCrawlerRotatorPlugIn::~NightCrawlerRotatorPlugIn()
{
	if (m_pSavedSerX)
		delete m_pSavedSerX;
	if (m_pSavedMutex)
		delete m_pSavedMutex;
	if (GetTheSkyXFacadeForDrivers())
		delete GetTheSkyXFacadeForDrivers();
	if (GetSleeper())
		delete GetSleeper();
	if (GetSimpleIniUtil())
		delete GetSimpleIniUtil();
	if (GetLogger())
		delete GetLogger();
	if (GetTickCountInterface())
		delete GetTickCountInterface();

}

int	NightCrawlerRotatorPlugIn::queryAbstraction(const char* pszName, void** ppVal)
{
	*ppVal = NULL;

	if (!strcmp(pszName, SerialPortParams2Interface_Name))
		*ppVal = dynamic_cast<SerialPortParams2Interface*>(this);
	else if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
		*ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);
	else if (!strcmp(pszName, X2GUIEventInterface_Name))
		*ppVal = dynamic_cast<X2GUIEventInterface*>(this);
	else if (!strcmp(pszName, MultiConnectionDeviceInterface_Name))
		*ppVal = dynamic_cast<MultiConnectionDeviceInterface*>(this);


	return SB_OK;
}


///////////////////////////////////////////////////////////////////////////
// Send command and receive respone. Returns false on error
// cResponse may be NULL & expectedBytes may be zero
// expectedBytes is basically non zero if a value is returned. It is the maximum number of bytes allowed.
int NightCrawlerRotatorPlugIn::WriteCommand(const char *szCommand, char *cResponse, unsigned long expectedBytes) const
{
	unsigned long writeBytes = strlen(szCommand);
	unsigned long wroteBytes = 0;
	if (cResponse != NULL)
		strncpy(cResponse, "BadFOOD", expectedBytes);

	// Always start with a clean slate... $%#@ happens.
	m_pSerX->purgeTxRx();

	// Write command, check for error
	m_pSerX->writeFile((void *)szCommand, writeBytes, wroteBytes);
	m_pSerX->flushTx();
	if (writeBytes != wroteBytes)
		return ERR_DATAOUT;


	// read result, check for error
	unsigned long readBytes = expectedBytes;
	int nTimeout = 5000;    // 3 seconds
	if (strcmp(szCommand, "SH#") == 0)
		nTimeout = 3600000; // 10 Minutes for home

							// Wait for at least one byte to show up back. The device will
							// always acknowledge with a #
	readBytes = 0;
	if (0 != m_pSerX->waitForBytesRx(1, nTimeout)) {
		printf("Timed out on command: %s\r\n", szCommand);
		return ERR_RXTIMEOUT;
	}

	char byte = 0;
	do {
		unsigned long rb = 0;
		if (SB_OK != m_pSerX->readFile(&byte, 1, rb) || rb != 1)
			return ERR_RXTIMEOUT;

		// Don't go past end of buffer
		if (readBytes < expectedBytes && cResponse != nullptr)
			cResponse[readBytes] = byte;

		readBytes++;
	} while (byte != '#');


	// Always NULL terminate the response
	if (cResponse != nullptr) {
		cResponse[readBytes] = NULL;
		if (readBytes > 0)
			cResponse[readBytes - 1] = NULL;

		if (strncmp(cResponse, "NACK", 4) == 0)
			return ERR_UNKNOWNCMD;
	}

	return SB_OK;
    }


//! [useResource]
int NightCrawlerRotatorPlugIn::useResource(MultiConnectionDeviceInterface *pPeer)
{
	NightCrawlerFocuserPlugIn *pFocuserPeer = dynamic_cast<NightCrawlerFocuserPlugIn*>(pPeer);
	if (pFocuserPeer == NULL) {
        return ERR_POINTER; // Peer must be a focuser pointer
        }
    
    // Use the resources held by the specified peer
	m_pIOMutex = pFocuserPeer->m_pSavedMutex;
	m_pSerX = pFocuserPeer->m_pSavedSerX;
	return SB_OK;
}
//! [useResource]

//! [swapResource]
int NightCrawlerRotatorPlugIn::swapResource(MultiConnectionDeviceInterface *pPeer)
{
	NightCrawlerFocuserPlugIn *pFocuserPeer = dynamic_cast<NightCrawlerFocuserPlugIn*>(pPeer);
	if (pFocuserPeer == NULL) {
        return ERR_POINTER; // Peer must be a OptecGeminiFocuserPlugIn pointer
        }
											   // Swap this driver instance's resources for the ones held by pPeer
	MutexInterface* pTempMutex = m_pSavedMutex;
	SerXInterface*  pTempSerX = m_pSavedSerX;
	m_pSavedMutex = pFocuserPeer->m_pSavedMutex;
	m_pSavedSerX = pFocuserPeer->m_pSavedSerX;
	pFocuserPeer->m_pSavedMutex = pTempMutex;
	pFocuserPeer->m_pSavedSerX = pTempSerX;
	return SB_OK;
}
//! [swapResource]


//! [isConnectionPossible]
// Much simpler for the gemini case.
int NightCrawlerRotatorPlugIn::isConnectionPossible(const int &nPeerArraySize, MultiConnectionDeviceInterface **ppPeerArray, bool &bConnectionPossible)
    {
		for (int nIndex = 0; nIndex < nPeerArraySize; ++nIndex)
            {
			NightCrawlerFocuserPlugIn *pPeer = dynamic_cast<NightCrawlerFocuserPlugIn*>(ppPeerArray[nIndex]);
			if (pPeer == NULL)
                {
				bConnectionPossible = false;
				return ERR_POINTER;
                }
            }

	bConnectionPossible = true;
	return SB_OK;
    }

//! [isConnectionPossible]




#define UNKNOWNPOSITION -1

int	NightCrawlerRotatorPlugIn::establishLink(void)
    {
    int nErr = SB_OK;
	m_bLinked = false;
	X2MutexLocker ml(GetMutex());

	if (!m_pSerX->isConnected()) {
		if (SB_OK != m_pSerX->open(serialName, baudRate()))
    		return ERR_NOLINK;
        }
    
    const char* cmdGetType = "PF#";
    if(SB_OK != (nErr =WriteCommand(cmdGetType,cFocuserType, 7)))
        return nErr;

    cFocuserType[6] = NULL;

    if(strncmp(cFocuserType, "2.5", 3) == 0)
        nStepsCount = 374920;
    else
        if(strncmp(cFocuserType, "3.0", 3) == 0)
            nStepsCount = 444080;
        else
            nStepsCount = 505960;

	//printf("Focuser/Rotator Type: %s, steps %d\n", cFocuserType, nStepsCount);

    const char* cmdGetFirmware = "PV#";
    if(SB_OK != (nErr = WriteCommand(cmdGetFirmware, cFirmware, 3)))
        return nErr;

    cFirmware[3] = NULL;
    
    const char* cmdGetSerialNumber = "PS#";
    if(SB_OK != (nErr = WriteCommand(cmdGetSerialNumber, cSerialNumber, 6)))
        return nErr;
    
    cSerialNumber[5] = NULL;
		
	//We made it. Now we are connected and shall report as such.
	m_bLinked = true;
    return nErr;
    }

int	NightCrawlerRotatorPlugIn::terminateLink(void)
{
	X2MutexLocker ml(GetMutex());

	if (m_pSerX && m_nInstanceCount == 1)
	{
		if (m_pSerX->isConnected())
			m_pSerX->close();
	}

	// We're not connected, so revert to our saved interfaces
	m_pSerX = m_pSavedSerX;
	m_pIOMutex = m_pSavedMutex;

	m_bLinked = false;
	return SB_OK;
}
bool NightCrawlerRotatorPlugIn::isLinked(void) const
{
	return m_bLinked;
}

void NightCrawlerRotatorPlugIn::deviceInfoNameShort(BasicStringInterface& str) const
{
	str = "NightCrawler Rotator"; // I don't think this one actually shows up anywhere
}

void NightCrawlerRotatorPlugIn::deviceInfoNameLong(BasicStringInterface& str) const
{
	str = PLUGIN_DISPLAY_NAME;
}
void NightCrawlerRotatorPlugIn::deviceInfoDetailedDescription(BasicStringInterface& str) const
{
	str = PLUGIN_DISPLAY_NAME;
}
void NightCrawlerRotatorPlugIn::deviceInfoFirmwareVersion(BasicStringInterface& str)
    {
	X2MutexLocker ml(GetMutex());
	if (!isLinked())
		str = "Unknown";
	else
		str = cFirmware;
    }

void NightCrawlerRotatorPlugIn::deviceInfoModel(BasicStringInterface& str)
    {
	X2MutexLocker ml(GetMutex());

    if(m_bLinked)
        str = cFocuserType;
    else
        str = "Not connected";
    }

void NightCrawlerRotatorPlugIn::driverInfoDetailedInfo(BasicStringInterface& str) const
    {
	str = "Software Bisque Native";
    }

double NightCrawlerRotatorPlugIn::driverInfoVersion(void) const
    {
	return DRIVER_VERSION;
    }

int	NightCrawlerRotatorPlugIn::position(double& dPosition)
    {
	X2MutexLocker ml(GetMutex());
    int nErr = SB_OK;
    
	if (!m_bLinked)
		return ERR_NOLINK;
    
    char cResponse[16];
    const char* cmdGetPosition = "2GP#";
    if(SB_OK != (nErr = WriteCommand(cmdGetPosition, cResponse, 9)))
        return ERR_CMDFAILED;
        
    cResponse[9] = NULL;
    int nSteps = atoi(cResponse);


    dPosition = double(nSteps) / double(nStepsCount) * 360.0;
    
	return nErr;
    }

int	NightCrawlerRotatorPlugIn::abort(void)
    {
	X2MutexLocker ml(GetMutex());
    int nErr = SB_OK;
    
	if (!m_bLinked)
		return ERR_NOLINK;
	
    const char* cmdStopMotors = "2SQ#";
    if(SB_OK != (nErr = WriteCommand(cmdStopMotors)))
        return ERR_CMDFAILED;

	return nErr;
    }

int	NightCrawlerRotatorPlugIn::startRotatorGoto(const double& dTargetPosition)
    {
	X2MutexLocker ml(GetMutex());
    int nErr = SB_OK;
    
	if (!m_bLinked)
		return ERR_NOLINK;
        
    double dTargetSteps = dTargetPosition / 360.0;
    dTargetSteps *= double(nStepsCount);
    int nTargetSteps = int(dTargetSteps);
    
    char cCmd[32];
    sprintf(cCmd, "2SN %08d#", nTargetSteps);
      if(SB_OK != (nErr = WriteCommand(cCmd)))
        return nErr;

    if(SB_OK != (nErr = WriteCommand("2SM#")))
        return nErr;


	return nErr;
    }

int	NightCrawlerRotatorPlugIn::isCompleteRotatorGoto(bool& bComplete) const
    {
	X2MutexLocker ml(((NightCrawlerRotatorPlugIn*)this)->GetMutex());
    int nErr = SB_OK;
    
	if (!m_bLinked)
		return ERR_NOLINK;

    char cResponse[8];
    if(SB_OK != (nErr = WriteCommand("2GM#", cResponse, 4)))
        return nErr;
    
    bComplete = (cResponse[1] == '0');

	return nErr;
    }

int	NightCrawlerRotatorPlugIn::endRotatorGoto(void)
{
	X2MutexLocker ml(GetMutex());

	return SB_OK;
}


int NightCrawlerRotatorPlugIn::execModalSettingsDialog()
{
	X2MutexLocker ml(GetMutex());

	int nErr = SB_OK;
	X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
	X2GUIInterface*					ui = uiutil.X2UI();
	X2GUIExchangeInterface*			dx = NULL;//Comes after ui is loaded
	bool bPressedOK = false;

	if (NULL == ui)
		return ERR_POINTER;

	if ((nErr = ui->loadUserInterface("NCRotator.ui", deviceType(), m_nPrivateISIndex)))
		return nErr;

	if (NULL == (dx = uiutil.X2DX()))
		return ERR_POINTER;
    
    if(!m_bLinked)
        dx->setEnabled("groupBox", false);
    else
        {
        char cResponse[16];
        
        // Step Delay
        if(SB_OK != (nErr = WriteCommand("2GR#", cResponse, 4)))
            return nErr;
        
        cResponse[3] = NULL;
        int nStepDelay = atoi(cResponse);
        dx->setPropertyInt("spinBox_MotorStep", "value", nStepDelay);
        }

	//Display the user interface
	if((nErr = ui->exec(bPressedOK)))
		return nErr;

	//Retrieve values from the user interface
	if (bPressedOK && m_bLinked)
        {
        int nMotorStep;
        dx->propertyInt("spinBox_MotorStep", "value", nMotorStep);

        // Motor step values
        char cCmdOut[32];
        sprintf(cCmdOut, "2SR %3d#", nMotorStep);
        if(SB_OK != (nErr = WriteCommand(cCmdOut)))
            return nErr;
        }

	return nErr;
}



void NightCrawlerRotatorPlugIn::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
    {
//	if (!strcmp(pszEvent, "on_pushButton_clicked"))
//		 findDevices(uiex);
    }
