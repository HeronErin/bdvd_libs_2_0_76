/* vi: set sw=4 ts=4: */
/*
 * Copyright (c) 1988, 1989, 1991, 1994, 1995, 1996, 1997, 1998, 1999, 2000
 *      The Regents of the University of California.  All rights reserved.
 *
 * Busybox port by Vladimir Oleynik (C) 2005 <dzo@simtreas.ru>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

//#define version "1.4a12"


/*
 * traceroute host  - trace the route ip packets follow going to "host".
 *
 * Attempt to trace the route an ip packet would follow to some
 * internet host.  We find out intermediate hops by launching probe
 * packets with a small ttl (time to live) then listening for an
 * icmp "time exceeded" reply from a gateway.  We start our probes
 * with a ttl of one and increase by one until we get an icmp "port
 * unreachable" (which means we got to "host") or hit a max (which
 * defaults to 30 hops & can be changed with the -m flag).  Three
 * probes (change with -q flag) are sent at each ttl setting and a
 * line is printed showing the ttl, address of the gateway and
 * round trip time of each probe.  If the probe answers come from
 * different gateways, the address of each responding system will
 * be printed.  If there is no response within a 5 sec. timeout
 * interval (changed with the -w flag), a "*" is printed for that
 * probe.
 *
 * Probe packets are UDP format.  We don't want the destination
 * host to process them so the destination port is set to an
 * unlikely value (if some clod on the destination is using that
 * value, it can be changed with the -p flag).
 *
 * A sample use might be:
 *
 *     [yak 71]% traceroute nis.nsf.net.
 *     traceroute to nis.nsf.net (35.1.1.48), 30 hops max, 56 byte packet
 *      1  helios.ee.lbl.gov (128.3.112.1)  19 ms  19 ms  0 ms
 *      2  lilac-dmc.Berkeley.EDU (128.32.216.1)  39 ms  39 ms  19 ms
 *      3  lilac-dmc.Berkeley.EDU (128.32.216.1)  39 ms  39 ms  19 ms
 *      4  ccngw-ner-cc.Berkeley.EDU (128.32.136.23)  39 ms  40 ms  39 ms
 *      5  ccn-nerif22.Berkeley.EDU (128.32.168.22)  39 ms  39 ms  39 ms
 *      6  128.32.197.4 (128.32.197.4)  40 ms  59 ms  59 ms
 *      7  131.119.2.5 (131.119.2.5)  59 ms  59 ms  59 ms
 *      8  129.140.70.13 (129.140.70.13)  99 ms  99 ms  80 ms
 *      9  129.140.71.6 (129.140.71.6)  139 ms  239 ms  319 ms
 *     10  129.140.81.7 (129.140.81.7)  220 ms  199 ms  199 ms
 *     11  nic.merit.edu (35.1.1.48)  239 ms  239 ms  239 ms
 *
 * Note that lines 2 & 3 are the same.  This is due to a buggy
 * kernel on the 2nd hop system -- lbl-csam.arpa -- that forwards
 * packets with a zero ttl.
 *
 * A more interesting example is:
 *
 *     [yak 72]% traceroute allspice.lcs.mit.edu.
 *     traceroute to allspice.lcs.mit.edu (18.26.0.115), 30 hops max
 *      1  helios.ee.lbl.gov (128.3.112.1)  0 ms  0 ms  0 ms
 *      2  lilac-dmc.Berkeley.EDU (128.32.216.1)  19 ms  19 ms  19 ms
 *      3  lilac-dmc.Berkeley.EDU (128.32.216.1)  39 ms  19 ms  19 ms
 *      4  ccngw-ner-cc.Berkeley.EDU (128.32.136.23)  19 ms  39 ms  39 ms
 *      5  ccn-nerif22.Berkeley.EDU (128.32.168.22)  20 ms  39 ms  39 ms
 *      6  128.32.197.4 (128.32.197.4)  59 ms  119 ms  39 ms
 *      7  131.119.2.5 (131.119.2.5)  59 ms  59 ms  39 ms
 *      8  129.140.70.13 (129.140.70.13)  80 ms  79 ms  99 ms
 *      9  129.140.71.6 (129.140.71.6)  139 ms  139 ms  159 ms
 *     10  129.140.81.7 (129.140.81.7)  199 ms  180 ms  300 ms
 *     11  129.140.72.17 (129.140.72.17)  300 ms  239 ms  239 ms
 *     12  * * *
 *     13  128.121.54.72 (128.121.54.72)  259 ms  499 ms  279 ms
 *     14  * * *
 *     15  * * *
 *     16  * * *
 *     17  * * *
 *     18  ALLSPICE.LCS.MIT.EDU (18.26.0.115)  339 ms  279 ms  279 ms
 *
 * (I start to see why I'm having so much trouble with mail to
 * MIT.)  Note that the gateways 12, 14, 15, 16 & 17 hops away
 * either don't send ICMP "time exceeded" messages or send them
 * with a ttl too small to reach us.  14 - 17 are running the
 * MIT C Gateway code that doesn't send "time exceeded"s.  God
 * only knows what's going on with 12.
 *
 * The silent gateway 12 in the above may be the result of a bug in
 * the 4.[23]BSD network code (and its derivatives):  4.x (x <= 3)
 * sends an unreachable message using whatever ttl remains in the
 * original datagram.  Since, for gateways, the remaining ttl is
 * zero, the icmp "time exceeded" is guaranteed to not make it back
 * to us.  The behavior of this bug is slightly more interesting
 * when it appears on the destination system:
 *
 *      1  helios.ee.lbl.gov (128.3.112.1)  0 ms  0 ms  0 ms
 *      2  lilac-dmc.Berkeley.EDU (128.32.216.1)  39 ms  19 ms  39 ms
 *      3  lilac-dmc.Berkeley.EDU (128.32.216.1)  19 ms  39 ms  19 ms
 *      4  ccngw-ner-cc.Berkeley.EDU (128.32.136.23)  39 ms  40 ms  19 ms
 *      5  ccn-nerif35.Berkeley.EDU (128.32.168.35)  39 ms  39 ms  39 ms
 *      6  csgw.Berkeley.EDU (128.32.133.254)  39 ms  59 ms  39 ms
 *      7  * * *
 *      8  * * *
 *      9  * * *
 *     10  * * *
 *     11  * * *
 *     12  * * *
 *     13  rip.Berkeley.EDU (128.32.131.22)  59 ms !  39 ms !  39 ms !
 *
 * Notice that there are 12 "gateways" (13 is the final
 * destination) and exactly the last half of them are "missing".
 * What's really happening is that rip (a Sun-3 running Sun OS3.5)
 * is using the ttl from our arriving datagram as the ttl in its
 * icmp reply.  So, the reply will time out on the return path
 * (with no notice sent to anyone since icmp's aren't sent for
 * icmp's) until we probe with a ttl that's at least twice the path
 * length.  I.e., rip is really only 7 hops away.  A reply that
 * returns with a ttl of 1 is a clue this problem exists.
 * Traceroute prints a "!" after the time if the ttl is <= 1.
 * Since vendors ship a lot of obsolete (DEC's Ultrix, Sun 3.x) or
 * non-standard (HPUX) software, expect to see this problem
 * frequently and/or take care picking the target host of your
 * probes.
 *
 * Other possible annotations after the time are !H, !N, !P (got a host,
 * network or protocol unreachable, respectively), !S or !F (source
 * route failed or fragmentation needed -- neither of these should
 * ever occur and the associated gateway is busted if you see one).  If
 * almost all the probes result in some kind of unreachable, traceroute
 * will give up and exit.
 *
 * Notes
 * -----
 * This program must be run by root or be setuid.  (I suggest that
 * you *don't* make it setuid -- casual use could result in a lot
 * of unnecessary traffic on our poor, congested nets.)
 *
 * This program requires a kernel mod that does not appear in any
 * system available from Berkeley:  A raw ip socket using proto
 * IPPROTO_RAW must interpret the data sent as an ip datagram (as
 * opposed to data to be wrapped in a ip datagram).  See the README
 * file that came with the source to this program for a description
 * of the mods I made to /sys/netinet/raw_ip.c.  Your mileage may
 * vary.  But, again, ANY 4.x (x < 4) BSD KERNEL WILL HAVE TO BE
 * MODIFIED TO RUN THIS PROGRAM.
 *
 * The udp port usage may appear bizarre (well, ok, it is bizarre).
 * The problem is that an icmp message only contains 8 bytes of
 * data from the original datagram.  8 bytes is the size of a udp
 * header so, if we want to associate replies with the original
 * datagram, the necessary information must be encoded into the
 * udp header (the ip id could be used but there's no way to
 * interlock with the kernel's assignment of ip id's and, anyway,
 * it would have taken a lot more kernel hacking to allow this
 * code to set the ip id).  So, to allow two or more users to
 * use traceroute simultaneously, we use this task's pid as the
 * source port (the high bit is set to move the port number out
 * of the "likely" range).  To keep track of which probe is being
 * replied to (so times and/or hop counts don't get confused by a
 * reply that was delayed in transit), we increment the destination
 * port number before each probe.
 *
 * Don't use this as a coding example.  I was trying to find a
 * routing problem and this code sort-of popped out after 48 hours
 * without sleep.  I was amazed it ever compiled, much less ran.
 *
 * I stole the idea for this program from Steve Deering.  Since
 * the first release, I've learned that had I attended the right
 * IETF working group meetings, I also could have stolen it from Guy
 * Almes or Matt Mathis.  I don't know (or care) who came up with
 * the idea first.  I envy the originators' perspicacity and I'm
 * glad they didn't keep the idea a secret.
 *
 * Tim Seaver, Ken Adelman and C. Philip Wood provided bug fixes and/or
 * enhancements to the original distribution.
 *
 * I've hacked up a round-trip-route version of this that works by
 * sending a loose-source-routed udp datagram through the destination
 * back to yourself.  Unfortunately, SO many gateways botch source
 * routing, the thing is almost worthless.  Maybe one day...
 *
 *  -- Van Jacobson (van@ee.lbl.gov)
 *     Tue Dec 20 03:50:13 PST 1988
 */

