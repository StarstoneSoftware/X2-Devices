#if !defined(_X2Focuser_H_)
#define _X2Focuser_H_
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../../../../licensedinterfaces/sberrorx.h"
#include "../../../../licensedinterfaces/focuserdriverinterface.h"
#include "../../../../licensedinterfaces/modalsettingsdialoginterface.h"
#include "../../../../licensedinterfaces/x2guiinterface.h"
#include "../../../../licensedinterfaces/focuser/focusertemperatureinterface.h"
#include "../../../../licensedinterfaces/serialportparams2interface.h"
#include "../../../../licensedinterfaces/basiciniutilinterface.h"
#include "../../../../licensedinterfaces/modalsettingsdialoginterface.h"
#include "../../../../licensedinterfaces/multiconnectiondeviceinterface.h"

// Forward declare the interfaces that this device is dependent upon
class SerXInterface;
class TheSkyXFacadeForDriversInterface;
class SleeperInterface;
class BasicIniUtilInterface;
class LoggerInterface;
class MutexInterface;
class TickCountInterface;
class X2GUIExchangeInterface;


#define PARENT_KEY_STRING   "NIGHTCRAWLER_FOC"
#define SERIAL_NAME         "PORT_NAME"
#define HOME_STARTUP        "HOMEME"
#define LOCK_ENCODER_KNOB   "LOCKENCODER"

#define MAX_SERIAL_PATH        512

class NightCrawlerFocuserPlugIn : public FocuserDriverInterface, public SerialPortParams2Interface, public ModalSettingsDialogInterface, public X2GUIEventInterface, public FocuserTemperatureInterface, public MultiConnectionDeviceInterface
{
public:
	NightCrawlerFocuserPlugIn(	const char*							pszDriverSelection,
								const int&							nInstanceIndex,
								SerXInterface						* pSerX,
								TheSkyXFacadeForDriversInterface	* pTheSkyX,
								SleeperInterface					* pSleeper,
								BasicIniUtilInterface				* pIniUtil,
								LoggerInterface						* pLogger,
								MutexInterface						* pIOMutex,
								TickCountInterface					* pTickCount);

	~NightCrawlerFocuserPlugIn();

public:

	/*!\name DriverRootInterface Implementation
	See DriverRootInterface.*/
	//@{ 
	virtual DeviceType							deviceType(void)							  { return DriverRootInterface::DT_FOCUSER; }
	virtual int									queryAbstraction(const char* pszName, void** ppVal);
	//@} 

	/*!\name DriverInfoInterface Implementation
	See DriverInfoInterface.*/
	//@{ 
	virtual void								driverInfoDetailedInfo(BasicStringInterface& str) const;
	virtual double								driverInfoVersion(void) const							;
	//@} 

	/*!\name HardwareInfoInterface Implementation
	See HardwareInfoInterface.*/
	//@{ 
	virtual void deviceInfoNameShort(BasicStringInterface& str) const				;
	virtual void deviceInfoNameLong(BasicStringInterface& str) const				;	
	virtual void deviceInfoDetailedDescription(BasicStringInterface& str) const		;
	virtual void deviceInfoFirmwareVersion(BasicStringInterface& str)				;
	virtual void deviceInfoModel(BasicStringInterface& str)							;
	//@} 

	/*!\name LinkInterface Implementation
	See LinkInterface.*/
	//@{ 
	virtual int									establishLink(void);
	virtual int									terminateLink(void);
	virtual bool								isLinked(void) const;
	virtual bool								isEstablishLinkAbortable(void) const	{ return false; }
	//@} 

	/*!\name FocuserGotoInterface2 Implementation
	See FocuserGotoInterface2.*/
	virtual int									focPosition(int& nPosition) 			;
	virtual int									focMinimumLimit(int& nMinLimit) 		;
	virtual int									focMaximumLimit(int& nMaxLimit)			;
	virtual int									focAbort()								;

	virtual int								startFocGoto(const int& nRelativeOffset)	;
	virtual int								isCompleteFocGoto(bool& bComplete) const	;
	virtual int								endFocGoto(void)							;

	virtual int								amountCountFocGoto(void) const				;
	virtual int								amountNameFromIndexFocGoto(const int& nZeroBasedIndex, BasicStringInterface& strDisplayName, int& nAmount);
	virtual int								amountIndexFocGoto(void)					;

	virtual int								initModalSettingsDialog(void){return 0;}
	virtual int								execModalSettingsDialog(void);
    
