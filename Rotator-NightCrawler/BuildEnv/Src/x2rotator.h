#if !defined(_X2Rotator_H_)
#define _X2Rotator_H_
#include <stdlib.h>		
#include <string.h>
#include "../../../../licensedinterfaces/sberrorx.h"
#include "../../../../licensedinterfaces/rotatordriverinterface.h"
#include "../../../../licensedinterfaces/modalsettingsdialoginterface.h"
#include "../../../../licensedinterfaces/x2guiinterface.h"
#include "../../../../licensedinterfaces/serialportparams2interface.h"
#include "../../../../licensedinterfaces/basiciniutilinterface.h"
#include "../../../../licensedinterfaces/modalsettingsdialoginterface.h"
#include "../../../../licensedinterfaces/multiconnectiondeviceinterface.h"

#include "../../../../focuserplugins/NightCrawler/BuildEnv/Src/x2focuser.h"

#define PLUGIN_DISPLAY_NAME "MoonLite NiteCrawler Rotator"
#define MAX_SERIAL_PATH        512


class SerXInterface;
class TheSkyXFacadeForDriversInterface;
class SleeperInterface;
class BasicIniUtilInterface;
class LoggerInterface;
class MutexInterface;
class SyncMountInterface;
class TickCountInterface;
class X2GUIExchangeInterface;

/*!
\brief The X2Rotator example.

\ingroup Example

Use this example to write an X2Rotator driver.
*/
class NightCrawlerRotatorPlugIn : public RotatorDriverInterface, public SerialPortParams2Interface, public ModalSettingsDialogInterface, public X2GUIEventInterface, public MultiConnectionDeviceInterface
{
	// Construction
public:

	/*!Standard X2 constructor*/
	NightCrawlerRotatorPlugIn(	const char*							pszDriverSelection,
								const int&							nInstanceIndex,
								SerXInterface						* pSerX,
								TheSkyXFacadeForDriversInterface	* pTheSkyX,
								SleeperInterface					* pSleeper,
								BasicIniUtilInterface				* pIniUtil,
								LoggerInterface						* pLogger,
								MutexInterface						* pIOMutex,
								TickCountInterface					* pTickCount);

	virtual ~NightCrawlerRotatorPlugIn();

public:

	// Operations
public:

	/*!\name DriverRootInterface Implementation
	See DriverRootInterface.*/
	//@{ 
	virtual DeviceType							deviceType(void)							  { return DriverRootInterface::DT_ROTATOR; }
	virtual int									queryAbstraction(const char* pszName, void** ppVal);
	//@} 

	/*!\name DriverInfoInterface Implementation
	virtual int deviceIdentifier(BasicStringInterface &sIdentifier) {
        printf("Asking the focuser for the deviceIdentifier\r\n");
        sIdentifier = "OptecGeminiRotatorFocuser";
        return SB_OK;
        }
See DriverInfoInterface.*/
	//@{ 
	virtual void								driverInfoDetailedInfo(BasicStringInterface& str) const;
	virtual double								driverInfoVersion(void) const;
	//@} 

	/*!\name HardwareInfoInterface Implementation
	See HardwareInfoInterface.*/
	//@{ 
	virtual void deviceInfoNameShort(BasicStringInterface& str) const;
	virtual void deviceInfoNameLong(BasicStringInterface& str) const;
	virtual void deviceInfoDetailedDescription(BasicStringInterface& str) const;
	virtual void deviceInfoFirmwareVersion(BasicStringInterface& str);
	virtual void deviceInfoModel(BasicStringInterface& str);
	//@} 

	/*!\name LinkInterface Implementation
	See LinkInterface.*/
	//@{ 
	virtual int									establishLink(void);
	virtual int									terminateLink(void);
	virtual bool								isLinked(void) const;
	virtual bool								isEstablishLinkAbortable(void) const	{ return false; }
	//@}

	virtual int								initModalSettingsDialog(void){ return 0; }
	virtual int								execModalSettingsDialog(void);
	virtual void uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent);


	/*!\name RotatorDriverInterface Implementation
	See RotatorDriverInterface.*/
	//@{ 
	virtual int									position(double& dPosition);
	virtual int									abort(void);

	virtual int									startRotatorGoto(const double& dTargetPosition);
	virtual int									isCompleteRotatorGoto(bool& bComplete) const;
	virtual int									endRotatorGoto(void);
	//@} 


	virtual int deviceIdentifier(BasicStringInterface &sIdentifier) {
        sIdentifier = "NiteCrawlerRotatorFocuser";
        return SB_OK;
        }
	virtual int isConnectionPossible(const int &nPeerArraySize, MultiConnectionDeviceInterface **ppPeerArray, bool &bConnectionPossible);
	virtual int useResource(MultiConnectionDeviceInterface *pPeer);
	virtual int swapResource(MultiConnectionDeviceInterface *pPeer);

	SerXInterface*							m_pSavedSerX;
	MutexInterface*							m_pSavedMutex;

	/*****************************************************************************************/
	// Implementation

	/*!Return serial port name as a string.*/
	virtual void			portName(BasicStringInterface& str) const { str = serialName; }
	/*!Set the serial port name as a string.*/
	virtual void			setPortName(const char* szPort)	{
		strncpy(serialName, szPort, MAX_SERIAL_PATH);
		if (m_pIniUtil)
			m_pIniUtil->writeString(PARENT_KEY_STRING, SERIAL_NAME, serialName);
	}

	virtual unsigned int	baudRate() const			{ return 57600; };
	virtual void			setBaudRate(unsigned int)	{};
	virtual bool			isBaudRateFixed() const		{ return true; }

	virtual void			setParity(const SerXInterface::Parity& parity) {};

	virtual SerXInterface::Parity	parity() const				{ return SerXInterface::B_NOPARITY; }
	virtual bool					isParityFixed() const		{ return true; }

	MutexInterface							*GetMutex()  { return m_pIOMutex; }

	//Use default impl of SerialPortParams2Interface for rest

	//@} 
private:
	const int                               m_nPrivateISIndex;
	SerXInterface*							m_pSerX;
	TheSkyXFacadeForDriversInterface* 		m_pTheSkyXForMounts;
	SleeperInterface*						m_pSleeper;
	BasicIniUtilInterface*                  m_pIniUtil;
	LoggerInterface*						m_pLogger;
	MutexInterface*							m_pIOMutex;
	TickCountInterface*						m_pTickCount;

	SerXInterface 							*GetSerX() { return m_pSerX; }
	TheSkyXFacadeForDriversInterface		*GetTheSkyXFacadeForDrivers() { return m_pTheSkyXForMounts; }
	SleeperInterface						*GetSleeper() { return m_pSleeper; }
	BasicIniUtilInterface					*GetSimpleIniUtil() { return m_pIniUtil; }
	LoggerInterface							*GetLogger() { return m_pLogger; }
	TickCountInterface						*GetTickCountInterface() { return m_pTickCount; }
    
    int WriteCommand(const char *szCommand, char *cResponse = NULL, unsigned long expectedBytes = 0) const;


	bool m_bLinked;
	int m_nInstanceIndex;
	double m_dPosition;
	bool m_bDoingGoto;
	double m_dTargetPosition;
	int m_nGotoStartStamp;
	char                                    serialName[MAX_SERIAL_PATH];
    int nStepsCount;                        // Steps per revolution

    char cFirmware[32];
    char cFocuserType[32];
    char cSerialNumber[32];

};




#endif
