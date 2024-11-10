#ifndef _CircuitLockedInterface_H
#define _CircuitLockedInterface_H

#define CircuitLockedInterface_Name "com.bisque.TheSkyX.CircuitLockedInterface/1.0"

class BasicStringInterface;

/*!
\brief The CircuitLockedInterface allows power control drivers to provide TheSky a means to lock a circuit.  A locked circuit is still visible but its state cannot be changed.

\ingroup Interface

This interface is optional.  By default, TheSky will consider all circuits unlocked.

Don't forget to respond accordingly in your queryAbstraction().
*/

class CircuitLockedInterface
{
public:

	virtual ~CircuitLockedInterface(){}

public:
	//CircuitLockedInterface
	/*! Called by TheSky to query if a particular circuit is locked or not.
	\param nZeroBasedIndex (in) the zero based index of the circuit.
	\param bZeroForNoOneForLocked (out) returns if the circuit is locked or not.
	*/
	virtual int circuitLocked(const int& nIndex, bool& bZeroForNoOneForLocked)=0;

};

#endif