#undef CONFIG_FEATURE_TRACEROUTE_VERBOSE
//#define CONFIG_FEATURE_TRACEROUTE_VERBOSE
#undef CONFIG_FEATURE_TRACEROUTE_SO_DEBUG   /* not in documentation man */
#undef CONFIG_FEATURE_TRACEROUTE_SOURCE_ROUTE
//#define CONFIG_FEATURE_TRACEROUTE_SOURCE_ROUTE
#undef CONFIG_FEATURE_TRACEROUTE_USE_ICMP
//#define CONFIG_FEATURE_TRACEROUTE_USE_ICMP

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <endian.h>
#include <getopt.h>

#include <sys/param.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "inet_common.h"

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include "busybox.h"


/*
 * Definitions for internet protocol version 4.
 * Per RFC 791, September 1981.
 */
#define IPVERSION       4

/*
 * Overlay for ip header used by other protocols (tcp, udp).
 */
struct ipovly {
	unsigned char  ih_x1[9];               /* (unused) */
	unsigned char  ih_pr;                  /* protocol */
	short   ih_len;                 /* protocol length */
	struct  in_addr ih_src;         /* source internet address */
	struct  in_addr ih_dst;         /* destination internet address */
};

/*
 * UDP kernel structures and variables.
 */
struct  udpiphdr {
	struct  ipovly ui_i;            /* overlaid ip structure */
	struct  udphdr ui_u;            /* udp header */
};
#define ui_next         ui_i.ih_next
#define ui_prev         ui_i.ih_prev
#define ui_x1           ui_i.ih_x1
#define ui_pr           ui_i.ih_pr
#define ui_len          ui_i.ih_len
#define ui_src          ui_i.ih_src
#define ui_dst          ui_i.ih_dst
#define ui_sport        ui_u.uh_sport
#define ui_dport        ui_u.uh_dport
#define ui_ulen         ui_u.uh_ulen
#define ui_sum          ui_u.uh_sum


/* Host name and address list */
struct hostinfo {
	char *name;
	int n;
	u_int32_t *addrs;
};

