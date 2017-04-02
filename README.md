`mrdisc(8)` is a UNIX compatible implementation of [RFC4286][], the
Multicast Router Discovery Protocol.  The protocol is intended to be a
way of informing IGMP/MLD snoopers on a LAN of any multicast capable
routers.

When complete, `mrdisc(8)` will be integrated in the SMCRoute, mrouted,
and pimd multicast routing daemons.  In fairness, both the Linux and
*BSD kernels should probably implement this instead.  When a multicast
routing daemon opens a routing socket and declares a set of VIFs the
kernel knows which interfaces to send IGMP/MLD discovery frames on.
Also, both kernels have bridges with IGMP/MLD snooping support.

You are free to use this software as you like, as long as you abide by
the terms of the [ISC License][]

[RFC4286]: https://tools.ietf.org/html/rfc4286
[ISC License]: https://en.wikipedia.org/wiki/ISC_license
