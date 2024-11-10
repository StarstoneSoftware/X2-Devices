/*  Copyright 2024 Starstone Software Systems, Inc.
    Richard S. Wright Jr.
    rwright@starstonesoftware.com
    
    Permission is hereby granted, free of charge, to any person obtaining a 
    copy of this software and associated documentation files (the “Software”), 
    to deal in the Software without restriction, including without limitation 
    the rights to use, copy, modify, merge, publish, distribute, sublicense, 
    and/or sell copies of the Software, and to permit persons to whom the 
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included 
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS 
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <atomic>

#ifdef _WIN32
#include <windows.h>
#else
#include "sys/time.h"
#define wchar_t char
#endif

#include "../../../licensedinterfaces/sberrorx.h"
#include "../../../licensedinterfaces/cameradriverinterface.h"
#include "../../../licensedinterfaces/modalsettingsdialoginterface.h"
#include "../../../licensedinterfaces/noshutterinterface.h"
#include "../../../licensedinterfaces/x2guiinterface.h"
#include "../../../licensedinterfaces/subframeinterface.h"
#include "../../../licensedinterfaces/pixelsizeinterface.h"
#include "../../../licensedinterfaces/addfitskeyinterface.h"
#include "../../../licensedinterfaces/cameradependentsettinginterface.h"
#include "../../../licensedinterfaces/loggerinterface.h"
#include "../../../licensedinterfaces/addfitskeyinterface.h"

#include "toupcam.h"


class SerXInterface;		
class TheSkyXFacadeForDriversInterface;
class SleeperInterface;
class BasicIniUtilInterface;
class LoggerInterface;
class MutexInterface;
class TickCountInterface;
class BasicStringInterface;


/*!
\brief The X2Camera example.

\ingroup Example

Use this example to write an X2Camera driver.
*/
class X2Camera: public CameraDriverInterface, public ModalSettingsDialogInterface, public X2GUIEventInterface,  public SubframeInterface, public NoShutterInterface, public PixelSizeInterface, public AddFITSKeyInterface, public CameraDependentSettingInterface, public LoggerInterface
{
public: 
	/*!Standard X2 constructor*/
	X2Camera(const char* pszSelectionString, 
					const int& nISIndex,
					SerXInterface*						pSerX,
					TheSkyXFacadeForDriversInterface* pTheSkyXForMounts,
					SleeperInterface*					pSleeper,
					BasicIniUtilInterface*				pIniUtil,
					LoggerInterface*					pLogger,
					MutexInterface*					pIOMutex,
					TickCountInterface*				pTickCount);
	virtual ~X2Camera();  

    
    enumCameraIndex m_Camera;
	/*!\name DriverRootInterface Implementation
	See DriverRootInterface.*/
	//@{ 
	virtual int									queryAbstraction(const char* pszName, void** ppVal)			;
	//@} 

	/*!\name DriverInfoInterface Implementation
	See DriverInfoInterface.*/
	//@{ 
	virtual void								driverInfoDetailedInfo(BasicStringInterface& str) const		;
	virtual double								driverInfoVersion(void) const								;
	//@} 

	/*!\name HardwareInfoInterface Implementation
	See HardwareInfoInterface.*/
	//@{ 
	virtual void deviceInfoNameShort(BasicStringInterface& str) const										;
	virtual void deviceInfoNameLong(BasicStringInterface& str) const										;
	virtual void deviceInfoDetailedDescription(BasicStringInterface& str) const								;
	virtual void deviceInfoFirmwareVersion(BasicStringInterface& str)										;
	virtual void deviceInfoModel(BasicStringInterface& str)													;
	//@} 

  // Logger Interface
	/*! Have a string logged in TheSkyX's Communication Log window.*/
	virtual int out(const char* szLogThis);

	// Helper for above...
	void sendToLog(const char *szFormat, ...);

	/*! Return the number of packets, retries and failures associated with device io if appropriate.*/
	virtual void packetsRetriesFailuresChanged(const int& p, const int& r, const int& f) {}

public://Properties

	/*!\name CameraDriverInterface Implementation
	See CameraDriverInterface.*/
	//@{ 

	virtual enumCameraIndex	cameraId();
	virtual	void		setCameraId(enumCameraIndex Cam);
	virtual bool		isLinked()					{return m_bLinked;}
	virtual void		setLinked(const bool bYes)	{m_bLinked = bYes;}
	
