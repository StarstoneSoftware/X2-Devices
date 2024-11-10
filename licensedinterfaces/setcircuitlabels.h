#ifndef _SetCircuitLabelsInterface_H
#define _SetCircuitLabelsInterface_H

#define SetCircuitLabelsInterface_Name "com.bisque.TheSkyX.SetCircuitLabelsInterface/1.0"

class BasicStringInterface;

/*!
\brief The SetCircuitLabelsInterface allows power control drivers to provide TheSky a means to edit the text label assoicated with each circuit.

\ingroup Interface

This interface is optional.  By default, TheSky will control the text labels assoicated with each ciricuit of a power control device.  If the power control
device is capable and it and controls text labels associated with each circuit and allows setting them, support this interface.

Don't forget to respond accordingly in your queryAbstraction().
*/

class SetCircuitLabelsInterface
{
public:

	virtual ~SetCircuitLabelsInterface(){}

public:
	//SetCircuitLabelsInterface
	/*! Called by TheSky to set the text label of a circuit.
	\param nZeroBasedIndex (in) the zero based index of the circuit.
	\param str (in) the text label associated with this circuit.
	*/
	virtual int setCircuitLabel(const int& nZeroBasedIndex, const char* str) = 0;

};

#endif