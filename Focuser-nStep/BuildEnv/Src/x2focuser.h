// X2Focuser.h : Declaration of the X2Focuser

#ifndef __NSTEP_H_
#define __NSTEP_H_

#include <stdlib.h>
#include "../../../../licensedinterfaces/focuserdriverinterface.h"
#include "../../../../licensedinterfaces/modalsettingsdialoginterface.h"
#include "../../../../licensedinterfaces/x2guiinterface.h"
#include "../../../../licensedinterfaces/focuser/focusertemperatureinterface.h"
#include "SerialDevices.h"

#ifdef SB_WIN_BUILD
#include <Windows.h>
#endif

// Forward declare the interfaces that this device is dependent upon
class SerXInterface;
class TheSkyXFacadeForDriversInterface;
class SleeperInterface;
class SimpleIniUtilInterface;
class LoggerInterface;
class MutexInterface;
class BasicIniUtilInterface;
class TickCountInterface;

/*!
\brief The X2Focuser example.

\ingroup Example

Use this example to write an X2Focuser driver.
*/
class X2Focuser : public FocuserDriverInterface, public ModalSettingsDialogInterface, public X2GUIEventInterface, public FocuserTemperatureInterface
{
public:
	X2Focuser(const char* pszDisplayName, 
												const int& nInstanceIndex,
												SerXInterface						* pSerXIn, 
												TheSkyXFacadeForDriversInterface	* pTheSkyXIn, 
												SleeperInterface					* pSleeperIn,
												BasicIniUtilInterface				* pIniUtilIn,
												LoggerInterface						* pLoggerIn,
												MutexInterface						* pIOMutexIn,
												TickCountInterface					* pTickCountIn);

	~X2Focuser();

public:

	/*!\name DriverRootInterface Implementation
	See DriverRootInterface.*/
	//@{ 
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

	//@}

	bool WriteCommand(const char *szCommand, char *cResponse, unsigned long expectedBytes) const;

private:
	BasicIniUtilInterface							*	GetBasicIniUtil() {return m_pIniUtil; }

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
	BasicIniUtilInterface					*GetSimpleIniUtil() {return m_pIniUtil; }
	LoggerInterface							*GetLogger() {return m_pLogger; }
	MutexInterface							*GetMutex()  {return m_pIOMutex;}
	TickCountInterface						*GetTickCountInterface() {return m_pTickCount;}


	void findDevices(X2GUIExchangeInterface *dx);
    
    double                                  fTempOffset;

    CSerialDevice                           serialDevices;

	char                                    serialName[256];
	bool m_bLinked;
	int m_nPosition;
    double fLastTemp;
    int nStepMode;
    
    int nBackLash;      // May be zero, +/-
    int nOrientation;   // +1 for normal, -1 for reverse
	int nLastMove;		// How much was last move
    
    int nFWWiringPhase;
    int nFWStepRate;
    int nFWMaxStepSpeed;
    bool bFWEnergized;
};


#endif //__X2Focuser_H_
