/**
 * \defgroup router Router
 * This module interfaces the DUBP process with the kernel's routing table to add/update/delete routes.
 * \{
 */

#include "router.h"

#include <limits.h>      /* for PATH_MAX */
#include <stdio.h>       /* for snprintf() */
#include <net/if.h>      /* for if_indextoname(), IF_NAMESIZE */
#include <sys/socket.h>  /* must come before linux/netlink.h so sa_family_t is defined */
                         /* http://groups.google.com/group/linux.kernel/browse_thread/thread/6de65a3145007ae5?pli=1 */

#include <linux/netlink.h>              /* for NETLINK_ROUTE */

#include <netlink/addr.h>               /* for nl_addr, nl_addr_build(), nl_addr_put() */
#include <netlink/errno.h>              /* for nl_geterror() */
#include <netlink/netlink.h>            /* for nl_connect(), nl_close() */
#include <netlink/route/route.h>        /* for rtnl_route*, rtnl_nexthop* */
#include <netlink/socket.h>             /* for nl_sock, nl_socket_alloc(), nl_socket_free() */
//#include <netlink/route/link/inet.h>    /* for ... */

#include "logger.h"
#include "procfile.h"
#include "commodity.h"
#include "neighbor.h"
#include "list.h"
#include "dubp.h"


static struct nl_sock *router_nlsk;     /**< Internal reference to the netlink socket. */
static unsigned int router_if_index;    /**< Interface to route over. */
static unsigned int router_family;      /**< Address family to route. */
static char router_origfwd;             /**< Previous forwarding state. */
static char router_procfile[PATH_MAX];  /**< Path to file in proc/sys controlling IP forwarding. */


/**
 * Initialize the router by binding and connecting a socket to the NETLINK_ROUTE protocol and enabling IP forwarding.
 *
 * \param if_index The interface to enable forwarding on.
 * \param family The address family to enable forwarding for.
 *
 * \retval 0 On success.
 * \retval -1 On error.
 */
int router_init(unsigned int if_index, unsigned int family) {

//    struct nl_cache *cache;
//    struct rtnl_link *link, *new;
    int n;
    char if_name[IF_NAMESIZE];

    router_if_index = if_index;
    router_family = family;

    if ((router_nlsk = nl_socket_alloc()) == NULL) {
        return -1;
    }

    /* nl_connect returns error number, can be used by nl_geterror(err) */
    if (nl_connect(router_nlsk, NETLINK_ROUTE) < 0) {
        return -1;
    }

    if (if_indextoname(if_index, if_name) == NULL) {
        return -1;   
    }

    if (router_family == AF_INET6) {
        n = snprintf(router_procfile, PATH_MAX, "/proc/sys/net/ipv6/conf/%s/forwarding", if_name);
    } else {
        n = snprintf(router_procfile, PATH_MAX, "/proc/sys/net/ipv4/conf/%s/forwarding", if_name);
    }

    if (n < 0 || n == PATH_MAX) {
        return -1;
    }

    if (procfile_write(router_procfile, &router_origfwd, '1') < 0) {
        return -1;   
    }

/** \todo Upgrade to newer libnl to support IP forwarding configuration */
//    /* get cache of links */
//    rtnl_link_alloc_cache(router_nlsk, router_family, &cache);
//
//    /* get link */
//    link = rtnl_link_get(cache, router_if_index);
//
//    /** \todo Support IPv6 address family via sysctl (libnl support yet to come -
//     * http://comments.gmane.org/gmane.linux.network/229869).*/
//    if (router_family == AF_INET6) {
//        /* crash and burn! */
//        return -1;
//    }
//
//    /* save current forwarding state */
//    if (rtnl_link_inet_get_conf(link, IPV4_DEVCONF_FORWARDING, &router_prevfwd) < 0) {
//        return -1;
//    }
//   
//    /* set new forwarding state */
//    if ((new = rtnl_link_alloc()) == NULL) {
//        return -1;   
//    }
//    if (rtnl_link_inet_set_conf(new, IPV4_DEVCONF_FORWARDING, 1) < 0) {
//        return -1;
//    }
//    if (rtnl_link_change(router_nlsk, link, new, 0) < 0) {
//        return -1;
//    }
//
//    rtnl_link_put(link);
//    nl_cache_free(cache);

    return 0;
}


/**
 * Cleanup the router and return interface's forwarding state to previous state.
 *
 * \todo Handle IPv6 ip forwarding reset.
 */
