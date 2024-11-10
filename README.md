# TheSky-X2
Open Source device plugins for TheSky using the X2 interface

TheSky is the leading cross platform astronomy application today. It supports various devices on Windows, macOs, and Linux using it's own interface called "X2".

You can find out more about X2 and get some sample device plug-ins directly from Software Bisque here:

https://www.bisque.com/wp-content/x2standard/

I've archived the /licensedinterfaces folder here (the complete kit is available at the above URL) for ease of building these plug-ins. Device platform SDK's are also included as .zip archives for some devices, but I do not promise to keep them entirely up to date. You may need to go to the vendors themselves in the case where an SDK is needed. The only reason I'm putting the SDK's here is in case devices or vendors fall away, here is an archive somewhere that someone could potentially use to build support for legacy devices down the road.

On the other hand many devices are simple serial devices, and some will require libUSB rather than a specific vendors SDK.

Another good source of X2 plug-ins and source code is https://rti-zone.org/software.php

These are offered without warranty or support. I do not have time (or patience) to support end users trying to build these for themselves. If anyone would like to contribute to this repository, send me a PR from your own fork, I'll be happy to review it and include changes here. In fact, I'd love to have someone else jump in to curate these plug-ins, and perhaps make a nice uniform cmake build system for them all that works on all platforms. As I am currently somewhat "over employed", all I can do is offer the code here as a gesture for now rather than sit on them till they rot away and have no use to anyone.

Richard S. Wright Jr.
rwright@starstonesoftware.com
