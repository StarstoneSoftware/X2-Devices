// x2camera.cpp  
//

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#include <pthread.h>
#define _strdup     strdup
#else


#endif

#ifdef SB_LINUX_BUILD
#include <pthread.h>
#include <unistd.h>
#define _strdup     strdup
#endif

#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <cstring>
#include <stdlib.h>
#include "x2camera.h"
#include "StopWatch.h"

#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/theskyxfacadefordriversinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/basiciniutilinterface.h"
#include "../../licensedinterfaces/mutexinterface.h"
#include "../../licensedinterfaces/tickcountinterface.h"
#include "../../licensedinterfaces/basicstringinterface.h"
#include "../../licensedinterfaces/x2guiinterface.h"

//As far as a naming convention goes, X2 implementors could do a search and 
//replace in all files and change "X2Camera" to "CoolCompanyCamera" 
//where CoolCompany is your company name.  This is not a requirement.

//For properties that need to be persistent
#ifdef MEADE_CAMERA
#define KEY_X2CAM_ROOT              "Meade_DSI-IV"
#define TOUP_DEVICE_NAME            "Meade Deep Sky Imager"
#define TOUP_GUI                    "MeadeDS4.ui"
#else
#define KEY_X2CAM_ROOT				"Mallincam_SKYRAIDER"
#define TOUP_DEVICE_NAME            "Mallincam SkyRaider"
#define TOUP_GUI                    "MallincamSR.ui"
#endif

#define KEY_X2CAM_DEVICE			"DEVICE_NAME"
#define KEY_X2CAM_GAIN              "DEVICE_GAIN"

// PUSH is single exposure, pull is a loop pulling data as quickly as possible
#define MC_INVALID_MODE         -1
#define MC_PUSH_MODE            0
#define MC_PULL_MODE            1

#define DRIVER_VERSION          1.03

// 0.3 Takes images with subframes and binning.... most of the time ;-)
// 0.5 It works...
// 0.7 Some doubt about subframes, but otherwise seems good. Need an optic now.
// 0.8 Added gain to fits interface
// 0.82 SDK Update - DS16c TEC and later cameras now work, so added to the list
// 1.0 Fixed issue where cooler would disengage between exposures
//     Removed streaming mode, as taking .SER is not fast anyway, and it's complicating things.
//     Internally, using trigger instead of snap!!!! Do NOT EVER GO BACK TO SNAP
// 1.01 Weird stuff going on under macOS and 16c camera
// 1.02 Updated SDK
// 1.03 Updated SDK only
///////////////////////////////////////////////////////////////////////////////////////////////////////
// Extract a subframe from a larger image.
void sbImgExtractSubFrame(unsigned short* pDst, const unsigned short* pSrc, const int nSrcWidth,
						const int nSubX, const int nSubY, const int nSubWidth, const int nSubHeight)
	{
    unsigned int nDstIndex = 0;

    // Copy a row at a time, faster
    for(int nRow = nSubY; nRow < (nSubHeight + nSubY); nRow++) {
        int nSrcIndex = nRow * nSrcWidth;
        memcpy(pDst + nDstIndex, pSrc + nSrcIndex + nSubX, sizeof(unsigned short) * nSubWidth);
        nDstIndex += nSubWidth;
        }
	}




X2Camera::X2Camera( const char* pszSelection,
					const int& nISIndex,
					SerXInterface*						pSerX,
					TheSkyXFacadeForDriversInterface*	pTheSkyXForMounts,
					SleeperInterface*					pSleeper,
					BasicIniUtilInterface*				pIniUtil,
					LoggerInterface*					pLogger,
					MutexInterface*						pIOMutex,
					TickCountInterface*					pTickCount)
    {
	m_nPrivateISIndex				= nISIndex;
	m_pSerX							= pSerX;
	m_pTheSkyXForMounts				= pTheSkyXForMounts;
	m_pSleeper						= pSleeper;
	m_pIniUtil						= pIniUtil;
	m_pLogger						= pLogger;	
	m_pIOMutex						= pIOMutex;
	m_pTickCount					= pTickCount;

	setCameraId(CI_PLUGIN);

	m_dCurSetTemp = -5;
	m_dCurPower = 100.0;
	m_CachedBinX = 1;
	m_CachedBinY = 1; 
	m_CachedCam = 0;
    bImgReady = false;

    bCamHasShutter = false;
    bCamHasTempControl = false;
    
    szSelectedCamera = _strdup(pszSelection);
    imgData = nullptr;
    
    bColor = false;
    }

X2Camera::~X2Camera()
{
    if(m_bLinked) {
        Mallincam_Stop(hCamera);
        Mallincam_Close(hCamera);
        }
        
	//Delete objects used through composition
	if (GetSerX())
		delete GetSerX();
	if (GetTheSkyXFacadeForDrivers())
		delete GetTheSkyXFacadeForDrivers();
	if (GetSleeper())
		delete GetSleeper();
	if (GetBasicIniUtil())
		delete GetBasicIniUtil();
	if (GetLogger())
		delete GetLogger();
	if (GetTickCountInterface())
		delete GetTickCountInterface();
    
    free((void *)szSelectedCamera);

  if (GetMutex()) {
      delete GetMutex();
      m_pIOMutex = nullptr;
        }
        
    delete [] imgData;
}