	virtual int			GetVersion(void)			{return CAMAPIVERSION;}
	virtual CameraDriverInterface::ReadOutMode readoutMode(void);
	virtual int			pathTo_rm_FitsOnDisk(char* lpszPath, const int& nPathSize);
    
    inline void UpPriority(void);
    inline void DownPriority(void);

public://Methods

	virtual int CCSettings(const enumCameraIndex& Camera, const enumWhichCCD& CCD);

	virtual int CCEstablishLink(enumLPTPort portLPT, const enumWhichCCD& CCD, enumCameraIndex DesiredCamera, enumCameraIndex& CameraFound, const int nDesiredCFW, int& nFoundCFW);
	virtual int CCDisconnect(const bool bShutDownTemp);

	virtual int CCGetChipSize(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nXBin, const int& nYBin, const bool& bOffChipBinning, int& nW, int& nH, int& nReadOut);
	virtual int CCGetNumBins(const enumCameraIndex& Camera, const enumWhichCCD& CCD, int& nNumBins);
	virtual	int CCGetBinSizeFromIndex(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nIndex, long& nBincx, long& nBincy);

	virtual int CCSetBinnedSubFrame(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nLeft, const int& nTop, const int& nRight, const int& nBottom);

	//SubframeInterface
	virtual int CCSetBinnedSubFrame3(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nLeft, const int& nTop, const int& nWidth, const int& nHeight);

	virtual void CCMakeExposureState(int* pnState, enumCameraIndex Cam, int nXBin, int nYBin, int abg, bool bRapidReadout);//SBIG specific

	virtual int CCStartExposure(const enumCameraIndex& Cam, const enumWhichCCD CCD, const double& dTime, enumPictureType Type, const int& nABGState, const bool& bLeaveShutterAlone);
	virtual int CCIsExposureComplete(const enumCameraIndex& Cam, const enumWhichCCD CCD, bool* pbComplete, unsigned int* pStatus);
	virtual int CCEndExposure(const enumCameraIndex& Cam, const enumWhichCCD CCD, const bool& bWasAborted, const bool& bLeaveShutterAlone);

	virtual int CCReadoutLine(const enumCameraIndex& Cam, const enumWhichCCD& CCD, const int& pixelStart, const int& pixelLength, const int& nReadoutMode, unsigned char* pMem);
	virtual int CCDumpLines(const enumCameraIndex& Cam, const enumWhichCCD& CCD, const int& nReadoutMode, const unsigned int& lines);

	virtual int CCReadoutImage(const enumCameraIndex& Cam, const enumWhichCCD& CCD, const int& nWidth, const int& nHeight, const int& nMemWidth, unsigned char* pMem);

	virtual int CCRegulateTemp(const bool& bOn, const double& dTemp);
	virtual int CCQueryTemperature(double& dCurTemp, double& dCurPower, char* lpszPower, const int nMaxLen, bool& bCurEnabled, double& dCurSetPoint);
	virtual int	CCGetRecommendedSetpoint(double& dRecSP);
	virtual int	CCSetFan(const bool& bOn);

	virtual int CCActivateRelays(const int& nXPlus, const int& nXMinus, const int& nYPlus, const int& nYMinus, const bool& bSynchronous, const bool& bAbort, const bool& bEndThread);

	virtual int CCPulseOut(unsigned int nPulse, bool bAdjust, const enumCameraIndex& Cam);

	virtual int CCSetShutter(bool bOpen);
	virtual int CCUpdateClock(void);

	virtual int CCSetImageProps(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nReadOut, void* pImage);	
	virtual int CCGetFullDynamicRange(const enumCameraIndex& Camera, const enumWhichCCD& CCD, unsigned long& dwDynRg);
	
	virtual void CCBeforeDownload(const enumCameraIndex& Cam, const enumWhichCCD& CCD);
	virtual void CCAfterDownload(const enumCameraIndex& Cam, const enumWhichCCD& CCD);

	virtual int PixelSize1x1InMicrons(const enumCameraIndex &Camera, const enumWhichCCD &CCD, double &x, double &y);
    virtual int CCHasShutter(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, bool &bHasShutter) { bHasShutter = bCamHasShutter; return SB_OK;  }

    virtual int countOfIntegerFields(int &nCount);
	virtual int valueForIntegerField(int nIndex, BasicStringInterface& sFieldName, BasicStringInterface& sFieldComment, int &nFieldValue);

