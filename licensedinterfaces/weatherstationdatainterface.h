#ifndef _WeatherStationDataInterface_H
#define _WeatherStationDataInterface_H

#ifndef INT_MIN
#define INT_MIN ((int) 0x8000/0x80000000)
#endif

#define WeatherStationDataInterface_Name "com.bisque.TheSkyX.WeatherStationDataInterface/1.0"

/*!
\brief The WeatherStationDataInterface gives x2 authors a means to write a TheSky weather station driver.

\ingroup Interface

The  primary purpose of this interface is to make it easy/simple for x2 implementors to get weather station data into TheSky. TheSky calls the weatherStationData() method at a regular interval to keep weather station information up-to-date. 
At minimum integration, a weather station driver can return one or say a few of the weatherStationData() parameters to have them displayed in TheSky weather station tab.
At a maximum integration, this weather station information resolves to a go or no-go state which TheSky can be configured to integrate with an enclosure (dome/roof) and open or close it accordingly.

For a working example, see the x2weatherstation example.

Support for this interface requires TheSky build 13488 or later.  Use TheSkyXFacadeForDriversInterface::build() to determine the build of TheSky in use an act accordingly based on your requirements.

Don't forget to respond accordingly in your queryAbstraction().
*/
class WeatherStationDataInterface
{
public:

	//! Cloud condition.
	enum class x2CloudCond {
		cloudUnknown = 0,	/*!<Cloud condition unknown.*/
		cloudClear = 1,		/*!<Cloud condition clear.*/
		cloudCloudy = 2,	/*!<Cloud condition cloudy.*/
		cloudVeryCloudy = 3,/*!<Cloud condition very cloudy.*/
		cloudDisabled = 4,	/*!<Cloud sensor disabled at TheSky level.*/
		cloudNoSensor = INT_MIN/*!<Hardware not equipped with a cloud sensor, graefully ignore/hidden.*/
	};

	//! Wind condition.
	enum class x2WindCond {
		windUnknown = 0,	/*!<Wind condition unknown.*/
		windCalm = 1,		/*!<Wind condition calm.*/
		windWindy = 2,		/*!<Wind condition windy.*/
		windVeryWindy = 3,	/*!<Wind condition very windy.*/
		windDisabled = 4,	/*!<Wind sensor disabled at TheSky level.*/
		windNoSensor = INT_MIN/*!<Hardware not equipped with a wind sensor, gracefully ignore/hidden.*/
	};

	//! Rain condition.
	enum class x2RainCond {
		rainUnknown = 0,	/*!<Rain condition unknown.*/
		rainDry = 1,		/*!<Rain condition dry.*/
		rainWet = 2,		/*!<Rain condition wet.*/
		rainRain = 3,		/*!<Rain condition raining.*/
		rainDisabled = 4,	/*!<Rain sensor disabled at TheSky level.*/
		rainNoSensor = INT_MIN/*!<Hardware not equipped with a rain sensor, gracefully ignore/hidden.*/
	};

	//! Daylight condition.
	enum class x2DayCond {
		dayUnknown = 0,		/*!<Day condition unknown.*/
		dayDark = 1,		/*!<Day condition dark.*/
		dayLight = 2,		/*!<Day condition light.*/
		dayVeryLight = 3,	/*!<Day condition very light.*/
		dayDisabled = 4,	/*!<Day sensor disabled at TheSky level.*/
		dayNoSensor = INT_MIN/*!<Hardware not equipped with a day sensor, gracefully ignore/hidden.*/
	};

	//! Windspeed units.
	enum class x2WindSpeedUnit {
		windSpeedKph= 0,	/*!<Windspeed is kilometers/hour.*/
		windSpeedMph= 1,	/*!<Windspeed is miles/hour.*/
		windSpeedMps= 2,	/*!<Windspeed is meters/second.*/
	};

	virtual ~WeatherStationDataInterface() {}

public:

