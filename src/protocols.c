/* Etherape
 * Copyright (C) 2000 Juan Toledo
 * $Id$
 *
 * This file is mostly a rehash of algorithms found in
 * packet-*. of ethereal
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "globals.h"
#include "protocols.h"
#include <ctype.h>
#include <string.h>

static GString *prot;
static const guint8 *packet;

gchar *
get_packet_prot (const guint8 * p)
{
  gchar **tokens = NULL;
  gchar *top_prot = NULL;
  gchar *str = NULL;

  guint i = 0;

  g_assert (p != NULL);

  if (prot)
    {
      g_string_free (prot, TRUE);
      prot = NULL;
    }
  prot = g_string_new ("");

  packet = p;

  switch (linktype)
    {
    case L_EN10MB:
      get_eth_type ();
      break;
    case L_FDDI:
      /* prot = g_string_new ("LLC"); */
      get_fddi_type ();
      break;
    case L_RAW:		/* Both for PPP and SLIP */
      prot = g_string_append (prot, "RAW/IP");
      get_ip ();
      break;
    case L_NULL:
      prot = g_string_append (prot, "NULL/IP");
      get_ip ();
      break;
    default:
      break;
    }

  /* TODO I think this is a serious waste of CPU and memory.
     * I think I should look at other ways of dealing with not recognized
     * protocols other than just creating entries saying UNKNOWN */
  /* I think I'll do this by changing the global protocols into
   * a GArray */

  if (prot)
    {
      tokens = g_strsplit (prot->str, "/", 0);
      while ((tokens[i] != NULL) && i <= STACK_SIZE)
	i++;
      g_strfreev (tokens);
      tokens = NULL;
    }
  for (; i <= STACK_SIZE; i++)
    prot = g_string_append (prot, "/UNKNOWN");

  tokens = g_strsplit (prot->str, "/", 0);
  for (i = 0; strcmp (tokens[i], "UNKNOWN"); i++)
    top_prot = tokens[i];

  g_assert (top_prot != NULL);
  str = g_strdup_printf ("%s/", top_prot);
  prot = g_string_prepend (prot, str);
  g_free (str);
  str = NULL;
  g_strfreev (tokens);
  tokens = NULL;

  /* g_message ("Protocol stack is %s", prot->str); */
  return prot->str;
}				/* get_packet_prot */


static void
get_eth_type (void)
{
  etype_t etype;
  ethhdrtype_t ethhdr_type = ETHERNET_II;	/* Default */

  etype = pntohs (&packet[12]);


  if (etype <= IEEE_802_3_MAX_LEN)
    {

      /* Is there an 802.2 layer? I can tell by looking at the first 2
       *        bytes after the 802.3 header. If they are 0xffff, then what
       *        follows the 802.3 header is an IPX payload, meaning no 802.2.
       *        (IPX/SPX is they only thing that can be contained inside a
       *        straight 802.3 packet). A non-0xffff value means that there's an
       *        802.2 layer inside the 802.3 layer */
      if (packet[14] == 0xff && packet[15] == 0xff)
	{
	  ethhdr_type = ETHERNET_802_3;
	}
      else
	{
	  ethhdr_type = ETHERNET_802_2;
	}

      /* Oh, yuck.  Cisco ISL frames require special interpretation of the
       *        destination address field; fortunately, they can be recognized by
       *        checking the first 5 octets of the destination address, which are
       *        01-00-0C-00-00 for ISL frames. */
      if (packet[0] == 0x01 && packet[1] == 0x00 && packet[2] == 0x0C
	  && packet[3] == 0x00 && packet[4] == 0x00)
	{
	  /* TODO Analyze ISL frames */
	  prot = g_string_new ("ISL");
	  return;
	}
    }

  if (ethhdr_type == ETHERNET_802_3)
    {
      prot = g_string_new ("802.3");
      return;
    }

  if (ethhdr_type == ETHERNET_802_2)
    {
      prot = g_string_new ("802.2");
      return;
    }

  /* Else, it's ETHERNET_II */

  prot = g_string_new ("ETH_II");
  get_eth_II (etype);
  return;

}				/* get_eth_type */

