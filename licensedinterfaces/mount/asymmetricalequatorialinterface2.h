#ifndef _AsymmetricalEquatorialInterface2_H
#define _AsymmetricalEquatorialInterface2_H

#include "mounttypeinterface.h"
#include "beyondthepoleinterface.h"

#define AsymmetricalEquatorialInterface2_Name "com.bisque.TheSkyX.AsymmetricalEquatorialInterface2/1.0"

/*!
\brief The AsymmetricalEquatorialInterface2 for equtorial mounts.

\ingroup Interface

If a X2 mount driver implements this interface, the mount is an asymmetrical equtorial mount (e.g. GEM or cross-axis).
\sa SymmetricalEquatorialInterface
\sa BeyondThePoleInterface
*/

class AsymmetricalEquatorialInterface2
{
public:

	virtual ~AsymmetricalEquatorialInterface2(){}

	/*!
	Simply returns the appropriate type of mount.
	*/
	MountTypeInterface::Type mountType(){return MountTypeInterface::Asymmetrical_Equatorial;}


};

#endif