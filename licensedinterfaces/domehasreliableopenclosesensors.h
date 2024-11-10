#ifndef _DomeHasHighlyRelaibleOpenCloseSensors_H
#define _DomeHasHighlyRelaibleOpenCloseSensors_H

#define DomeHasHighlyRelaibleOpenCloseSensors_Name "com.bisque.TheSkyX.DomeHasHighlyRelaibleOpenCloseSensors/1.0"

/*!   
\brief The DomeHasHighlyRelaibleOpenCloseSensors is an optional interface that indicates the dome that supports this interface has very reliable sensors for the critical open and close operations that
can provide highly reliable, hardware feedback to indicate the dome is open or closed. In other words, the calls to dapiIsOpenComplete() dapiIsCloseComplete() expect to provide the actual, hardware state of open/close sensor at 
the time of the call, callable at any time, provided there is a link to the dome.

\ingroup Interface

If this interface is supported, after a succesful open or close, TheSky will report the dome Slit state as "Open" or "Closed" where as domes that don't support this interface
will have thier slit state reported as "Last open succeeded" or "Last close succeeded".

TheSky changes its behavior slightly for open/close and auto-startup/shutdown based upon this interface. For domes that don't support it, TheSky internally keeps a peristent, software value for the slit state otherwise, TheSky
relies entirely upon this interface for the dome slit state.  There are also times when TheSky has to decide if the dome should be closed and without supporting this interface, TheSky will simply issue the close again since 
w/o supporting this interface, the feedback on open and close isn't considered reliable.

Support for this interface requires TheSky build 13488 or later.  Use TheSkyXFacadeForDriversInterface::build() to determine the build of TheSky in use an act accordingly based on your requirements.
*/

class DomeHasHighlyRelaibleOpenCloseSensors
{
public:

	virtual ~DomeHasHighlyRelaibleOpenCloseSensors(){}

	/*There is no implementation needed, merely supporting this interface (in queryAbstration) is all that is required.*/
public:

};

#endif