	virtual void uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent);

    virtual int focTemperature(double &dTemperature);
    
    
	/*!Return serial port name as a string.*/
	virtual void			portName(BasicStringInterface& str) const { str = serialName; }
	/*!Set the serial port name as a string.*/
	virtual void			setPortName(const char* szPort)	{ strncpy(serialName, szPort, MAX_SERIAL_PATH);
                                                            if (m_pIniUtil)
                                                                m_pIniUtil->writeString(PARENT_KEY_STRING, SERIAL_NAME,  serialName);
                                                            }

    
    

	/*!Return the buad rate.*/
	virtual unsigned int	baudRate() const { return 57600; }
	/*!Set the baud rate.*/
	virtual void			setBaudRate(unsigned int) {}
	/*!Return if the parameter is fixed or not.  The general user interface will hide this parameter if it is fixed.*/
	virtual bool			isBaudRateFixed() const { return true; }

	/*!Return the parity.*/
	virtual SerXInterface::Parity	parity() const { return SerXInterface::Parity::B_NOPARITY; }
	/*!Set the parity.*/
	virtual void					setParity(const SerXInterface::Parity& parity) {}
	/*!Return if the parameter is fixed or not.  The general user interface will hide this parameter if it is fixed.*/
	virtual bool					isParityFixed() const { return true; } //Generic serial port ui will hide if fixed

	/*!Return the number of data bits.*/
	virtual int				dataBits() const {return 8;}
	/*!Set the number of data bits.*/
	virtual void			setDataBits(const int& nValue){}
	/*!Return if the parameter is fixed or not.  The general user interface will hide this parameter if it is fixed.*/
	virtual bool			isDataBitsFixed(){return true;}

	/*!Return the number of stop bits.*/
	virtual int				stopBits() const {return 1;}
	/*!Set the number of stop bits.*/
	virtual void			setStopBits(const int& nValue){}
	/*!Return if the parameter is fixed or not.  The general user interface will hide this parameter if it is fixed.*/
	virtual bool			isStopBitsFixed(){return true;}

	/*!Return the flow control. Zero means no flow control.*/
	virtual int				flowControl() const {return 0;}
	/*!Set the flow control.*/
	virtual void			setFlowControl(const int& nValue){}
	/*!Return if the parameter is fixed or not.  The general user interface will hide this parameter if it is fixed.*/
	virtual bool			isFlowControlFixed(){return true;}

	virtual int deviceIdentifier(BasicStringInterface &sIdentifier) {
        sIdentifier = "NiteCrawlerRotatorFocuser";
        return SB_OK;
        }
    
	virtual int isConnectionPossible(const int &nPeerArraySize, MultiConnectionDeviceInterface **ppPeerArray, bool &bConnectionPossible);
	virtual int useResource(MultiConnectionDeviceInterface *pPeer);
	virtual int swapResource(MultiConnectionDeviceInterface *pPeer);
    
    SerXInterface*							m_pSavedSerX;
	MutexInterface*							m_pSavedMutex;



	//@}

private:

	//Standard device driver tools
    const int                               m_nPrivateISIndex;
	SerXInterface*							m_pSerX;		
	TheSkyXFacadeForDriversInterface* 		m_pTheSkyXForMounts;
	SleeperInterface*						m_pSleeper;
	BasicIniUtilInterface*					m_pIniUtil;
	LoggerInterface*						m_pLogger;
	mutable MutexInterface*					m_pIOMutex;
	TickCountInterface*						m_pTickCount;

	SerXInterface 							*GetSerX() {return m_pSerX; }		
	TheSkyXFacadeForDriversInterface		*GetTheSkyXFacadeForDrivers() {return m_pTheSkyXForMounts;}
	SleeperInterface						*GetSleeper() {return m_pSleeper; }
	BasicIniUtilInterface					*GetBasicIniUtil() { return m_pIniUtil; }
	LoggerInterface							*GetLogger() {return m_pLogger; }
	MutexInterface							*GetMutex()  {return m_pIOMutex;}
	TickCountInterface						*GetTickCountInterface() {return m_pTickCount;}
    
    int WriteCommand(const char *szCommand, char *cResponse = nullptr, unsigned long expectedBytes = 0) const;
    
    // Assemble a focuser command, and send to the correct focuser motor
    const char* focusCommand(const char* szCommand) {
            static char cCommand[32];
            int nMotor = (bIsAuxFocuser) ? 3: 1;
            sprintf(cCommand, "%d%s", nMotor, szCommand);
            return cCommand;
            }

    char serialName[MAX_SERIAL_PATH];
    bool    m_bLinked;
    
    bool    bIsAuxFocuser;
    int     nFocuserPositon;
	int		m_nMaxPos;
    
    char cFirmware[32];
    char cFocuserType[32];
    char cSerialNumber[32];
    int  iLastPositon;


	int m_nInstanceIndex;
};


#endif