//DriverRootInterface
int	X2Camera::queryAbstraction(const char* pszName, void** ppVal)			
    {
	X2MutexLocker ml(GetMutex());

	if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
		*ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);
	else
		if (!strcmp(pszName, X2GUIEventInterface_Name))
			*ppVal = dynamic_cast<X2GUIEventInterface*>(this);
        else
        if (!strcmp(pszName, SubframeInterface_Name))
            *ppVal = dynamic_cast<SubframeInterface*>(this);
        else if(!strcmp(pszName, NoShutterInterface_Name))
		    *ppVal = dynamic_cast<NoShutterInterface*>(this);
	    else if (!strcmp(pszName, PixelSizeInterface_Name))
			*ppVal = dynamic_cast<PixelSizeInterface*>(this);
        else if (!strcmp(pszName, AddFITSKeyInterface_Name)) 
            *ppVal = dynamic_cast<AddFITSKeyInterface*>(this);
		else if (!strcmp(pszName, LoggerInterface_Name))
			*ppVal = dynamic_cast<LoggerInterface*>(GetLogger());

	return SB_OK;
    }

/////////////////////////////////////////////////////////////////////
// Logging output
int X2Camera::out(const char* szLogThis)
{
	if (GetLogger())
		return GetLogger()->out(szLogThis);
	else
		return ERR_POINTER;
}

////////////////////////////////////////////////////////////////////
// A helper, nicer way to send output to the log. Less clutter
void X2Camera::sendToLog(const char *szFormat, ...)
{
	char outputString[512];

	va_list argptr;
	va_start(argptr, szFormat);
	vsnprintf(outputString, 512, szFormat, argptr);
	va_end(argptr);


	// Send to TheSkyX's log
	out(outputString);
}


// Because I only use this once, and loops make my heart hurt....
inline void RoundDouble2(double &value)
    {
    char cBuff[32];
    sprintf(cBuff, "%.2f", value);
    value = atof(cBuff);
    }


int X2Camera::PixelSize1x1InMicrons(const enumCameraIndex &Camera, const enumWhichCCD &CCD, double &x, double &y)
	{
    if(!m_bLinked) {
        x = 0.0;
        y = 0.0;
        return ERR_COMMNOLINK;
        }
        
	x = cameraInfo.xpixsz;
	y = cameraInfo.ypixsz;

	RoundDouble2(x);
	RoundDouble2(y);

	return SB_OK;
	}


//DriverInfoInterface
void X2Camera::driverInfoDetailedInfo(BasicStringInterface& str) const		
    {
	X2MutexLocker ml(GetMutex());

    if(!m_bLinked) {
        str = "Camera Not Connected";
        return;
        }
        
    str = getCString(Mallincam_Version());
    }
    
double X2Camera::driverInfoVersion(void) const								
    {
	X2MutexLocker ml(GetMutex());

	return DRIVER_VERSION;
    }

//HardwareInfoInterface
void X2Camera::deviceInfoNameShort(BasicStringInterface& str) const										
{
	X2MutexLocker ml(GetMutex());

    if(!m_bLinked) {
        str = "Camera Not Connected";
        return;
        }

	str = TOUP_DEVICE_NAME;
}
void X2Camera::deviceInfoNameLong(BasicStringInterface& str) const										
{
	X2MutexLocker ml(GetMutex());

    if(!m_bLinked) {
        str = "Camera Not Connected";
        return;
        }

    str = TOUP_DEVICE_NAME;
}
void X2Camera::deviceInfoDetailedDescription(BasicStringInterface& str) const								
{
	X2MutexLocker ml(GetMutex());

    if(!m_bLinked) {
        str = "Camera Not Connected";
        return;
        }
        
	str = "High Speed Camera"; 
}
void X2Camera::deviceInfoFirmwareVersion(BasicStringInterface& str)										
    {
	X2MutexLocker ml(GetMutex());

    if(!m_bLinked) {
        str = "Camera Not Connected";
        return;
        }
        
    char fwVer[16];
    Mallincam_get_FwVersion(hCamera, fwVer);
    str = fwVer;
    }



int X2Camera::countOfIntegerFields(int &nCount)
    {
    nCount = 1;
    return SB_OK;
    }

int X2Camera::valueForIntegerField(int nIndex, BasicStringInterface& sFieldName, BasicStringInterface& sFieldComment, int &nFieldValue)
    {
    sFieldName = "Gain";
    sFieldComment = "Mallincam Gain Setting";
    nFieldValue = currentGain;
    return SB_OK;
    }

int X2Camera::countOfDoubleFields (int &nCount)
    {
    nCount = 0;
    return SB_OK;
    }

int X2Camera::valueForDoubleField (int nIndex, BasicStringInterface& sFieldName, BasicStringInterface& sFieldComment, double &dFieldValue)
    {
    return SB_OK;
    }


