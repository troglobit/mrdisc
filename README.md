[![Travis Status][]][Travis]

`mrdisc(8)` is a UNIX compatible implementation of [RFC4286][], the
Multicast Router Discovery Protocol.  It is intended to be a way of
informing IGMP/MLD snoopers on a LAN of multicast capable routers.

    Usage: mrdisc IFNAME [IFNAME ...]

When complete, `mrdisc(8)` will be integrated in the SMCRoute, mrouted,
and pimd multicast routing daemons.  In fairness, both the Linux and
*BSD kernels should probably implement this instead.  When a multicast
routing daemon opens a routing socket and declares a set of VIFs, the
kernel knows which interfaces to send IGMP/MLD discovery frames on.

Also, both Linux and the *BSD kernels have bridges with built-in IGMP
and MLD snooping support that would greatly benefit from dynamically
learning multicast router ports.

You are free to use this software as you like, as long as you abide by
the terms of the [ISC License][License].

[RFC4286]:       https://tools.ietf.org/html/rfc4286
[License]:       https://en.wikipedia.org/wiki/ISC_license
[Travis]:        https://travis-ci.org/troglobit/netcalc
[Travis Status]: https://travis-ci.org/troglobit/netcalc.png?branch=master