/* Data section of the probe packet */
struct outdata {
	unsigned char seq;             /* sequence number of this packet */
	unsigned char ttl;             /* ttl packet left with */
	struct timeval tv ATTRIBUTE_PACKED; /* time packet left */
};

struct IFADDRLIST {
	u_int32_t addr;
	char device[sizeof(struct ifreq)];
};


static const char route[] = "/proc/net/route";

/* last inbound (icmp) packet */
static unsigned char  packet[512] ATTRIBUTE_ALIGNED(32);

static struct ip *outip;               /* last output (udp) packet */
static struct udphdr *outudp;          /* last output (udp) packet */
static struct outdata *outdata;        /* last output (udp) packet */

#ifdef CONFIG_FEATURE_TRACEROUTE_USE_ICMP
static struct icmp *outicmp;           /* last output (icmp) packet */
#endif

#ifdef CONFIG_FEATURE_TRACEROUTE_SOURCE_ROUTE
/* Maximum number of gateways (include room for one noop) */
#define NGATEWAYS ((int)((MAX_IPOPTLEN - IPOPT_MINOFF - 1) / sizeof(u_int32_t)))
/* loose source route gateway list (including room for final destination) */
static u_int32_t gwlist[NGATEWAYS + 1];
#endif

static int s;                          /* receive (icmp) socket file descriptor */
static int sndsock;                    /* send (udp/icmp) socket file descriptor */

static struct sockaddr_storage whereto;        /* Who to try to reach */
static struct sockaddr_storage wherefrom;      /* Who we are */
static int packlen;                    /* total length of packet */
static int minpacket;                  /* min ip packet size */
static int maxpacket = 32 * 1024;      /* max ip packet size */
static int pmtu;                       /* Path MTU Discovery (RFC1191) */

static char *hostname;

static u_short ident;
static u_short port = 32768 + 666;     /* start udp dest port # for probe packets */

static int waittime = 5;               /* time to wait for response (in seconds) */
static int nflag;                      /* print addresses numerically */
static int doipcksum = 1;              /* calculate ip checksums by default */

#ifdef CONFIG_FEATURE_TRACEROUTE_SOURCE_ROUTE
static int optlen;                     /* length of ip options */
#else
#define optlen 0
#endif

#ifdef CONFIG_FEATURE_TRACEROUTE_USE_ICMP
static int useicmp;                    /* use icmp echo instead of udp packets */
#endif
#ifdef CONFIG_FEATURE_TRACEROUTE_VERBOSE
static int verbose;
#endif

/*
 * Return the interface list
 */
static int
ifaddrlist(struct IFADDRLIST **ipaddrp)
{
	int fd, nipaddr;
#ifdef HAVE_SOCKADDR_SA_LEN
	int n;
#endif
	struct ifreq *ifrp, *ifend, *ifnext;
	struct sockaddr_in *addr_sin;
	struct IFADDRLIST *al;
	struct ifconf ifc;
	struct ifreq ibuf[(32 * 1024) / sizeof(struct ifreq)], ifr;
	struct IFADDRLIST *st_ifaddrlist;

	fd = bb_xsocket(AF_INET, SOCK_DGRAM, 0);

	ifc.ifc_len = sizeof(ibuf);
	ifc.ifc_buf = (caddr_t)ibuf;

	if (ioctl(fd, SIOCGIFCONF, (char *)&ifc) < 0 ||
	    ifc.ifc_len < sizeof(struct ifreq)) {
		if (errno == EINVAL)
			bb_error_msg_and_die(
			    "SIOCGIFCONF: ifreq struct too small (%d bytes)",
			    (int)sizeof(ibuf));
		else
			bb_perror_msg_and_die("SIOCGIFCONF");
	}
	ifrp = ibuf;
	ifend = (struct ifreq *)((char *)ibuf + ifc.ifc_len);

	nipaddr = 1 + (ifc.ifc_len / sizeof(struct ifreq));
	st_ifaddrlist = xcalloc(nipaddr, sizeof(struct IFADDRLIST));
	al = st_ifaddrlist;
	nipaddr = 0;

	for (; ifrp < ifend; ifrp = ifnext) {
#ifdef HAVE_SOCKADDR_SA_LEN
		n = ifrp->ifr_addr.sa_len + sizeof(ifrp->ifr_name);
		if (n < sizeof(*ifrp))
			ifnext = ifrp + 1;
		else
			ifnext = (struct ifreq *)((char *)ifrp + n);
		if (ifrp->ifr_addr.sa_family != AF_INET)
			continue;
#else
		ifnext = ifrp + 1;
#endif
		/*
		 * Need a template to preserve address info that is
		 * used below to locate the next entry.  (Otherwise,
		 * SIOCGIFFLAGS stomps over it because the requests
		 * are returned in a union.)
		 */
		strncpy(ifr.ifr_name, ifrp->ifr_name, sizeof(ifr.ifr_name));
		if (ioctl(fd, SIOCGIFFLAGS, (char *)&ifr) < 0) {
			if (errno == ENXIO)
				continue;
			bb_perror_msg_and_die("SIOCGIFFLAGS: %.*s",
			    (int)sizeof(ifr.ifr_name), ifr.ifr_name);
		}

		/* Must be up */
		if ((ifr.ifr_flags & IFF_UP) == 0)
			continue;

		safe_strncpy(al->device, ifr.ifr_name, sizeof(ifr.ifr_name) + 1);
#ifdef sun
		/* Ignore sun virtual interfaces */
		if (strchr(al->device, ':') != NULL)
			continue;
#endif
		if (ioctl(fd, SIOCGIFADDR, (char *)&ifr) < 0)
			bb_perror_msg_and_die("SIOCGIFADDR: %s", al->device);

		addr_sin = (struct sockaddr_in *)&ifr.ifr_addr;
		al->addr = addr_sin->sin_addr.s_addr;
		++al;
		++nipaddr;
	}
	if(nipaddr == 0)
	    bb_error_msg_and_die ("Can't find any network interfaces");
	(void)close(fd);

	*ipaddrp = st_ifaddrlist;
	return nipaddr;
}


static void
setsin(struct sockaddr_in *addr_sin, u_int32_t addr)
{
	memset(addr_sin, 0, sizeof(*addr_sin));
#ifdef HAVE_SOCKADDR_SA_LEN
	addr_sin->sin_len = sizeof(*addr_sin);
#endif
	addr_sin->sin_family = AF_INET;
	addr_sin->sin_addr.s_addr = addr;
}