int X2Camera::countOfStringFields (int &nCount)
    {
    if(bColor)
        nCount = 1;
    else
        nCount = 0;
    
    return SB_OK;
    }

int X2Camera::valueForStringField (int nIndex, BasicStringInterface& sFieldName, BasicStringInterface& sFieldComment, BasicStringInterface &sFieldValue)
    {
	sFieldName = "DEBAYER";
	sFieldComment = "Debayer Pattern";

    // All Mallincams support color when binned
	if (bColor)// && m_CachedBinX == 1)
		sFieldValue = cDeBayer;
	else
		sFieldValue = "MONO";

	return SB_OK;
    }


void X2Camera::deviceInfoModel(BasicStringInterface& str)													
{
	X2MutexLocker ml(GetMutex());

    if(!m_bLinked) {
        str = "No Camera Connected";
        return;
        }
        
    char cSerial[32];
    Mallincam_get_SerialNumber(hCamera, cSerial);
    str = cSerial;
    }

// Reverse the fourCC code
char *GetFourCC(unsigned int nFourCC)
	{
	static char cBayer[5];
	cBayer[4] = NULL;
	cBayer[0] = nFourCC & 0x000000ff;
	cBayer[1] = (nFourCC & 0x0000ff00) >> 8;
	cBayer[2] = (nFourCC & 0x00ff0000) >> 16;
	cBayer[3] = (nFourCC & 0xff000000) >> 24;
	return cBayer;
	}

int X2Camera::CCEstablishLink(const enumLPTPort portLPT, const enumWhichCCD& CCD, enumCameraIndex DesiredCamera, enumCameraIndex& CameraFound, const int nDesiredCFW, int& nFoundCFW)
    {
    HRESULT hr;
    X2MutexLocker ml(GetMutex());

	// On construction, get data from ini
    LoadAllOptions();
    bCamHasShutter = false;
    bCamHasTempControl = false;
    bCamCanReadTemp = false;
    bCachedTECEnabled = false;
    bColor = false;
    nLastStreamingMode = MC_PUSH_MODE;
    streamingMode = MC_PUSH_MODE;


    // Attempt to open a specific camera model
    m_bLinked = false;

	// Are there any cameras connected?
	unsigned cnt = Mallincam_EnumV2(deviceList);
	if (cnt == 0) {
		sendToLog("No cameras connected");
		return ERR_NOLINK;
		}

	// Have we selected a device?
	if (selectedCamera[0] == 0) 
		return ERR_NODEVICESELECTED;

	sendToLog("Trying to open ->%s<-", selectedCamera);

	// Look for the camera
	hCamera = NULL;
	unsigned int i = 0;
	for(i = 0; i < cnt; i++)  {
        sendToLog("Looking at %s",getCString(deviceList[i].displayname));
		if (strncmp(selectedCamera, getCString(deviceList[i].displayname), 64) == 0) {
			hCamera = Mallincam_OpenByIndex(i);
			break;
			}
        }

	if(hCamera == NULL) {
        sendToLog("Could not find %s", selectedCamera);
        return ERR_NOLINK;
        }

	memcpy(&cameraInfo, deviceList[i].model, sizeof(MallincamModelV2));

	sendToLog("Camera size is %d x %d", cameraInfo.res[0].width, cameraInfo.res[0].height);
 
    if(imgData) delete [] imgData;
        imgData = new unsigned short[cameraInfo.res[0].width * cameraInfo.res[0].height];
        
    if(imgData == nullptr) {
        Mallincam_Close(hCamera);
        hCamera = NULL;
        return ERR_MEMORY;
        }

    // Make sure camera is not running
    Mallincam_Stop(hCamera);

 
    // We always want RAW mode
    if(0 != Mallincam_put_Option(hCamera, MALLINCAM_OPTION_RAW, 1)) {
        Mallincam_Close(hCamera);
        hCamera = NULL;
        return ERR_CMDFAILED;
        }
 
    // We expect 16-bit values for ADU 
    Mallincam_put_Option(hCamera, MALLINCAM_OPTION_BITDEPTH, 1);
    Mallincam_put_AutoExpoEnable(hCamera, 0);

    // Set the gain
    Mallincam_put_ExpoAGain(hCamera, currentGain);
	Mallincam_put_Option(hCamera, MALLINCAM_OPTION_TESTPATTERN, 0);	// no test pattern
    
                        
    // Get color information? Assume RGGB for now if color
	bColor = ((Mallincam_get_MonoMode(hCamera) == 1));
    sendToLog("Color flag is %d", bColor);
	if (bColor) {
		Mallincam_put_Option(hCamera, MALLINCAM_OPTION_COLORMATIX, 1);
		Mallincam_put_Option(hCamera, MALLINCAM_OPTION_DEMOSAIC, 1);	// VNG interpolation

		Mallincam_get_RawFormat(hCamera, &nFourCC, &bitsPerPixel);
		sendToLog("Color format: %d (%s), bits per pixel %d", nFourCC, GetFourCC(nFourCC), bitsPerPixel);
		strcpy(cDeBayer, GetFourCC(nFourCC));
		}
    else
        strcpy(cDeBayer, "MONO");
        
    // Get the bit depth, we will need this
    bitDepth = Mallincam_get_MaxBitDepth(hCamera);
    sendToLog("Bit depth = %d", bitDepth);
    
    sendToLog("Camera Flags = %0x", cameraInfo.flag);
    bCamCanReadTemp = cameraInfo.flag & MALLINCAM_FLAG_GETTEMPERATURE;
    bCamHasTempControl = cameraInfo.flag & MALLINCAM_FLAG_TEC_ONOFF;//TOUPCAM_FLAG_PUTTEMPERATURE;
    sendToLog("Temp flags %d, %d", bCamCanReadTemp, bCamHasTempControl);
    
    // Range of valid gains for this camera
    Mallincam_get_ExpoAGainRange(hCamera, &minGain, &maxGain, &defaultGain);
    sendToLog("Min gain: %d, Max gain: %d, def gain: %d", minGain, maxGain, defaultGain);
    if(currentGain < minGain || currentGain > maxGain)
        currentGain = defaultGain;
        
    // Check trigger modes
    if(cameraInfo.flag & MALLINCAM_FLAG_TRIGGER_SOFTWARE)
        sendToLog("MALLINCAM_FLAG_TRIGGER_SOFTWARE");
    
    if(cameraInfo.flag & MALLINCAM_FLAG_TRIGGER_SINGLE)
        sendToLog("MALLINCAM_FLAG_TRIGGER_SINGLE");
    
    if(cameraInfo.flag & MALLINCAM_FLAG_TRIGGER_EXTERNAL)
        sendToLog("MALLINCAM_FLAG_TRIGGER_EXTERNAL");
    
    
#ifndef SB_WIN_BUILD
    Mallincam_put_Option(hCamera, MALLINCAM_OPTION_UPSIDE_DOWN, 1); // FLIP ON non-Windows
#endif
    
	// Start in push mode by default
    Mallincam_put_Option(hCamera, MALLINCAM_OPTION_TRIGGER, 1);
    Mallincam_put_eSize(hCamera, 0);
	hr = Mallincam_StartPushModeV3(hCamera, DataCallback, (void *)this, EventCallback, nullptr);
    if(hr != 0)
        sendToLog("Mallincam_StartPushModeV3 error: %x", hr);

    
	int nMaxWidth, nMaxHeight;
	Mallincam_get_Resolution(hCamera, 0, &nMaxWidth, &nMaxHeight);
	sendToLog("Max width/height is %d x %d", nMaxWidth, nMaxHeight);

    bImgReady = false;
	m_bLinked = true;
    return SB_OK;
	}