void router_cleanup() {

//    struct nl_cache *cache;
//    struct rtnl_link *link, *new;
//
//    rtnl_link_alloc_cache(sk, router_family, &cache);
//    link = rtnl_link_get(cache, router_if_index);
//    new = rtnl_link_alloc();
//    rtnl_link_inet_set_conf(new, IPV4_DEVCONF_FORWARDING, router_prevfwd);
//    rtnl_link_change(router_nlsk, link, new, 0);
//    
//    rtnl_link_put(link);
//    nl_cache_free(cache);

    /** \todo error handling */  
    procfile_write(router_procfile, NULL, router_origfwd);

    nl_close(router_nlsk);
    nl_socket_free(router_nlsk);
}


/**
 * Update the backlogs on each commodity.  Update the backlog differential to each neighbor for each commodity.  Update
 * the max backlog differential for each commodity.
 */
void router_update() {

    elm_t *e, *f;
    neighbor_t *n, *nopt;
    commodity_t *c, *ctemp;
    uint32_t diffopt;
    struct netaddr naddr;
    union netaddr_socket nsaddr;
   
    /* convert my address into a netaddr for easy comparison */
    nsaddr.std = *dubpd.saddr; 
    netaddr_from_socket(&naddr, &nsaddr);

    /* update my commodity levels */
    for(e = LIST_FIRST(&dubpd.clist); e != NULL; e = LIST_NEXT(e, elms)) {
            c = (commodity_t *)e->data;
            assert(c->queue);
            c->cdata.backlog = fifo_length(c->queue);
    }

    ntable_mutex_lock(&dubpd.ntable);

    /* update backlog differential for each neighbor's commodity */
    for (e = LIST_FIRST(&dubpd.ntable.nlist); e != NULL; e = LIST_NEXT(e, elms)) {
        n = (neighbor_t *)e->data;

        for (f = LIST_FIRST(&n->clist); f != NULL; f = LIST_NEXT(f, elms)) {
            c = (commodity_t *)f->data;

            /* find matching commodity in dubpd.clist */
            if ((ctemp = clist_find(&dubpd.clist, c)) == NULL) {
                DUBP_LOG_ERR("Neighbor knows about commodity that I don't!");
            }

            if (ctemp->cdata.backlog >= c->cdata.backlog) {
                c->backdiff = ctemp->cdata.backlog - c->cdata.backlog;
            } else {
                c->backdiff = 0;
            } 
        }
    }

    /* find the optimal next hop for each commodity */
    /* for each commodity, also save the max backlog differential */
    for(e = LIST_FIRST(&dubpd.clist); e != NULL; e = LIST_NEXT(e, elms)) {
        c = (commodity_t *)e->data;

        if (netaddr_cmp(&naddr, &(c->cdata.addr)) == 0) {
            /* the commodity is destined to me! ignore it */
            struct netaddr_str tempstr;
            DUBP_LOG_DBG("Ignoring commodity destined to: %s", netaddr_to_string(&tempstr, &c->cdata.addr));
            c->backdiff = 0;
            continue;
        }

        /* tie breaker */
        int num = 0;
        diffopt = 0;
        nopt = NULL;

        /* try to find this commodity in neighbor's clist */
        for(f = LIST_FIRST(&dubpd.ntable.nlist); f != NULL; f = LIST_NEXT(f, elms)) {
            n = (neighbor_t *)f->data;

            if ((ctemp = clist_find(&n->clist, c)) == NULL) {
                DUBP_LOG_ERR("I know about a commodity that my neighbor doesn't!");
            }

            if (!n->bidir) {
                /* I can hear the neighbor, but not sure if I can speak to the neighbor, skip him */
                continue;
            }

            if (netaddr_cmp(&n->addr,&ctemp->cdata.addr)) {
                /* The neighbor is the commodity's destination, send to him */
                /** \todo Fully consider the built-in assumption -> unicast commodities (single-destination) */
                nopt = n;
                break;
            }

            if (ctemp->backdiff < diffopt) {
                /* we found a neighbor with smaller backlog differential, or less than zero, ignore it */
                continue;
            } else if (ctemp->backdiff == diffopt) {
                /* we found a neighbor with equal backlog differential */
                num++;
            } else {
                /* we found a neighbor with larger backlog differential */
                num = 1;
                diffopt = ctemp->backdiff;
            }
                
            /* we use the following test to determine if we have a new nexthop */
            if (((double)rand())/((double)RAND_MAX) >= ((double)(num-1))/((double)num)) {
                /* this results in uniformly choosing amongst an unknown number of ties */
                /* when num == 1, we always satisfy the test */
                nopt = n;
            }
        }

        union netaddr_socket nsaddr_dst, nsaddr_nh;
        netaddr_to_socket(&nsaddr_dst, &(c->cdata.addr));
 
        /* if we have a valid neighbor... */
        if (nopt) {
            /* by here, we have the best nexthop for commodity c, set it */
            /* convert commodity destination and nexthop addresses from netaddr to socket */
            netaddr_to_socket(&nsaddr_nh, &(nopt->addr));
            router_route_update(&(nsaddr_dst.std), &(nsaddr_nh.std), dubpd.ipver, dubpd.if_index);
            /* save the max differential inside my commodity list */
            c->backdiff = diffopt;
        } else {
            /* no valid neighbors to send commodity to! */
            c->backdiff = 0;
        }
    }

    ntable_mutex_unlock(&dubpd.ntable);

}