/*
 * Return the source address for the given destination address
 */
static void
findsaddr(const struct sockaddr_in *to, struct sockaddr_in *from)
{
	int i, n;
	FILE *f;
	u_int32_t mask;
	u_int32_t dest, tmask;
	struct IFADDRLIST *al;
	char buf[256], tdevice[256], device[256];

	f = bb_xfopen(route, "r");

	/* Find the appropriate interface */
	n = 0;
	mask = 0;
	device[0] = '\0';
	while (fgets(buf, sizeof(buf), f) != NULL) {
		++n;
		if (n == 1 && strncmp(buf, "Iface", 5) == 0)
			continue;
		if ((i = sscanf(buf, "%255s %x %*s %*s %*s %*s %*s %x",
		    tdevice, &dest, &tmask)) != 3)
			bb_error_msg_and_die ("junk in buffer");
		if ((to->sin_addr.s_addr & tmask) == dest &&
		    (tmask > mask || mask == 0)) {
			mask = tmask;
			strcpy(device, tdevice);
		}
	}
	fclose(f);

	if (device[0] == '\0')
		bb_error_msg_and_die ("Can't find interface");

	/* Get the interface address list */
	n = ifaddrlist(&al);

	/* Find our appropriate source address */
	for (i = n; i > 0; --i, ++al)
		if (strcmp(device, al->device) == 0)
			break;
	if (i <= 0)
		bb_error_msg_and_die("Can't find interface %s", device);

	setsin(from, al->addr);
}

/*
"Usage: %s [-dFIlnrvx] [-g gateway] [-i iface] [-f first_ttl]\n"
"\t[-m max_ttl] [ -p port] [-q nqueries] [-s src_addr] [-t tos]\n"
"\t[-w waittime] [-z pausemsecs] host [packetlen]"

*/

/* String to value with optional min and max. Handles decimal and hex. */
static int
str2val(const char *str, const char *what, int mi, int ma)
{
	const char *cp;
	int val;
	char *ep;

	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
		cp = str + 2;
		val = (int)strtol(cp, &ep, 16);
	} else
		val = (int)strtol(str, &ep, 10);
	if (*ep != '\0') {
		bb_error_msg_and_die("\"%s\" bad value for %s \n", str, what);
	}
	if (val < mi && mi >= 0) {
		if (mi == 0)
			bb_error_msg_and_die("%s must be >= %d\n", what, mi);
		else
			bb_error_msg_and_die("%s must be > %d\n", what, mi - 1);
	}
	if (val > ma && ma >= 0)
		bb_error_msg_and_die("%s must be <= %d\n", what, ma);
	return val;
}


/*
 * Subtract 2 timeval structs:  out = out - in.
 * Out is assumed to be >= in.
 */