int X2Camera::CCDisconnect(const bool bShutDownTemp)
    {
	X2MutexLocker ml(GetMutex());
    
	if (m_bLinked)
        {
    	setLinked(false);
        if(hCamera != NULL) {
            Mallincam_Stop(hCamera);
		    Mallincam_Close(hCamera);
            }
            
		hCamera = NULL;
        }

	return SB_OK;
    }



int X2Camera::CCQueryTemperature(double& dCurTemp, double& dCurPower, char* lpszPower, const int nMaxLen, bool& bCurEnabled, double& dCurSetPoint)
    {
    static short lastTemp = -1;
    
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

    if(!bCamCanReadTemp) {
        dCurTemp = m_dCurSetTemp;
        dCurPower = m_dCurPower;
    	return ERR_NOT_IMPL;
        }

    // Unsupported
    dCurPower = -100.0;
    
    // This sometimes fails... let it go and just report last temperature
    short temp;
    HRESULT hr = Mallincam_get_Temperature(hCamera, &temp);
    if (hr != 0) {
        temp = lastTemp;
        sendToLog("Ignoring benign temp fetch failure: %x", hr);
    }

    // All is well proceed
    dCurTemp = double(temp) / 10.0;
    bCurEnabled = bCachedTECEnabled;
    dCurSetPoint = m_dCurSetTemp;
    lastTemp = temp;
	return SB_OK;
    }

int X2Camera::CCRegulateTemp(const bool& bOn, const double& dTemp)
    { 
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;
        
    if(!bCamHasTempControl) 
        return ERR_NOT_IMPL;

    // Save the current set point
    m_dCurSetTemp = dTemp;
    bCachedTECEnabled = bOn;    
    Mallincam_put_Option(hCamera, MALLINCAM_OPTION_TEC, bOn);
    short temp = short(dTemp * 10.0);
    Mallincam_put_Option(hCamera, MALLINCAM_OPTION_TECTARGET, temp);
    
    return SB_OK;
    }

int X2Camera::CCGetRecommendedSetpoint(double& RecTemp)
    {
	X2MutexLocker ml(GetMutex());

    if(!bCamHasTempControl) {
        RecTemp = 100;//Set to 100 if you cannot recommend a setpoint
        return ERR_NOT_IMPL;
        }
        
        
    RecTemp = -10.0;
    return SB_OK;
    }  