/**
 * Release packets back to kernel.
 *
 * Find the commodity with the largest backlog differential and send \a count packets from it.
 *
 * \param count Number of packets to release.
 */ 
void router_release(unsigned int count) {

    elm_t *e;
    commodity_t *c = NULL;
    commodity_t *ctemp;
    uint32_t diffopt = 0;

    /* search for largest max differential */
    for (e = LIST_FIRST(&dubpd.clist); e != NULL; e = LIST_NEXT(e, elms)) {
        ctemp = (commodity_t *)e->data;
        
        if (ctemp->backdiff > diffopt) {
            c = ctemp;
            diffopt = ctemp->backdiff;
        }
    }

    if (c) {
        /* only send up to min(count,diffopt/2) packets! otherwise gradient will reverse */
        count = diffopt/2 > count ? count : diffopt/2;
        /* release up to count packets of this commodity */
        while (count--) {fifo_send_packet(c->queue);}
    }
}


/**
 * Update a route in the kernel's routing table.  
 *
 * \param dst Address of the destination.
 * \param nh Address of the nexthop.  If NULL, remove the route to \a dst.
 * \param family Address family.
 * \param ifindex Index of the outgoing interface.
 */
void router_route_update(struct sockaddr *dst, struct sockaddr *nh, unsigned int family, unsigned int ifindex) {

    int err;
    struct nl_addr *nl_dst_addr, *nl_nh_addr;
    struct rtnl_route *route;
    struct rtnl_nexthop *nexthop;

    /* input checking */
    if (dst == NULL) {
        DUBP_LOG_ERR("Destination address is empty");
    }

    /* convert socket addresses to netlink abstract addresses */
    /** \note some ugly looking typecasting to first extract address from sockaddr and then to pass as (void *) */
    if (family == AF_INET6) {
        nl_dst_addr = nl_addr_build(AF_INET6, (void *)&((struct sockaddr_in6 *)dst)->sin6_addr, sizeof(struct in6_addr));
        (nh != NULL) ? nl_nh_addr = nl_addr_build(AF_INET6, (void *)&((struct sockaddr_in6 *)nh)->sin6_addr, sizeof(struct in6_addr)) : NULL;
    } else {
        nl_dst_addr = nl_addr_build(AF_INET, (void *)&((struct sockaddr_in *)dst)->sin_addr, sizeof(struct in_addr));
        (nh != NULL) ? nl_nh_addr = nl_addr_build(AF_INET, (void *)&((struct sockaddr_in *)nh)->sin_addr, sizeof(struct in_addr)) : NULL;
    }

    if (nl_dst_addr == NULL || (nh != NULL && nl_nh_addr == NULL)) {
        DUBP_LOG_ERR("Unable to convert socket addresses to netlink abstract addresses");
    }

    /* create route and add preliminary fields */
    if ((route = rtnl_route_alloc()) == NULL ) {
        DUBP_LOG_ERR("Unable to allocate netlink route");
    }
    rtnl_route_set_table(route,rtnl_route_str2table("main"));
    rtnl_route_set_scope(route,rtnl_str2scope("universe"));
    /** \todo Change from static to dubp-specific protocol number? */
    rtnl_route_set_protocol(route,rtnl_route_str2proto("static"));
    rtnl_route_set_family(route,family);
    rtnl_route_set_dst(route,nl_dst_addr);  
    rtnl_route_set_type(route,nl_str2rtntype("unicast"));

    if (nh != NULL) {
        /* add nexthop information and add to table */
        nexthop = rtnl_route_nh_alloc();
        rtnl_route_nh_set_ifindex(nexthop,ifindex);
        rtnl_route_nh_set_gateway(nexthop,nl_nh_addr);
        rtnl_route_add_nexthop(route,nexthop);
        if ((err = rtnl_route_add(router_nlsk,route,NLM_F_REPLACE)) < 0) {
            DUBP_LOG_ERR("Error adding route: %s\n", nl_geterror(err));
        }
        rtnl_route_nh_free(nexthop);
        nl_addr_put(nl_nh_addr); 
    } else {
        /* remove from table */
        /** \todo figure out what happens if route doesn't already exist in table */
        if ((err = rtnl_route_delete(router_nlsk,route,0)) < 0) {
            DUBP_LOG_ERR("Error deleting route: %s\n", nl_geterror(err));
        }
    }

    nl_addr_put(nl_dst_addr);  
}

/** \} */