static void
get_fddi_type (void)
{
  prot = g_string_new ("LLC");
  /* Ok, this is only temporary while I truly dissect LLC 
   * and fddi */
  if ((packet[19] == 0x08) && (packet[20] == 0x00))
    {
      prot = g_string_append (prot, "/IP");
      get_ip ();
    }

}				/* get_fdd_type */
static void
get_eth_II (etype_t etype)
{
  switch (etype)
    {
    case ETHERTYPE_IP:
      prot = g_string_append (prot, "/IP");
      get_ip ();
      break;
    case ETHERTYPE_ARP:
      prot = g_string_append (prot, "/ARP");
      break;
    case ETHERTYPE_IPv6:
      prot = g_string_append (prot, "/IPv6");
      break;
    case ETHERTYPE_X25L3:
      prot = g_string_append (prot, "/X25L3");
      break;
    case ETHERTYPE_REVARP:
      prot = g_string_append (prot, "/REVARP");
      break;
    case ETHERTYPE_ATALK:
      prot = g_string_append (prot, "/ATALK");
      break;
    case ETHERTYPE_AARP:
      prot = g_string_append (prot, "/AARP");
      break;
    case ETHERTYPE_IPX:
      prot = g_string_append (prot, "/IPX");
      break;
    case ETHERTYPE_VINES:
      prot = g_string_append (prot, "/VINES");
      break;
    case ETHERTYPE_TRAIN:
      prot = g_string_append (prot, "/TRAIN");
      break;
    case ETHERTYPE_LOOP:
      prot = g_string_append (prot, "/LOOP");
      break;
    case ETHERTYPE_PPPOED:
      prot = g_string_append (prot, "/PPPOED");
      break;
    case ETHERTYPE_PPPOES:
      prot = g_string_append (prot, "/PPPOES");
      break;
    case ETHERTYPE_VLAN:
      prot = g_string_append (prot, "/VLAN");
      break;
    case ETHERTYPE_SNMP:
      prot = g_string_append (prot, "/SNMP");
      break;
    case ETHERTYPE_UNK:
    default:
      prot = g_string_append (prot, "/ETH_UNKNOWN");
    }

  return;
}				/* get_eth_II */

static void
get_ip (void)
{
  guint16 fragment_offset;
  iptype_t ip_type;
  ip_type = packet[l3_offset + 9];
  fragment_offset = pntohs (packet + l3_offset + 6);
  fragment_offset &= 0x0fff;

  switch (ip_type)
    {
    case IP_PROTO_ICMP:
      prot = g_string_append (prot, "/ICMP");
      break;
    case IP_PROTO_TCP:
      if (fragment_offset)
	prot = g_string_append (prot, "/TCP_FRAGMENT");
      else
	{
	  prot = g_string_append (prot, "/TCP");
	  get_tcp ();
	}
      break;
    case IP_PROTO_UDP:
      if (fragment_offset)
	prot = g_string_append (prot, "/UDP_FRAGMENT");
      else
	{
	  prot = g_string_append (prot, "/UDP");
	  get_udp ();
	}
      break;
    case IP_PROTO_IGMP:
      prot = g_string_append (prot, "/IGMP");
      break;
    case IP_PROTO_GGP:
      prot = g_string_append (prot, "/GGP");
      break;
    case IP_PROTO_IPIP:
      prot = g_string_append (prot, "/IPIP");
      break;
#if 0				/* TODO How come IPIP and IPV4 share the same number? */
    case IP_PROTO_IPV4:
      prot = g_string_append (prot, "/IPV4");
      break;
#endif
    case IP_PROTO_EGP:
      prot = g_string_append (prot, "/EGP");
      break;
    case IP_PROTO_PUP:
      prot = g_string_append (prot, "/PUP");
      break;
    case IP_PROTO_IDP:
      prot = g_string_append (prot, "/IDP");
      break;
    case IP_PROTO_TP:
      prot = g_string_append (prot, "/TP");
      break;
    case IP_PROTO_IPV6:
      prot = g_string_append (prot, "/IPV6");
      break;
    case IP_PROTO_ROUTING:
      prot = g_string_append (prot, "/ROUTING");
      break;
    case IP_PROTO_FRAGMENT:
      prot = g_string_append (prot, "/FRAGMENT");
      break;
    case IP_PROTO_RSVP:
      prot = g_string_append (prot, "/RSVP");
      break;
    case IP_PROTO_GRE:
      prot = g_string_append (prot, "/GRE");
      break;
    case IP_PROTO_ESP:
      prot = g_string_append (prot, "/ESP");
      break;
    case IP_PROTO_AH:
      prot = g_string_append (prot, "/AH");
      break;
    case IP_PROTO_ICMPV6:
      prot = g_string_append (prot, "/ICPMPV6");
      break;
    case IP_PROTO_NONE:
      prot = g_string_append (prot, "/NONE");
      break;
    case IP_PROTO_DSTOPTS:
      prot = g_string_append (prot, "/DSTOPTS");
      break;
    case IP_PROTO_VINES:
      prot = g_string_append (prot, "/VINES");
      break;
    case IP_PROTO_EIGRP:
      prot = g_string_append (prot, "/EIGRP");
      break;
    case IP_PROTO_OSPF:
      prot = g_string_append (prot, "/OSPF");
      break;
    case IP_PROTO_ENCAP:
      prot = g_string_append (prot, "/ENCAP");
      break;
    case IP_PROTO_PIM:
      prot = g_string_append (prot, "/PIM");
      break;
    case IP_PROTO_IPCOMP:
      prot = g_string_append (prot, "/IPCOMP");
      break;
    case IP_PROTO_VRRP:
      prot = g_string_append (prot, "/VRRP");
      break;
    default:
      prot = g_string_append (prot, "/IP_UNKNOWN");
    }

  return;
}