int X2Camera::CCGetExtendedSettingName(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, BasicStringInterface &sSettingName)
    {
    sSettingName = "Stream Mode";
    return SB_OK;
    }

int X2Camera::CCGetExtendedValueCount(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, int &nCount)
    {
    nCount = 2;
    return SB_OK;
    }
    
    
int X2Camera::CCGetExtendedValueName(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, const int nIndex, BasicStringInterface &sName)
    {
    if(nIndex == MC_PUSH_MODE)
        sName = "Single Exposures";
    else
        sName = "Streaming";
    
    return SB_OK;
    }
    
    
int X2Camera::CCStartExposureAdditionalArgInterface(const enumCameraIndex& Cam, const enumWhichCCD CCD, const double& dTime, enumPictureType Type,
                            const int& nABGState, const bool& bLeaveShutterAlone, const int &nIndex)
    {
    // Save the readout mode for later
    streamingMode = nIndex;

    return CCStartExposure(Cam, CCD, dTime, Type, nABGState, bLeaveShutterAlone);
    }
    
    
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// There is data to be had...
void __stdcall X2Camera::DataCallback(const void* pData, const MallincamFrameInfoV2* pInfo, int bSnap, void* pCallbackCtx)
    {
    X2Camera *pThis = (X2Camera*)pCallbackCtx;
    if(pThis == nullptr)
        return;
        
    if(pThis->GetMutex() == nullptr)
        return;
        
    X2MutexLocker ml(pThis->GetMutex());
    
	pThis->sendToLog("Reporting image of size %d x %d", pInfo->width, pInfo->height);
	pThis->sendToLog("Grabbing frame %d x %d", pThis->m_CachedWidth, pThis->m_CachedHeight);
    memcpy(pThis->imgData, pData, sizeof(unsigned short) * pThis->m_CachedWidth * pThis->m_CachedHeight);
    pThis->bImgReady = true;
    }


///////////////////////////////////////////////////////////////////////////////////////
// Data that comes through here is preview data... usually smaller resolution too.
void __stdcall X2Camera::EventCallback(unsigned nEvent, void* pCallbackCtx)
{
    X2Camera *pThis = (X2Camera*)pCallbackCtx;
    if(pThis == nullptr)
        return;

    if(pThis->GetMutex() == nullptr)
        return;
        
    X2MutexLocker ml(pThis->GetMutex());

    static int g_total = 0;
    static int g_totalstill = 0;
    pThis->sendToLog("Event callback, event: %d", nEvent);
    
    if(MALLINCAM_EVENT_TRIGGERFAIL == nEvent)
    {
        pThis->sendToLog("Trigger failed!");
    }
    
    // only in pull mode
    if (MALLINCAM_EVENT_IMAGE == nEvent)
    {
        MallincamFrameInfoV2 info = { 0 };
        HRESULT hr = Mallincam_PullImageV2(pThis->hCamera, pThis->imgData, 16, &info);
        if (FAILED(hr))
            pThis->sendToLog("failed to pull image, hr = %08x", hr);
        else {
            pThis->sendToLog("pull image ok, total = %u, resolution = %u x %u", ++g_total, info.width, info.height);
            pThis->bImgReady = true;
            }
    }
    else if (MALLINCAM_EVENT_STILLIMAGE == nEvent)
    {
        MallincamFrameInfoV2 info = { 0 };
        HRESULT hr = Mallincam_PullStillImageV2(pThis->hCamera, pThis->imgData, 16, &info);
        if (FAILED(hr))
            pThis->sendToLog("failed to pull still image, hr = %08x", hr);
        else {
            pThis->sendToLog("pull still image ok, total = %u, resolution = %u x %u", ++g_totalstill, info.width, info.height);
            pThis->bImgReady = true;
            }
    }    
}


