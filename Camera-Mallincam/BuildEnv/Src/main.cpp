#include "stdio.h"
#include "main.h"
#include "x2camera.h"
#include "../../licensedinterfaces/basicstringinterface.h"

//Ths file normally does not require changing

#ifdef MEADE_CAMERA
#define PLUGIN_NAME "MeadeDSI"
#else
#define PLUGIN_NAME "Mallincam"
#endif

extern "C" PlugInExport int sbPlugInName2(BasicStringInterface& str)
{
	str = PLUGIN_NAME;

	return 0;
}

extern "C" PlugInExport int sbPlugInFactory2(	const char* pszSelection, 
												const int& nInstanceIndex,
												SerXInterface					* pSerXIn, 
												TheSkyXFacadeForDriversInterface* pTheSkyXIn, 
												SleeperInterface		* pSleeperIn,
												BasicIniUtilInterface   * pIniUtilIn,
												LoggerInterface			* pLoggerIn,
												MutexInterface			* pIOMutexIn,
												TickCountInterface		* pTickCountIn,
												void** ppObjectOut)
{
	*ppObjectOut = NULL;
	X2Camera* gpMyImpl=NULL;

	if (NULL == gpMyImpl)
		gpMyImpl = new X2Camera(	pszSelection, 
									nInstanceIndex,
									pSerXIn, 
									pTheSkyXIn, 
									pSleeperIn,
									pIniUtilIn,
									pLoggerIn,
									pIOMutexIn,
									pTickCountIn);

		*ppObjectOut = gpMyImpl;

	return 0;
}

