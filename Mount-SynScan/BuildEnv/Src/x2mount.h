#pragma once
#include "../../licensedinterfaces/mountdriverinterface.h"
#include "../../licensedinterfaces/serialportparams2interface.h"

//Optional interfaces, uncomment and implement as required.
#include "../../licensedinterfaces/mount/slewtointerface.h"
#include "../../licensedinterfaces/mount/syncmountinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"

/// TBD
#include "../../licensedinterfaces/mount/asymmetricalequatorialinterface2.h"
#include "../../licensedinterfaces/mount/openloopmoveinterface.h"
#include "../../licensedinterfaces/mount/needsrefractioninterface.h"
#include "../../licensedinterfaces/mount/linkfromuithreadinterface.h"
#include "../../licensedinterfaces/mount/trackingratesinterface.h"
#include "../../licensedinterfaces/parkinterface.h"
#include "../../licensedinterfaces/mount/beyondthepoleinterface.h"


// Forward declare the interfaces that the this driver is "given" by TheSkyX
class SerXInterface;
class TheSkyXFacadeForDriversInterface;
class SleeperInterface;
class BasicIniUtilInterface;
class LoggerInterface;
class MutexInterface;
class AbstractSyncMount;
class TickCountInterface;

#define PARENT_KEY              "SkyWatcherSyncScanMount"
#define CHILD_KEY_PORT_NAME     "PortName"
#define MAX_PORT_NAME_SIZE      255

#define TRACKING_MODE_OFF       0
#define TRACKING_MODE_ALT_AZ    1
#define TRACKING_MODE_EQ        2
#define TRACKING_MODE_PEC       3