int X2Camera::CCStartExposure(const enumCameraIndex& Cam, const enumWhichCCD CCD, const double& dTime, enumPictureType Type, const int& nABGState, const bool& bLeaveShutterAlone)
    {   
	X2MutexLocker ml(GetMutex());
    static double dLastExposureTime = -1.0;

	if (!m_bLinked)
		return ERR_NOLINK;
        
	bool bLight = true;
	switch (Type)
	{
		case PT_FLAT:
		case PT_LIGHT:
			bLight = true;
			break;
		case PT_AUTODARK:
		case PT_DARK:	
		case PT_BIAS:
			bLight = false;
			break;
		default:				
			bLight = true;
            break;
	}
    
    // The current image buffer is invalid
    bImgReady = false;
        
    // Set mode to single only if it has changed
/*    if(streamingMode == MC_PUSH_MODE) {// && nLastStreamingMode != MC_PUSH_MODE) { // Single exposure mode
        sendToLog("Setting push mode and exposure time\n");
        //Toupcam_Stop(hCamera);
        //Toupcam_Pause(hCamera, true);
		Toupcam_put_eSize(hCamera, 0);
        Toupcam_put_ExpoTime(hCamera, (unsigned int)(dTime * 1000000.0)); // in microseconds
        //Toupcam_StartPushModeV3(hCamera, DataCallback, (void *)this, nullptr, nullptr);    
        //Toupcam_Pause(hCamera, false);
        }
        
    // Set mode to streaming, but only if it's changed. Streaming mode does not need to trigger
    //if(streamingMode == MC_PULL_MODE && (nLastStreamingMode != MC_PULL_MODE || dLastExposureTime != dTime)) {
    //    sendToLog("Changing to pull mode");    
	//	Toupcam_put_eSize(hCamera, 0);
	//	Toupcam_StartPullModeWithCallback(hCamera, EventCallback, (void*)this);
    //    Toupcam_put_ExpoTime(hCamera, (unsigned int)(dTime * 1000000.0)); // in microseconds
    //    }
        
    // If in push mode (single exposure), trigger an exposure
    if(streamingMode == MC_PUSH_MODE) {
        sendToLog("Triggering an image");
        Toupcam_Trigger(hCamera, 1);
        }
*/
    HRESULT hr;
    hr = Mallincam_put_ExpoTime(hCamera, (unsigned int)(dTime * 1000000.0)); /* in microseconds */
    if(hr != 0)
        sendToLog("Mallincam_put_expoTime error: %x", hr);

    sendToLog("Triggering an image");
    hr = Mallincam_Trigger(hCamera, 1);
    if(hr != 0)
        sendToLog("Mallincam_Trigger error: %x", hr);

    exposureTime = dTime;
    dLastExposureTime = dTime;

    // Save streaming mode
    nLastStreamingMode = streamingMode;
    return SB_OK;
    }   


//////////////////////////////////////////////////////////////////////
// This get's called when the sky thinks you should be done
// pStatus is SBIG specific.
int X2Camera::CCIsExposureComplete(const enumCameraIndex& Cam, const enumWhichCCD CCD, bool* pbComplete, unsigned int* pStatus)
    {
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;
  
    *pbComplete = bImgReady;	// Mutex protected in background thread
   
    return SB_OK;
    }



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The frame is actually already read out by now
int X2Camera::CCReadoutImage(const enumCameraIndex& Cam, const enumWhichCCD& CCD, const int& nWidth, const int& nHeight, const int& nMemWidth, unsigned char* pMem)
    {
    X2MutexLocker ml(GetMutex());

    if (!m_bLinked)
        return ERR_NOLINK;
 
    unsigned short *pShortData = (unsigned short *)pMem; // Treat this like an unsigned short
	memset(pShortData, 0, nWidth * nHeight * 2);

    // Extract a subframe? If the width and height both are the same, it can't be a subframe
	if (nWidth == m_CachedWidth && nHeight == m_CachedHeight)
	{
		sendToLog("High speed copy, %d x %d", nWidth, nHeight);
		memcpy(pMem, imgData, sizeof(unsigned short)*nWidth*nHeight);
        }
    else {
        sendToLog("Software subframe. Data width %d, sub width %d", nWidth, nSubWidth);
        sbImgExtractSubFrame(pShortData, imgData, m_CachedWidth, nSubLeft, nSubTop, nSubWidth, nSubHeight);
        }
        
    // We need to scale the data up to 16-bits
    int diff = 16 - bitDepth;
    if(diff == 0)
        return SB_OK;
        
    for (int i = 0; i < nWidth * nHeight; i++)
            pShortData[i] = pShortData[i] << diff;
	
    return SB_OK;
    }



// Called after each exposure. Watch for bWasAborted flag. This is called before the readout... just FYI
int X2Camera::CCEndExposure(const enumCameraIndex& Cam, const enumWhichCCD CCD, const bool& bWasAborted, const bool& bLeaveShutterAlone)           
    {   
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

	int nErr = SB_OK;

    if(bWasAborted) {

        }


	return nErr;
    }

int X2Camera::CCGetChipSize(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nXBin, const int& nYBin, const bool& bOffChipBinning, int& nW, int& nH, int& nReadOut)
    {
	X2MutexLocker ml(GetMutex());
    sendToLog("Getting chip size");
    
	m_CachedCam = CCD;
	m_CachedBinX = nXBin;
    m_CachedBinY = nYBin;

	nW = cameraInfo.res[0].width / nXBin;
	nH = cameraInfo.res[0].height / nYBin;

	if ((nW % 2) != 0) nW--;
	if ((nH % 2) != 0) nH--;
        
	m_CachedWidth = nW;
	m_CachedHeight = nH;

	return SB_OK;
    }

int X2Camera::CCGetNumBins(const enumCameraIndex& Camera, const enumWhichCCD& CCD, int& nNumBins)
    {
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		nNumBins = 1;
	else
        nNumBins = 4;
        
	return SB_OK;
    }