static inline void
tvsub(struct timeval *out, struct timeval *in)
{

	if ((out->tv_usec -= in->tv_usec) < 0)   {
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

static int
wait_for_reply(int sock, struct sockaddr_in *fromp, const struct timeval *tp)
{
	fd_set fds;
	struct timeval now, wait;
	struct timezone tz;
	int cc = 0;
	socklen_t fromlen = sizeof(*fromp);

	FD_ZERO(&fds);
	FD_SET(sock, &fds);

	wait.tv_sec = tp->tv_sec + waittime;
	wait.tv_usec = tp->tv_usec;
	(void)gettimeofday(&now, &tz);
	tvsub(&wait, &now);

	if (select(sock + 1, &fds, NULL, NULL, &wait) > 0)
		cc = recvfrom(sock, (char *)packet, sizeof(packet), 0,
			    (struct sockaddr *)fromp, &fromlen);

	return cc;
}

/*
 * Checksum routine for Internet Protocol family headers (C Version)
 */
static u_short
in_cksum(u_short *addr, int len)
{
	int nleft = len;
	u_short *w = addr;
	u_short answer;
	int sum = 0;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1)
		sum += *(unsigned char *)w;

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
	sum += (sum >> 16);                     /* add carry */
	answer = ~sum;                          /* truncate to 16 bits */
	return answer;
}


static void
send_probe(int seq, int ttl, struct timeval *tp)
{
	int cc;
	struct udpiphdr *ui, *oui;
	struct ip tip;

	outip->ip_ttl = ttl;
	outip->ip_id = htons(ident + seq);

	/*
	 * In most cases, the kernel will recalculate the ip checksum.
	 * But we must do it anyway so that the udp checksum comes out
	 * right.
	 */
	if (doipcksum) {
		outip->ip_sum =
		    in_cksum((u_short *)outip, sizeof(*outip) + optlen);
		if (outip->ip_sum == 0)
			outip->ip_sum = 0xffff;
	}

	/* Payload */
	outdata->seq = seq;
	outdata->ttl = ttl;
	memcpy(&outdata->tv, tp, sizeof(outdata->tv));

#ifdef CONFIG_FEATURE_TRACEROUTE_USE_ICMP
	if (useicmp)
		outicmp->icmp_seq = htons(seq);
	else
#endif
		outudp->dest = htons(port + seq);

#ifdef CONFIG_FEATURE_TRACEROUTE_USE_ICMP
	if (useicmp) {
		/* Always calculate checksum for icmp packets */
		outicmp->icmp_cksum = 0;
		outicmp->icmp_cksum = in_cksum((u_short *)outicmp,
		    packlen - (sizeof(*outip) + optlen));
		if (outicmp->icmp_cksum == 0)
			outicmp->icmp_cksum = 0xffff;
	} else
#endif
	       if (doipcksum) {
		/* Checksum (we must save and restore ip header) */
		tip = *outip;
		ui = (struct udpiphdr *)outip;
		oui = (struct udpiphdr *)&tip;
		/* Easier to zero and put back things that are ok */
		memset((char *)ui, 0, sizeof(ui->ui_i));
		ui->ui_src = oui->ui_src;
		ui->ui_dst = oui->ui_dst;
		ui->ui_pr = oui->ui_pr;
		ui->ui_len = outudp->len;
		outudp->check = 0;
		outudp->check = in_cksum((u_short *)ui, packlen);
		if (outudp->check == 0)
			outudp->check = 0xffff;
		*outip = tip;
	}

#ifdef CONFIG_FEATURE_TRACEROUTE_VERBOSE
	/* XXX undocumented debugging hack */
	if (verbose > 1) {
		const u_short *sp;
		int nshorts, i;

		sp = (u_short *)outip;
		nshorts = (u_int)packlen / sizeof(u_short);
		i = 0;
		printf("[ %d bytes", packlen);
		while (--nshorts >= 0) {
			if ((i++ % 8) == 0)
				printf("\n\t");
			printf(" %04x", ntohs(*sp));
			sp++;
		}
		if (packlen & 1) {
			if ((i % 8) == 0)
				printf("\n\t");
			printf(" %02x", *(unsigned char *)sp);
		}
		printf("]\n");
	}
#endif

#if !defined(IP_HDRINCL) && defined(IP_TTL)
	if (setsockopt(sndsock, IPPROTO_IP, IP_TTL,
	    (char *)&ttl, sizeof(ttl)) < 0) {
		bb_perror_msg_and_die("setsockopt ttl %d", ttl);
	}
#endif

	cc = sendto(sndsock, (char *)outip,
	    packlen, 0, (struct sockaddr *)&whereto, sizeof(whereto));
	if (cc < 0 || cc != packlen)  {
		if (cc < 0)
			bb_perror_msg_and_die("sendto");
		printf("%s: wrote %s %d chars, ret=%d\n",
		    bb_applet_name, hostname, packlen, cc);
		(void)fflush(stdout);
	}
}

static inline double
deltaT(struct timeval *t1p, struct timeval *t2p)
{
	double dt;

	dt = (double)(t2p->tv_sec - t1p->tv_sec) * 1000.0 +
	     (double)(t2p->tv_usec - t1p->tv_usec) / 1000.0;
	return dt;
}

#ifdef CONFIG_FEATURE_TRACEROUTE_VERBOSE
/*
 * Convert an ICMP "type" field to a printable string.
 */
static inline const char *
pr_type(unsigned char t)
{
	static const char * const ttab[] = {
	"Echo Reply",   "ICMP 1",       "ICMP 2",       "Dest Unreachable",
	"Source Quench", "Redirect",    "ICMP 6",       "ICMP 7",
	"Echo",         "Router Advert", "Router Solicit", "Time Exceeded",
	"Param Problem", "Timestamp",   "Timestamp Reply", "Info Request",
	"Info Reply",   "Mask Request", "Mask Reply"
	};

	if (t > 18)
		return "OUT-OF-RANGE";

	return ttab[t];
}
#endif

static int
packet_ok(unsigned char *buf, int cc, struct sockaddr_in *from, int seq)
{
	struct icmp *icp;
	unsigned char type, code;
	int hlen;
	struct ip *ip;

	ip = (struct ip *) buf;
	hlen = ip->ip_hl << 2;
	if (cc < hlen + ICMP_MINLEN) {
#ifdef CONFIG_FEATURE_TRACEROUTE_VERBOSE
		if (verbose)
			printf("packet too short (%d bytes) from %s\n", cc,
				inet_ntoa(from->sin_addr));
#endif
		return 0;
	}
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
	type = icp->icmp_type;
	code = icp->icmp_code;
	/* Path MTU Discovery (RFC1191) */
	if (code != ICMP_UNREACH_NEEDFRAG)
		pmtu = 0;
	else {
		pmtu = ntohs(icp->icmp_nextmtu);
	}
	if ((type == ICMP_TIMXCEED && code == ICMP_TIMXCEED_INTRANS) ||
	    type == ICMP_UNREACH || type == ICMP_ECHOREPLY) {
		struct ip *hip;
		struct udphdr *up;

		hip = &icp->icmp_ip;
		hlen = hip->ip_hl << 2;
#ifdef CONFIG_FEATURE_TRACEROUTE_USE_ICMP
		if (useicmp) {
			struct icmp *hicmp;

			/* XXX */
			if (type == ICMP_ECHOREPLY &&
			    icp->icmp_id == htons(ident) &&
			    icp->icmp_seq == htons(seq))
				return -2;

			hicmp = (struct icmp *)((unsigned char *)hip + hlen);
			/* XXX 8 is a magic number */
			if (hlen + 8 <= cc &&
			    hip->ip_p == IPPROTO_ICMP &&
			    hicmp->icmp_id == htons(ident) &&
			    hicmp->icmp_seq == htons(seq))
				return (type == ICMP_TIMXCEED ? -1 : code + 1);
		} else
#endif
		      {
			up = (struct udphdr *)((unsigned char *)hip + hlen);
			/* XXX 8 is a magic number */
			if (hlen + 12 <= cc &&
			    hip->ip_p == IPPROTO_UDP &&
			    up->source == htons(ident) &&
			    up->dest == htons(port + seq))
				return (type == ICMP_TIMXCEED ? -1 : code + 1);
		}
	}
#ifdef CONFIG_FEATURE_TRACEROUTE_VERBOSE
	if (verbose) {
		int i;
		u_int32_t *lp = (u_int32_t *)&icp->icmp_ip;

		printf("\n%d bytes from %s to "
		       "%s: icmp type %d (%s) code %d\n",
		    cc, inet_ntoa(from->sin_addr),
		    inet_ntoa(ip->ip_dst), type, pr_type(type), icp->icmp_code);
		for (i = 4; i < cc ; i += sizeof(*lp))
			printf("%2d: x%8.8x\n", i, *lp++);
	}
#endif
	return 0;
}


/*
 * Construct an Internet address representation.
 * If the nflag has been supplied, give
 * numeric value, otherwise try for symbolic name.
 */
static inline void
inetname(struct sockaddr_in *from)
{
	const char *n = NULL;
	const char *ina;
	char name[257];

	if (!nflag && from->sin_addr.s_addr != INADDR_ANY) {
		if(INET_rresolve(name, sizeof(name), from, 0x4000, 0xffffffff) >= 0)
			n = name;
	}
	ina = inet_ntoa(from->sin_addr);
	if (nflag)
		printf(" %s", ina);
	else
		printf(" %s (%s)", (n ? n : ina), ina);
}

static inline void
print(unsigned char *buf, int cc, struct sockaddr_in *from)
{
	struct ip *ip;
	int hlen;

	ip = (struct ip *) buf;
	hlen = ip->ip_hl << 2;
	cc -= hlen;

	inetname(from);
#ifdef CONFIG_FEATURE_TRACEROUTE_VERBOSE
	if (verbose)
		printf(" %d bytes to %s", cc, inet_ntoa (ip->ip_dst));
#endif
}


static struct hostinfo *
gethostinfo(const char *host)
{
	int n;
	struct hostent *hp;
	struct hostinfo *hi;
	char **p;
	u_int32_t addr, *ap;

	hi = xcalloc(1, sizeof(*hi));
	addr = inet_addr(host);
	if ((int32_t)addr != -1) {
		hi->name = bb_xstrdup(host);
		hi->n = 1;
		hi->addrs = xcalloc(1, sizeof(hi->addrs[0]));
		hi->addrs[0] = addr;
		return hi;
	}

	hp = xgethostbyname(host);
	if (hp->h_addrtype != AF_INET || hp->h_length != 4)
		bb_perror_msg_and_die("bad host %s", host);
	hi->name = bb_xstrdup(hp->h_name);
	for (n = 0, p = hp->h_addr_list; *p != NULL; ++n, ++p)
		continue;
	hi->n = n;
	hi->addrs = xcalloc(n, sizeof(hi->addrs[0]));
	for (ap = hi->addrs, p = hp->h_addr_list; *p != NULL; ++ap, ++p)
		memcpy(ap, *p, sizeof(*ap));
	return hi;
}

static void
freehostinfo(struct hostinfo *hi)
{
	free(hi->name);
	hi->name = NULL;
	free((char *)hi->addrs);
	free((char *)hi);
}

#ifdef CONFIG_FEATURE_TRACEROUTE_SOURCE_ROUTE
static void
getaddr(u_int32_t *ap, const char *host)
{
	struct hostinfo *hi;

	hi = gethostinfo(host);
	*ap = hi->addrs[0];
	freehostinfo(hi);
}
#endif


int
traceroute_main(int argc, char *argv[])
{
	int code, n;
	char *cp;
	unsigned char *outp;
	u_int32_t *ap;
	struct sockaddr_in *from = (struct sockaddr_in *)&wherefrom;
	struct sockaddr_in *to = (struct sockaddr_in *)&whereto;
	struct hostinfo *hi;
	int on = 1;
	struct protoent *pe;
	int ttl, probe, i;
	int seq = 0;
	int tos = 0;
	char *tos_str = NULL;
	char *source = NULL;
	unsigned long op;

#ifdef CONFIG_FEATURE_TRACEROUTE_SOURCE_ROUTE
	int lsrr = 0;
#endif
	u_short off = 0;
	struct IFADDRLIST *al;
	char *device = NULL;
	int max_ttl = 30;
	char *max_ttl_str = NULL;
	char *port_str = NULL;
	int nprobes = 3;
	char *nprobes_str = NULL;
	char *waittime_str = NULL;
	u_int pausemsecs = 0;
	char *pausemsecs_str = NULL;
	int first_ttl = 1;
	char *first_ttl_str = NULL;
#ifdef CONFIG_FEATURE_TRACEROUTE_SOURCE_ROUTE
	llist_t *sourse_route_list = NULL;
#endif

	opterr = 0;
#ifdef CONFIG_FEATURE_TRACEROUTE_SOURCE_ROUTE
	bb_opt_complementally = "x-x:g::";
#else
	bb_opt_complementally = "x-x";
#endif

	op = bb_getopt_ulflags(argc, argv, "FIlnrdvxt:i:m:p:q:s:w:z:f:"
#define USAGE_OP_DONT_FRAGMNT (1<<0)    /* F  */
#define USAGE_OP_USE_ICMP     (1<<1)    /* I  */
#define USAGE_OP_TTL_FLAG     (1<<2)    /* l  */
#define USAGE_OP_ADDR_NUM     (1<<3)    /* n  */
#define USAGE_OP_BYPASS_ROUTE (1<<4)    /* r  */
#define USAGE_OP_DEBUG        (1<<5)    /* d */
#define USAGE_OP_VERBOSE      (1<<6)    /* v */
#define USAGE_OP_IP_CHKSUM    (1<<7)    /* x */

#ifdef CONFIG_FEATURE_TRACEROUTE_SOURCE_ROUTE
					"g:"
#endif
	, &tos_str, &device, &max_ttl_str, &port_str, &nprobes_str,
	&source, &waittime_str, &pausemsecs_str, &first_ttl_str
#ifdef CONFIG_FEATURE_TRACEROUTE_SOURCE_ROUTE
	, &sourse_route_list
#endif
	);

	if(op & USAGE_OP_DONT_FRAGMNT)
		off = IP_DF;
#ifdef CONFIG_FEATURE_TRACEROUTE_USE_ICMP
	useicmp = op & USAGE_OP_USE_ICMP;
#endif
	nflag = op & USAGE_OP_ADDR_NUM;
#ifdef CONFIG_FEATURE_TRACEROUTE_VERBOSE
	verbose = op &  USAGE_OP_VERBOSE;
#endif
	if(op & USAGE_OP_IP_CHKSUM) {
		doipcksum = 0;
		bb_error_msg("Warning: ip checksums disabled");
	}
	if (tos_str)
		tos = str2val(tos_str, "tos", 0, 255);
	if(max_ttl_str)
		max_ttl = str2val(max_ttl_str, "max ttl", 1, 255);
	if(port_str)
		port = (u_short)str2val(port_str, "port", 1, (1 << 16) - 1);
	if(nprobes_str)
		nprobes = str2val(optarg, "nprobes", 1, -1);
	if(source) {
	    /*
	     * set the ip source address of the outbound
	     * probe (e.g., on a multi-homed host).
	     */
	     if (getuid()) bb_error_msg_and_die("-s %s: Permission denied", source);
	}
	if(waittime_str)
		waittime = str2val(waittime_str, "wait time", 2, 24 * 60 * 60);
	if(pausemsecs_str)
		pausemsecs = str2val(pausemsecs_str, "pause msecs", 0, 60 * 60 * 1000);
	if(first_ttl_str)
		first_ttl = str2val(first_ttl_str, "first ttl", 1, 255);

#ifdef CONFIG_FEATURE_TRACEROUTE_SOURCE_ROUTE
	if(sourse_route_list) {
		llist_t *l_sr;

		for(l_sr = sourse_route_list; l_sr; ) {
			if (lsrr >= NGATEWAYS)
				bb_error_msg_and_die("No more than %d gateways", NGATEWAYS);
			getaddr(gwlist + lsrr, l_sr->data);
			++lsrr;
			l_sr = l_sr->link;
			free(sourse_route_list);
			sourse_route_list = l_sr;
		}
		optlen = (lsrr + 1) * sizeof(gwlist[0]);
	}
#endif

	if (first_ttl > max_ttl) {
		bb_error_msg_and_die(
		    "first ttl (%d) may not be greater than max ttl (%d)",
		    first_ttl, max_ttl);
	}

	minpacket = sizeof(*outip) + sizeof(*outdata) + optlen;

#ifdef CONFIG_FEATURE_TRACEROUTE_USE_ICMP
	if (useicmp)
		minpacket += 8;                 /* XXX magic number */
	else
#endif
		minpacket += sizeof(*outudp);
	packlen = minpacket;                    /* minimum sized packet */

	/* Process destination and optional packet size */
	switch (argc - optind) {

	case 2:
		packlen = str2val(argv[optind + 1],
		    "packet length", minpacket, maxpacket);
		/* Fall through */

	case 1:
		hostname = argv[optind];
		hi = gethostinfo(hostname);
		setsin(to, hi->addrs[0]);
		if (hi->n > 1)
			bb_error_msg(
		    "Warning: %s has multiple addresses; using %s",
				hostname, inet_ntoa(to->sin_addr));
		hostname = hi->name;
		hi->name = NULL;
		freehostinfo(hi);
		break;

	default:
		bb_show_usage();
	}

	cp = "icmp";
	if ((pe = getprotobyname(cp)) == NULL)
		bb_perror_msg_and_die("unknown protocol %s", cp);

	/* Insure the socket fds won't be 0, 1 or 2 */
	do n = bb_xopen(bb_dev_null, O_RDONLY); while (n < 2);
	if (n > 2)
		close(n);

	s = bb_xsocket(AF_INET, SOCK_RAW, pe->p_proto);

#ifdef CONFIG_FEATURE_TRACEROUTE_SO_DEBUG
	if (op & USAGE_OP_DEBUG)
		(void)setsockopt(s, SOL_SOCKET, SO_DEBUG, (char *)&on,
		    sizeof(on));
#endif
	if (op & USAGE_OP_BYPASS_ROUTE)
		(void)setsockopt(s, SOL_SOCKET, SO_DONTROUTE, (char *)&on,
		    sizeof(on));

	sndsock = bb_xsocket(AF_INET, SOCK_RAW, IPPROTO_RAW);

#ifdef CONFIG_FEATURE_TRACEROUTE_SOURCE_ROUTE
#if defined(IP_OPTIONS)
	if (lsrr > 0) {
		unsigned char optlist[MAX_IPOPTLEN];

		cp = "ip";
		if ((pe = getprotobyname(cp)) == NULL)
			bb_perror_msg_and_die("unknown protocol");

		/* final hop */
		gwlist[lsrr] = to->sin_addr.s_addr;
		++lsrr;

		/* force 4 byte alignment */
		optlist[0] = IPOPT_NOP;
		/* loose source route option */
		optlist[1] = IPOPT_LSRR;
		i = lsrr * sizeof(gwlist[0]);
		optlist[2] = i + 3;
		/* Pointer to LSRR addresses */
		optlist[3] = IPOPT_MINOFF;
		memcpy(optlist + 4, gwlist, i);

		if ((setsockopt(sndsock, pe->p_proto, IP_OPTIONS,
		    (char *)optlist, i + sizeof(gwlist[0]))) < 0) {
			bb_perror_msg_and_die("IP_OPTIONS");
		    }
	}
#endif /* IP_OPTIONS */
#endif /* CONFIG_FEATURE_TRACEROUTE_SOURCE_ROUTE */

#ifdef SO_SNDBUF
	if (setsockopt(sndsock, SOL_SOCKET, SO_SNDBUF, (char *)&packlen,
	    sizeof(packlen)) < 0) {
		bb_perror_msg_and_die("SO_SNDBUF");
	}
#endif
#ifdef IP_HDRINCL
	if (setsockopt(sndsock, IPPROTO_IP, IP_HDRINCL, (char *)&on,
	    sizeof(on)) < 0 && errno != ENOPROTOOPT) {
		bb_perror_msg_and_die("IP_HDRINCL");
	}
#else
#ifdef IP_TOS
	if (tos_str && setsockopt(sndsock, IPPROTO_IP, IP_TOS,
	    (char *)&tos, sizeof(tos)) < 0) {
		bb_perror_msg_and_die("setsockopt tos %d", tos);
	}
#endif
#endif
#ifdef CONFIG_FEATURE_TRACEROUTE_SO_DEBUG
	if (op & USAGE_OP_DEBUG)
		(void)setsockopt(sndsock, SOL_SOCKET, SO_DEBUG, (char *)&on,
		    sizeof(on));
#endif
	if (op & USAGE_OP_BYPASS_ROUTE)
		(void)setsockopt(sndsock, SOL_SOCKET, SO_DONTROUTE, (char *)&on,
		    sizeof(on));

	/* Revert to non-privileged user after opening sockets */
	xsetgid(getgid());
	xsetuid(getuid());

	outip = (struct ip *)xcalloc(1, (unsigned)packlen);

	outip->ip_v = IPVERSION;
	if (tos_str)
		outip->ip_tos = tos;
	outip->ip_len = htons(packlen);
	outip->ip_off = htons(off);
	outp = (unsigned char *)(outip + 1);
	outip->ip_dst = to->sin_addr;

	outip->ip_hl = (outp - (unsigned char *)outip) >> 2;
	ident = (getpid() & 0xffff) | 0x8000;
#ifdef CONFIG_FEATURE_TRACEROUTE_USE_ICMP
	if (useicmp) {
		outip->ip_p = IPPROTO_ICMP;

		outicmp = (struct icmp *)outp;
		outicmp->icmp_type = ICMP_ECHO;
		outicmp->icmp_id = htons(ident);

		outdata = (struct outdata *)(outp + 8); /* XXX magic number */
	} else
#endif
	       {
		outip->ip_p = IPPROTO_UDP;

		outudp = (struct udphdr *)outp;
		outudp->source = htons(ident);
		outudp->len =
		    htons((u_short)(packlen - (sizeof(*outip) + optlen)));
		outdata = (struct outdata *)(outudp + 1);
	}

	/* Get the interface address list */
	n = ifaddrlist(&al);

	/* Look for a specific device */
	if (device != NULL) {
		for (i = n; i > 0; --i, ++al)
			if (strcmp(device, al->device) == 0)
				break;
		if (i <= 0) {
			bb_error_msg_and_die("Can't find interface %s", device);
		}
	}

	/* Determine our source address */
	if (source == NULL) {
		/*
		 * If a device was specified, use the interface address.
		 * Otherwise, try to determine our source address.
		 */
		if (device != NULL)
			setsin(from, al->addr);
		findsaddr(to, from);
	} else {
		hi = gethostinfo(source);
		source = hi->name;
		hi->name = NULL;
		/*
		 * If the device was specified make sure it
		 * corresponds to the source address specified.
		 * Otherwise, use the first address (and warn if
		 * there are more than one).
		 */
		if (device != NULL) {
			for (i = hi->n, ap = hi->addrs; i > 0; --i, ++ap)
				if (*ap == al->addr)
					break;
			if (i <= 0) {
				bb_error_msg_and_die(
				    "%s is not on interface %s",
				    source, device);
			}
			setsin(from, *ap);
		} else {
			setsin(from, hi->addrs[0]);
			if (hi->n > 1)
				bb_error_msg(
			"Warning: %s has multiple addresses; using %s",
				    source, inet_ntoa(from->sin_addr));
		}
		freehostinfo(hi);
	}

	outip->ip_src = from->sin_addr;
#ifndef IP_HDRINCL
	bb_xbind(sndsock, (struct sockaddr *)from, sizeof(*from));
#endif

	fprintf(stderr, "traceroute to %s (%s)", hostname, inet_ntoa(to->sin_addr));
	if (source)
		fprintf(stderr, " from %s", source);
	fprintf(stderr, ", %d hops max, %d byte packets\n", max_ttl, packlen);
	(void)fflush(stderr);

	for (ttl = first_ttl; ttl <= max_ttl; ++ttl) {
		u_int32_t lastaddr = 0;
		int gotlastaddr = 0;
		int got_there = 0;
		int unreachable = 0;
		int sentfirst = 0;

		printf("%2d ", ttl);
		for (probe = 0; probe < nprobes; ++probe) {
			int cc;
			struct timeval t1, t2;
			struct timezone tz;
			struct ip *ip;

			if (sentfirst && pausemsecs > 0)
				usleep(pausemsecs * 1000);
			(void)gettimeofday(&t1, &tz);
			send_probe(++seq, ttl, &t1);
			++sentfirst;
			while ((cc = wait_for_reply(s, from, &t1)) != 0) {
				(void)gettimeofday(&t2, &tz);
				i = packet_ok(packet, cc, from, seq);
				/* Skip short packet */
				if (i == 0)
					continue;
				if (!gotlastaddr ||
				    from->sin_addr.s_addr != lastaddr) {
					print(packet, cc, from);
					lastaddr = from->sin_addr.s_addr;
					++gotlastaddr;
				}
				printf("  %.3f ms", deltaT(&t1, &t2));
				ip = (struct ip *)packet;
				if (op & USAGE_OP_TTL_FLAG)
					printf(" (%d)", ip->ip_ttl);
				if (i == -2) {
					if (ip->ip_ttl <= 1)
						printf(" !");
					++got_there;
					break;
				}
				/* time exceeded in transit */
				if (i == -1)
					break;
				code = i - 1;
				switch (code) {

				case ICMP_UNREACH_PORT:
					if (ip->ip_ttl <= 1)
						printf(" !");
					++got_there;
					break;

				case ICMP_UNREACH_NET:
					++unreachable;
					printf(" !N");
					break;

				case ICMP_UNREACH_HOST:
					++unreachable;
					printf(" !H");
					break;

				case ICMP_UNREACH_PROTOCOL:
					++got_there;
					printf(" !P");
					break;

				case ICMP_UNREACH_NEEDFRAG:
					++unreachable;
					printf(" !F-%d", pmtu);
					break;

				case ICMP_UNREACH_SRCFAIL:
					++unreachable;
					printf(" !S");
					break;

				case ICMP_UNREACH_FILTER_PROHIB:
				case ICMP_UNREACH_NET_PROHIB:   /* misuse */
					++unreachable;
					printf(" !A");
					break;

				case ICMP_UNREACH_HOST_PROHIB:
					++unreachable;
					printf(" !C");
					break;

				case ICMP_UNREACH_HOST_PRECEDENCE:
					++unreachable;
					printf(" !V");
					break;

				case ICMP_UNREACH_PRECEDENCE_CUTOFF:
					++unreachable;
					printf(" !C");
					break;

				case ICMP_UNREACH_NET_UNKNOWN:
				case ICMP_UNREACH_HOST_UNKNOWN:
					++unreachable;
					printf(" !U");
					break;

				case ICMP_UNREACH_ISOLATED:
					++unreachable;
					printf(" !I");
					break;

				case ICMP_UNREACH_TOSNET:
				case ICMP_UNREACH_TOSHOST:
					++unreachable;
					printf(" !T");
					break;

				default:
					++unreachable;
					printf(" !<%d>", code);
					break;
				}
				break;
			}
			if (cc == 0)
				printf(" *");
			(void)fflush(stdout);
		}
		putchar('\n');
		if (got_there ||
		    (unreachable > 0 && unreachable >= nprobes - 1))
			break;
	}
	return 0;
}
