COTS-BPRD: Commercial Off-The-Shelf Backpressure Routing Daemon
===============================================================

COTS-BPRD (BPRD) is a backpressure-inspired routing daemon targeted at
commercial-off-the-shelf Linux-based systems with IEEE 802.11 wireless
networking capabilities.  Based on a wide body of research literature,
backpressure routing strategies are roughly centered around forwarding
schemes that dynamically route traffic away from areas of high
congestion along congestion gradients.  BPRD consists of a network
layer approach to backpressure routing and is implemented as a multi-
threaded user-space program written in C.  Key dependencies/features
of BPRD include:

* *libnl* for dynamically querying and configuring network interfaces,
* *iptables* and *libnetfilterqueue* for capturing/releasing packets
  and tracking commodity congestion levels,
* *libpacketbb* for reading and writing hello messages that
  communicate congestion levels between neighboring nodes, and
* *libnlroute* for dynamically querying and configuring each node's
  routing table.


Build/Installation Notes:
=========================

* BPRD development took place back in 2012 on Ubuntu 11.04 32-bit
  Server.  We have made necessary updates to at least ensure successful
  compilation on Ubuntu 14.04 64-bit Server, but no guarantees exist on
  whether or not BPRD operates as intended.
* The following dependences have been noted on Ubuntu 14.04 64-bit Server:
  + build-essential
  + autotools-dev
  + autoconf
  + libtool
  + pkg-config
  + libnetfilter-queue-dev
  + libnl-3-dev
  + libnl-route-3-dev
  + doxygen (optional)

* BPRD makes use of autotools:

      ./autogen.sh
      ./configure
      make
      make install

* Doxygen documentation may be built using:

      make doc

* Optional syslog configuration file provided in scripts/
* Optional bash autocompletion file provided in scripts/


Usage Notes:
============

* List command line options:

      bprd --help

* IPv6 not supported despite the ambitious command line option
* Commodities are specified in "ADDR,ID" tuples.  BPRD will treat all UDP
  packets destined to ADDR as a tracked commodity and will place them in a
  libnetfilter queue with index ID.
* Commodities may be specified on the command line or via an input file.
  A sample input configuration file provided in scripts/.


Known Issues:
=============

* `fifo_length()` does not check for null queue
* `router_cleanup()` not called when SIGINT issued


Acknowledgements:
=================

Many useful resources were consulted in the development of BPRD.  We have
attempted to provide correct attribution where appropriate and possible.
If you feel that we missed an opportunity to do so, please contact us!