int X2Camera::CCGetBinSizeFromIndex(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nIndex, long& nBincx, long& nBincy)
    {
	X2MutexLocker ml(GetMutex());

	switch (nIndex)
        {
		case 0:		nBincx=nBincy=1;break;
		case 1:		nBincx=nBincy=2;break;
		case 2:		nBincx=nBincy=3;break;
        case 3:     nBincx=nBincy=4;break;
		default:	nBincx=nBincy=1;break;
        }

	return SB_OK;
    }

int X2Camera::CCUpdateClock(void)
{   
	X2MutexLocker ml(GetMutex());

	return SB_OK;
}


// False is open, true is closed
// 0 is open, 1 is closed
// bLight frame is coming in
int X2Camera::CCSetShutter(bool bOpen)           
    {
	X2MutexLocker ml(GetMutex());
        
	return SB_OK;;
    }

// If bSychronous is true, block until time is finished
// If bSynchronous is false, someone is pressing the button and I get a zero when the button goes up
int X2Camera::CCActivateRelays(const int& nXPlus, const int& nXMinus, const int& nYPlus, const int& nYMinus, const bool& bSynchronous, const bool& bAbort, const bool& bEndThread)
    {   
	X2MutexLocker ml(GetMutex()); 
    return ERR_NOT_IMPL;
	}


// SBIG Specific, other models can ignore.
int X2Camera::CCPulseOut(unsigned int nPulse, bool bAdjust, const enumCameraIndex& Cam)
{   
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}

void X2Camera::CCBeforeDownload(const enumCameraIndex& Cam, const enumWhichCCD& CCD)
{
	X2MutexLocker ml(GetMutex());
}

void X2Camera::CCAfterDownload(const enumCameraIndex& Cam, const enumWhichCCD& CCD)
{
	X2MutexLocker ml(GetMutex());
	return;
}

int X2Camera::CCReadoutLine(const enumCameraIndex& Cam, const enumWhichCCD& CCD, const int& pixelStart, const int& pixelLength, const int& nReadoutMode, unsigned char* pMem)
{   
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}           

int X2Camera::CCDumpLines(const enumCameraIndex& Cam, const enumWhichCCD& CCD, const int& nReadoutMode, const unsigned int& lines)
{                                     
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}           


int X2Camera::CCSetBinnedSubFrame3(const enumCameraIndex& Camera, const enumWhichCCD& CCDOrig, const int& nLeft, const int& nTop, const int& nWidth, const int& nHeight)
    {
	X2MutexLocker ml(GetMutex());
//    static int nLastLeft, nLastTop, nLastWidth, nLastHeight = 0;
    static int nLastBin = -1;
    
    sendToLog("Setting up a subframe: %d, %d  %d x %d", nLeft, nTop, nWidth, nHeight);
    
	nSubLeft = nLeft;
    nSubTop = nTop;
    
    nSubWidth = nWidth;
    nSubHeight = nHeight;
                
    // If binning has changed, reset, must be done before the ROI command
    if(nLastBin != m_CachedBinX) {
        sendToLog("Binning changed to %d", m_CachedBinX);
        Mallincam_put_Option(hCamera, MALLINCAM_OPTION_BINNING, m_CachedBinX);
        nLastBin = m_CachedBinX;
        }

//    // Offsets must be even, not odd, same for width and height, and can't be
//    // less than 16
//    if((nSubLeft % 2) != 0) nSubLeft++;
//    if((nSubTop % 2) != 0) nSubTop++;
//    
//    if((nSubWidth % 2) != 0) nSubWidth++;
//    if((nSubHeight % 2) != 0) nSubHeight++;
//    if(nSubWidth < 16) nSubWidth = 16;
//    if(nSubHeight < 16) nSubHeight = 16;
//    
//    
//    if(nLastLeft != nLeft && nLastTop != nTop && nLastWidth != nWidth && nLastHeight != nHeight) {
//        sendToLog("Resetting ROI: %d, %d, %d x %d", nLeft, nTop, nWidth, nHeight);
//        Toupcam_put_Roi(hCamera, nLeft, nTop, nWidth, nHeight);
//        }
//        
//    nLastLeft = nLeft;
//    nLastTop = nTop;
//    nLastWidth = nWidth;
//    nLastHeight = nHeight;
                
	return SB_OK;
    }


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Matt says to implement this, otherwise if might crash if someone runs a new plug-in against a really old copy
// of TheSkyX.....
int X2Camera::CCSetBinnedSubFrame(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nLeft, const int& nTop, const int& nRight, const int& nBottom)
    {
	X2MutexLocker ml(GetMutex());
        
	nSubLeft = nLeft;
	nSubTop = nTop;
	nSubWidth = nRight - nLeft + 1;
	nSubHeight = nBottom - nTop + 1;
      
    return SB_OK;
    }




int X2Camera::CCSetImageProps(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nReadOut, void* pImage)
{
	X2MutexLocker ml(GetMutex());

	return SB_OK;
}

int X2Camera::CCGetFullDynamicRange(const enumCameraIndex& Camera, const enumWhichCCD& CCD, unsigned long& dwDynRg)
    {
	X2MutexLocker ml(GetMutex());
	// 65535 for 16 bit cameras (required implementation for @Focus to work).
	dwDynRg = 65535;
	return SB_OK;
    }