static void
get_tcp (void)
{
  tcp_service_t *service;
  tcp_type_t src_port, dst_port;
  gchar *str;

  if (!tcp_services)
    load_services ();

  src_port = pntohs (packet + l3_offset + 20);
  dst_port = pntohs (packet + l3_offset + 22);

  if (!(service = g_tree_lookup (tcp_services, &src_port)))
    service = g_tree_lookup (tcp_services, &dst_port);

  if (!service)
    {
      prot = g_string_append (prot, "/TCP_UNKNOWN");
      return;
    }
  str = g_strdup_printf ("/%s", service->name);
  prot = g_string_append (prot, str);
  g_free (str);
  str = NULL;
  return;
}				/* get_tcp */

static void
get_udp (void)
{
  udp_service_t *service;
  udp_type_t src_port, dst_port;
  gchar *str;

  if (!udp_services)
    load_services ();

  src_port = pntohs (packet + l3_offset + 20);
  dst_port = pntohs (packet + l3_offset + 22);

  if (!(service = g_tree_lookup (udp_services, &src_port)))
    service = g_tree_lookup (udp_services, &dst_port);

  if (!service)
    {
      prot = g_string_append (prot, "/UDP_UNKNOWN");
      return;
    }
  str = g_strdup_printf ("/%s", service->name);
  prot = g_string_append (prot, str);
  g_free (str);
  str = NULL;
  return;
}				/* get_udp */

/* Comparison function to sort tcp services port number */
static gint
tcp_compare (gconstpointer a, gconstpointer b)
{
  tcp_type_t port_a, port_b;

  port_a = *(tcp_type_t *) a;
  port_b = *(tcp_type_t *) b;

  if (port_a > port_b)
    return 1;
  if (port_a < port_b)
    return -1;
  return 0;
}				/* tcp_compare */

/* Comparison function to sort udp services port number */
static gint
udp_compare (gconstpointer a, gconstpointer b)
{
  udp_type_t port_a, port_b;

  port_a = *(tcp_type_t *) a;
  port_b = *(tcp_type_t *) b;

  if (port_a > port_b)
    return 1;
  if (port_a < port_b)
    return -1;
  return 0;
}				/* udp_compare */

static void
load_services (void)
{
  FILE *services = NULL;
  gchar *line;
  gchar **t1 = NULL, **t2 = NULL;
  gchar *str;
  tcp_service_t *tcp_service;
  udp_service_t *udp_service;
  guint i;
  char filename[PATH_MAX];

  tcp_type_t port_number;	/* udp and tcp are the same */

  strcpy (filename, CONFDIR "/services");
  if (!(services = fopen (filename, "r")))
    {
      strcpy (filename, "/etc/services");
      if (!(services = fopen (filename, "r")))
	{
	  g_my_critical (_
			 ("Failed to open %s. No TCP or UDP services will be recognized"),
			 filename);
	  return;
	}
    }

  g_my_info (_("Reading TCP and UDP services from %s"), filename);

  tcp_services = g_tree_new ((GCompareFunc) tcp_compare);
  udp_services = g_tree_new ((GCompareFunc) udp_compare);

  line = g_malloc (LINESIZE);

  while (fgets (line, LINESIZE, services))
    {
      if (line[0] != '#' && line[0] != ' ' && line[0] != '\n'
	  && line[0] != '\t')
	{
	  gboolean error = FALSE;

	  if (!g_strdelimit (line, " \t\n", ' '))
	    error = TRUE;

	  if (error || !(t1 = g_strsplit (line, " ", 0)))
	    error = TRUE;
	  if (!error && t1[0])
	    g_strup (t1[0]);

	  for (i = 1; t1[i] && !strcmp ("", t1[i]); i++);


	  if (!error && (str = t1[i]))
	    if (!(t2 = g_strsplit (str, "/", 0)))
	      error = TRUE;

	  if (error || !t2[0])
	    error = TRUE;

#if 0
	  for (i = 0; !error && t2[0][i]; i++)
	    if (!isdigit (t2[0][i]))
	      error = TRUE;
#endif
	  /* TODO The h here is not portable */
	  if (!sscanf (t2[0], "%hd", &port_number) || (port_number < 1))
	    error = TRUE;

	  if (error || !t2[1])
	    error = TRUE;

	  if (error
	      || (g_strcasecmp ("udp", t2[1]) && g_strcasecmp ("tcp", t2[1])))
	    error = TRUE;

	  if (error)
	    g_warning ("Unable to  parse line %s", line);
	  else
	    {
	      g_my_debug ("Loading service %s %s %d", t2[1], t1[0],
			  port_number);
	      if (!g_strcasecmp ("tcp", t2[1]))
		{
		  tcp_service = g_malloc (sizeof (tcp_service_t));
		  tcp_service->number = port_number;
		  tcp_service->name = g_strdup (t1[0]);
		  g_tree_insert (tcp_services,
				 &(tcp_service->number), tcp_service);
		}
	      else
		{
		  udp_service = g_malloc (sizeof (udp_service_t));
		  udp_service->number = port_number;
		  udp_service->name = g_strdup (t1[0]);
		  g_tree_insert (udp_services,
				 &(udp_service->number), udp_service);
		}
	    }

	  g_strfreev (t2);
	  t2 = NULL;
	  g_strfreev (t1);
	  t1 = NULL;

	}
    }

  g_free (line);
  line = NULL;
  fclose (services);
}				/* load_services */