	virtual int countOfDoubleFields (int &nCount);
	virtual int valueForDoubleField (int nIndex, BasicStringInterface& sFieldName, BasicStringInterface& sFieldComment, double &dFieldValue);

	virtual int countOfStringFields (int &nCount);
	virtual int valueForStringField (int nIndex, BasicStringInterface& sFieldName, BasicStringInterface& sFieldComment, BasicStringInterface &sFieldValue);


	//@} 

    int CCGetExtendedSettingName(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, BasicStringInterface &sSettingName);
    int CCGetExtendedValueCount(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, int &nCount);
    int CCGetExtendedValueName(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, const int nIndex, BasicStringInterface &sName);
    int CCStartExposureAdditionalArgInterface(const enumCameraIndex& Cam, const enumWhichCCD CCD, const double& dTime, enumPictureType Type,
                                const int& nABGState, const bool& bLeaveShutterAlone, const int &nIndex);


	//
	/*!\name ModalSettingsDialogInterface Implementation
	See ModalSettingsDialogInterface.*/
	//@{ 
	virtual int								initModalSettingsDialog(void){return 0;}
	virtual int								execModalSettingsDialog(void);
	//@} 
	
	//
	/*!\name X2GUIEventInterface Implementation
	See X2GUIEventInterface.*/
	//@{ 
	virtual void uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent);
	//@} 

	//Implemenation below here
private:
	SerXInterface 									*	GetSerX() {return m_pSerX; }		
	TheSkyXFacadeForDriversInterface				*	GetTheSkyXFacadeForDrivers() {return m_pTheSkyXForMounts;}
	SleeperInterface								*	GetSleeper() {return m_pSleeper; }
	BasicIniUtilInterface							*	GetBasicIniUtil() {return m_pIniUtil; }
	LoggerInterface									*	GetLogger() {return m_pLogger; }
public:
	MutexInterface									*	GetMutex() const  {return m_pIOMutex;}
private:
	TickCountInterface								*	GetTickCountInterface() {return m_pTickCount;}

	SerXInterface									*	m_pSerX;		
	TheSkyXFacadeForDriversInterface				*	m_pTheSkyXForMounts;
	SleeperInterface								*	m_pSleeper;
	BasicIniUtilInterface							*	m_pIniUtil;
	LoggerInterface									*	m_pLogger;
	MutexInterface									*	m_pIOMutex;
	TickCountInterface								*	m_pTickCount;

	int m_nPrivateISIndex;

	double exposureTime;
	int m_CachedBinX;
	int m_CachedBinY;
	int m_CachedWidth;
	int m_CachedHeight;
	int m_CachedCam;
	double m_dCurSetTemp;
	double m_dCurPower;
    bool bColor;
    char cDeBayer[5];
    bool bHueristicHotPixel;
    
    const char *szSelectedCamera;
        
    bool     bCamHasShutter;
    bool     bCamHasTempControl;
    bool     bCamCanReadTemp;
    bool     bCachedTECEnabled;
    
    void     SaveAllOptions(void);
    void     LoadAllOptions(void);
    
	int		nSubLeft, nSubTop, nSubWidth, nSubHeight;
 
 
    static void __stdcall DataCallback(const void* pData, const ToupcamFrameInfoV2* pInfo, int bSnap, void* pCallbackCtx);
    static void __stdcall EventCallback(unsigned nEvent, void* pCallbackCtx);

 
    // ToupTek spcifics
public:
    HToupcam				hCamera;					// Handle to the camera
	ToupcamDeviceV2			deviceList[TOUPCAM_MAX];	// List of connected cameras
	ToupcamModelV2			cameraInfo;					// Details about the camera
    int                     streamingMode;              // Pull or Push?
    unsigned short*         imgData;                    // Image Data
    std::atomic<bool>       bImgReady;                  // Is the image ready?
    int                     bitDepth;                   // Bit depth for the camera
    unsigned short          minGain;
    unsigned short          maxGain;                    // Gain settngs
    unsigned short          defaultGain;
    unsigned short          currentGain;
    int                     nLastStreamingMode;

	unsigned int nFourCC, bitsPerPixel;

	// The currently selected camera
	char					selectedCamera[64];


	const char*	getCString(const wchar_t* convertMe) const {
 #ifdef _WIN32   
		static char cOut[256];
		wcstombs(cOut, convertMe, 256);
		return cOut;
#else
        return convertMe;
#endif
	}    
};