// SBIG Specific. All other camera's can ignore.
void X2Camera::CCMakeExposureState(int* pnState, enumCameraIndex Cam, int nXBin, int nYBin, int abg, bool bRapidReadout)
{
	X2MutexLocker ml(GetMutex());

	return;
}

int X2Camera::CCSettings(const enumCameraIndex& Camera, const enumWhichCCD& CCD)
{
	X2MutexLocker ml(GetMutex());

	return ERR_NOT_IMPL;
}

int X2Camera::CCSetFan(const bool& bOn)
{
	X2MutexLocker ml(GetMutex());

	return SB_OK;
}

int	X2Camera::pathTo_rm_FitsOnDisk(char* lpszPath, const int& nPathSize)
{
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

	//Just give a file path to a FITS and TheSkyX will load it
		
	return SB_OK;
}

CameraDriverInterface::ReadOutMode X2Camera::readoutMode(void)		
{
	X2MutexLocker ml(GetMutex());

    return CameraDriverInterface::rm_Image;
}


enumCameraIndex	X2Camera::cameraId()
{
	X2MutexLocker ml(GetMutex());

	return m_Camera;
}

void X2Camera::setCameraId(enumCameraIndex Cam)	
{
	m_Camera = Cam;
}

///////////////////////////////////////////////////////////
// Save all options to .ini file
void X2Camera::SaveAllOptions(void)
    {
	if (GetBasicIniUtil()) {
		GetBasicIniUtil()->writeString(KEY_X2CAM_ROOT, KEY_X2CAM_DEVICE, selectedCamera);
        GetBasicIniUtil()->writeInt(KEY_X2CAM_ROOT, KEY_X2CAM_GAIN, currentGain);
        sendToLog("Gain wrote: %d\n", currentGain);
		}
    }
    
///////////////////////////////////////////////////////////
// Load all options from .ini file
void X2Camera::LoadAllOptions(void)
    {
	if (GetBasicIniUtil()) {
		GetBasicIniUtil()->readString(KEY_X2CAM_ROOT, KEY_X2CAM_DEVICE, "", selectedCamera, 64);
        currentGain = GetBasicIniUtil()->readInt(KEY_X2CAM_ROOT, KEY_X2CAM_GAIN, -1);
        sendToLog("Gain Read: %d", currentGain);
	}
}
    
    
void X2Camera::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
    {
    (void)uiex;
    (void)pszEvent;
    }


int X2Camera::execModalSettingsDialog()
    {
	int nErr = SB_OK;
	X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
	X2GUIInterface*					ui = uiutil.X2UI();
	X2GUIExchangeInterface*			dx = NULL;//Comes after ui is loaded
	bool bPressedOK = false;

	if (NULL == ui)
		return ERR_POINTER;

	if ((nErr = ui->loadUserInterface(TOUP_GUI, deviceType(), m_nPrivateISIndex)))
		return nErr;

	if (NULL == (dx = uiutil.X2DX()))
		return ERR_POINTER;

	LoadAllOptions();

	unsigned cnt = Mallincam_EnumV2(deviceList);
	if (cnt == 0) {
		dx->comboBoxAppendString("comboBox", "No Cameras Detected");
		dx->setPropertyInt("groupBox", "enabled", false);
		dx->setPropertyInt("pushButtonOK", "enabled", false);
		}
	else {
		for (unsigned int i = 0; i < cnt; i++) 
			dx->comboBoxAppendString("comboBox", getCString(deviceList[i].displayname));
			
        if(!m_bLinked) {
            dx->setPropertyString("labelGain", "text", "Gain:");
        
        
            }
        else {
            char cTemp[32];
            sprintf(cTemp, "Gain (%d%% to %d%%):", minGain, maxGain);
            dx->setPropertyString("labelGain", "text", cTemp);
            dx->setPropertyInt("spinBoxGain", "minimum", minGain);
            dx->setPropertyInt("spinBoxGain", "maximum", maxGain);
            dx->setPropertyInt("spinBoxGain", "value", currentGain);
            }
	}

	//////////////////////////////////////////////////////////
	//Display the user interface
	if ((nErr = ui->exec(bPressedOK)))
		return nErr;

	// Retreive values from the user interface
	if (bPressedOK)
        {
        // Get radio button state
		int i = dx->currentIndex("comboBox");
		strncpy(selectedCamera, getCString(deviceList[i].displayname), 64);
        GetBasicIniUtil()->writeString(KEY_X2CAM_ROOT, KEY_X2CAM_DEVICE, selectedCamera);
        sendToLog("Saving camera: %s", selectedCamera);
        if(m_bLinked) {
            int temp;
            dx->propertyInt("spinBoxGain", "value", temp);
            sendToLog("Value from control is %d", temp);
            currentGain = temp;
            Mallincam_put_ExpoAGain(hCamera, currentGain);

            SaveAllOptions();
            }
            
        }
        
	return nErr;
    }
