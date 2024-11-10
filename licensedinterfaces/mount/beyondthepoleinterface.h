#ifndef _BeyondThePoleInterface_H
#define _BeyondThePoleInterface_H

#define BeyondThePoleInterface_Name "com.bisque.TheSkyX.BeyondThePoleInterface/1.0"

/*!
\brief The BeyondThePoleInterface.

\ingroup Interface

Implement this interface to allow TheSkyX to slew the mount "beyond the pole".


If knowsBeyondThePole() returns false (the default),

For an asymmetrical equatorial mount,
the mount cannot distinguish unambiguosly if the OTA end of the declination axis
is either east or west of the pier. This somewhat restricts use of the
mount with TPoint - the mount must always have the OTA end of the declination
axis higher than the counterweights. In other words, the mount should not slew past the meridian.

For a symmetrical equatorial mount, the mount does not allow the eyepiece/camera to swing through the fork.

\sa SymmetricalEquatorialInterface
\sa AsymmetricalEquatorialInterface2
*/

class BeyondThePoleInterface 
{
public:

	virtual ~BeyondThePoleInterface(){}

	/*!
	If knowsBeyondThePole() returns false, the mount
	cannot distinguish unambiguosly if the OTA end of the declination axis
	is either east or west of the pier. This somewhat restricts use of the
	mount with TPoint - the mount must always have the OTA end of the declination
	axis higher than the counterweights. In other words, the mount should not slew past the meridian.
	*/
	virtual bool knowsBeyondThePole() { return false; }

	/*!
	If knowsBeyondThePole() returns true,
	then beyondThePole() tells TheSkyX unambiguously
	if the OTA end of the declination axis
	is either east (0) or west of the pier (1).
	Note, the return value must be correct even
	for cases where the OTA end of the Dec axis
	is lower than the counterweights.
	*/
	virtual int beyondThePole(bool& bYes) { bYes = false; return 0; }

	/*!
	Return the hour angle at which the mount automatically flips.
	*/
	virtual double flipHourAngle() {return 0;}

	/*!
	Return the east and west hour angle limits.
	*/
	virtual int hourAngleLimits(double& dHoursEast, double& dHoursWest){dHoursEast=dHoursWest=1.0;return 0;}

	/*!
	If the hardware is capable of a configurable flip hour angle, return non-zero, otherwise return zero.
	*/
	virtual bool canSetFlipHourAngle() { return false; }

	/*!
	Allow TheSky to set the the hour angle at which the mount automatically flips.
	*/
	virtual void setFlipHourAngle(double dFlipHourAngle) { return; }


};

#endif