	/**
	* TheSky calls this method to query this driver to know the units of temperature returned. Note, TheSky always displays weather station temperature in degrees C.
	* 
	* Override and return false to have temperature units be degrees F.
	*/
	virtual bool temperatureIsInDegreesC()
	{
		return true;
	}

	/**
	* TheSky calls this method to query this driver to know the units of windspeed returned.  Note, TheSky always displays weather station windspeed in meters/second.
	*/
	virtual WeatherStationDataInterface::x2WindSpeedUnit windSpeedUnit()
	{
		return WeatherStationDataInterface::x2WindSpeedUnit::windSpeedMps;
	}

	/*!
	TheSky calls this method to have the x2 driver return the most up to date weather information.
 
	This data is typical from the Boltwood Cloud sensor but can easily be extended/used by other weather station hardware providing the same information.  
 
	Note all values are marked as [in] because they are initialized so that if left unchanged, TheSky deems the weather station as not being equipped with the corresonding sensor.  IOW, x2 implementors should only alter values that your pariticular hardware is able to measure and return.
 
	@param[in,out] dSkyTemp - sky ambient temperature in context of cloud sensing.
	@param[in,out] dAmbTemp - ambient temperature.
	@param[in,out] dSenT - sensor case temperature.
	@param[in,out] dWind - wind speed.
	@param[in,out] nPercentHumdity - relative humidity in %.
	@param[in,out] dDewPointTemp - dew point temperature.
	@param[in,out] nRainHeaterPercentPower - heater setting in % 
	@param[in,out] nRainFlag - rain flag, =0 for dry, =1 for rain in the last minute, =2 for rain right now
	@param[in,out] nWetFlag - wet flag, =0 for dry, =1 for wet in the last minute, =2 for wet right now
	@param[in,out] nSecondsSinceGoodData - seconds since the last valid data, only used for Boltwood 
	@param[in,out] dVBNow - date/time given as the VB6 Now() function result (in days) data last captured, only used for Boltwood 
	@param[in,out] dBarometricPressure - the barometric pressure in mB.  This is the actual pressure, which is what the mount pointing calculations need, not a QNH figure.
	@param[in,out] cloudCondition - see  WeatherStationDataInterface::x2CloudCond
	@param[in,out] windCondition - see  WeatherStationDataInterface::x2WindCond
	@param[in,out] rainCondition - see  WeatherStationDataInterface::x2RainCond
	@param[in,out] daylightCondition - see  WeatherStationDataInterface::x2DayCond
	@param[in,out] nRoofCloseThisCycle- Set to 1 if this weather station hardware has determined close conditions (no-go) or 0 if not (good-to-go).  Leave unchanged to have TheSky's internal logic dictate go or no-go.  TheSky determines go or no-go conditions by internal logic using nearly all values here, and possibly some values outside the context of weather data.  For the case when a weather station implementation returns a subset of these weather parameters, the practical solution is for this weather station to dictate this parameter.  Otherwise, the permutations of having go no-go logic to apply to all the possible number of subsets is too vast. A common <i><b>subset</b></i> of values for go/no-go might be supported in the future if this subset can be established.
	* 
	*/
	virtual int weatherStationData(

		double& dSkyTemp,
		double&	dAmbTemp,
		double& dSenT,
		double& dWind,

		int&	nPercentHumdity,
		double& dDewPointTemp,
		int&	nRainHeaterPercentPower,
		int&	nRainFlag,
		int&	nWetFlag,
		int&	nSecondsSinceGoodData,
		double& dVBNow,
		double& dBarometricPressure,

		WeatherStationDataInterface::x2CloudCond& cloudCondition,
		WeatherStationDataInterface::x2WindCond& windCondition,
		WeatherStationDataInterface::x2RainCond& rainCondition,
		WeatherStationDataInterface::x2DayCond& daylightCondition,

		int& nRoofCloseThisCycle
	) = 0;
};

#endif