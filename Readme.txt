The "ics710" driver/device support module interfaces GE-IP high-resolution (24-bit)
compactPCI digitizer (google 'ICS-710' for detailed specs). Currently this driver
only works under Linux (tested on Linux kernel 2.6.32-5-686 and Debian 6.0.2).

Since the EPICS driver is based on ICS-710 Liunx SDK which includes Device Driver
and API, you should contact GE-IP to get that software package, compile and
install it following 'readme' and the shell scripts before you compile and use
this EPICS driver.

If your cPCI CPU board has no disk, you might be interested in using PXE to
netboot Debirf (http://cmrg.fifthhorseman.net/wiki/debirf, or google it).

    Web Browser view of sources:
    http://epics.hg.sourceforge.net/hgweb/epics/ics710 Read-only Mercurial
    access: hg clone http://epics.hg.sourceforge.net:8000/hgroot/epics/ics710
    Read/write Mercurial access: hg clone
    ssh://USERNAME@epics.hg.sourceforge.net/hgroot/epics/ics710
