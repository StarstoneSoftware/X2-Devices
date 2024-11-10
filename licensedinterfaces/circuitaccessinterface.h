#ifndef _CircuitAccessInterface_H
#define _CircuitAccessInterface_H

#define CircuitAccessInterface_Name "com.bisque.TheSkyX.CircuitAccessInterface/1.0"

class BasicStringInterface;

/*!
\brief The CircuitAccessInterface allows power control drivers to provide TheSky a means to dictact access to a circuit, typically by a built in means of authentication.  A locked circuit is still visible but its state cannot be changed.

\ingroup Interface

This interface is optional.  By default, TheSky will consider all circuits accessible.

Don't forget to respond accordingly in your queryAbstraction().
*/

class CircuitAccessInterface
{
public:

	virtual ~CircuitAccessInterface(){}

public:
	//CircuitAccessInterface
	/*! Called by TheSky to query if a particular circuit is locked or not.
	\param nZeroBasedIndex (in) the zero based index of the circuit.
	\param bZeroForNoAccessOneForAccess (out) returns the access state of the circuit, zero means no access, one means access.
	*/
	virtual int circuitAccess(const int& nIndex, bool& bZeroForNoAccessOneForAccess)=0;

};

#endif