class X2Mount : public MountDriverInterface, public SerialPortParams2Interface, public SyncMountInterface, public SlewToInterface, public LoggerInterface, public AsymmetricalEquatorialInterface2, /*ParkInterface, */BeyondThePoleInterface

						//Optional interfaces, uncomment and implement as required.
						//,public OpenLoopMoveInterface,
						//,public LinkFromUIThreadInterface,
						//,public TrackingRatesInterface 
{
public:
	/*!Standard X2 constructor*/
	X2Mount(const char* pszDriverSelection,
				const int& nInstanceIndex,
				SerXInterface					* pSerX, 
				TheSkyXFacadeForDriversInterface	* pTheSkyX, 
				SleeperInterface					* pSleeper,
				BasicIniUtilInterface			* pIniUtil,
				LoggerInterface					* pLogger,
				MutexInterface					* pIOMutex,
				TickCountInterface				* pTickCount);

	~X2Mount();

// Operations
public:

	/*!\name DriverRootInterface Implementation
	See DriverRootInterface.*/
	//@{ 
	virtual DeviceType							deviceType(void)							  {return DriverRootInterface::DT_MOUNT;}
	virtual int									queryAbstraction(const char* pszName, void** ppVal) ;
	//@} 

	/*!\name LinkInterface Implementation
	See LinkInterface.*/
	//@{ 
	virtual int									establishLink(void)						;
	virtual int									terminateLink(void)						;
	virtual bool								isLinked(void) const					;
	virtual bool								isEstablishLinkAbortable(void) const	;
	//@} 

	/*!\name DriverInfoInterface Implementation
	See DriverInfoInterface.*/
	//@{ 
	virtual void								driverInfoDetailedInfo(BasicStringInterface& str) const;
	virtual double								driverInfoVersion(void) const				;
	//@} 

	/*!\name HardwareInfoInterface Implementation
	See HardwareInfoInterface.*/
	//@{ 
	virtual void deviceInfoNameShort(BasicStringInterface& str) const				;
	virtual void deviceInfoNameLong(BasicStringInterface& str) const				;
	virtual void deviceInfoDetailedDescription(BasicStringInterface& str) const	;
	virtual void deviceInfoFirmwareVersion(BasicStringInterface& str)				;
	virtual void deviceInfoModel(BasicStringInterface& str)						;
	//@} 

	virtual int									raDec(double& ra, double& dec, const bool& bCached = false)					;
	virtual int									abort(void)																	;

    //SerialPortParams2Interface
    virtual void            portName(BasicStringInterface& str) const;
    virtual void            setPortName(const char* szPort);
    virtual unsigned int    baudRate() const                {return 9600;};
    virtual void            setBaudRate(unsigned int)       {};
    virtual bool            isBaudRateFixed() const        {return true;}

    virtual SerXInterface::Parity   parity() const                {return SerXInterface::B_NOPARITY;}
    virtual void                    setParity(const SerXInterface::Parity& parity){(void)parity;};
    virtual bool                    isParityFixed() const        {return true;}

	//Optional interfaces, uncomment and implement as required.

	//SyncMountInterface
	virtual int syncMount(const double& ra, const double& dec)									;
	virtual bool isSynced()																		;

	//SlewToInterface
	virtual int								startSlewTo(const double& dRa, const double& dDec)	;
	virtual int								isCompleteSlewTo(bool& bComplete) const				;
	virtual int								endSlewTo(void)										;
 
    virtual int                             startSlewToAltAz(const double& dAlt, const double& dAz);
	
    // LoggerInterface
    virtual int out(const char* szLogThis);
    virtual void packetsRetriesFailuresChanged(const int& p, const int& r, const int& f);

    
	//AsymmetricalEquatorialInterface
	virtual bool knowsBeyondThePole(); 
	virtual int beyondThePole(bool& bYes);
//	virtual double flipHourAngle();
//	virtual int gemLimits(double& dHoursEast, double& dHoursWest);

    MountTypeInterface::Type mountType(){return MountTypeInterface::Asymmetrical_Equatorial;}

	//OpenLoopMoveInterface
	//virtual int								startOpenLoopMove(const MountDriverInterface::MoveDir& Dir, const int& nRateIndex);
	//virtual int								endOpenLoopMove(void)															;
	//virtual bool								allowDiagonalMoves()															;
	//virtual int								rateCountOpenLoopMove(void) const												;
	//virtual int								rateNameFromIndexOpenLoopMove(const int& nZeroBasedIndex, char* pszOut, const int& nOutMaxSize);
	//virtual int								rateIndexOpenLoopMove(void);

	//NeedsRefractionInterface
    // SyncScan compensates for refraction
	virtual bool							needsRefactionAdjustments(void) { return false; }

	//LinkFromUIThreadInterface

	//TrackingRatesInterface 
	//virtual int setTrackingRates( const bool& bTrackingOn, const bool& bIgnoreRates, const double& dRaRateArcSecPerSec, const double& dDecRateArcSecPerSec);
	//virtual int trackingRates( bool& bTrackingOn, double& dRaRateArcSecPerSec, double& dDecRateArcSecPerSec);

    /*!Return true if the device is parked.*/
//    virtual bool                            isParked(void);
//    
//    /*!Initiate the park process.*/
//    virtual int                                startPark(const double& dAz, const double& dAlt);
//    
//    /*!Called to monitor the park process.  \param bComplete Set to true if the park is complete, otherwise set to false.*/
//    virtual int                                isCompletePark(bool& bComplete) const;
//    
//    /*!Called once the park is complete.  This is called once for every corresponding startPark() allowing software implementations of park.*/
//    virtual int                                endPark(void);




// Implementation
private:	

	SerXInterface 							*GetSerX() {return m_pSerX; }		
	TheSkyXFacadeForDriversInterface		*GetTheSkyXFacadeForMounts() {return m_pTheSkyXForMounts;}
	SleeperInterface						*GetSleeper() {return m_pSleeper; }
	BasicIniUtilInterface					*GetSimpleIniUtil() {return m_pIniUtil; }
	LoggerInterface							*GetLogger() {return m_pLogger; }
	MutexInterface							*GetMutex() const {return m_pIOMutex;}
	TickCountInterface						*GetTickCountInterface() {return m_pTickCount;}
	

	int m_nPrivateMulitInstanceIndex;
	SerXInterface*							m_pSerX;		
	TheSkyXFacadeForDriversInterface* 		m_pTheSkyXForMounts;
	SleeperInterface*						m_pSleeper;
	BasicIniUtilInterface*					m_pIniUtil;
	LoggerInterface*							m_pLogger;
	MutexInterface*							m_pIOMutex;
	TickCountInterface*						m_pTickCount;
 
    // Stuff...
    char        m_PortName[MAX_PORT_NAME_SIZE];
    char        m_Firmware[32];
    char        szLogString[128];
    
    char        modeTracking;
    bool        bEQMount;
    bool        bIsSynced;
    bool        bSouthPole;
    
    int SendCommand(const char *szCommandString, char *szResponsString, int nRespLength);
   



};
