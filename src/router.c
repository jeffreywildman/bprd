/**
 * \defgroup Router
 * \{
 */

#include "router.h"

#include <sys/socket.h>  /* must come before linux/netlink.h so sa_family_t is defined */
                         /* http://groups.google.com/group/linux.kernel/browse_thread/thread/6de65a3145007ae5?pli=1 */

#include <linux/netlink.h>              /* for NETLINK_ROUTE */

#include <netlink/addr.h>               /* for nl_addr, nl_addr_build(), nl_addr_put() */
#include <netlink/errno.h>              /* for nl_geterror() */
#include <netlink/netlink.h>            /* for nl_connect(), nl_close() */
#include <netlink/route/route.h>        /* for rtnl_route*, rtnl_nexthop* */
#include <netlink/socket.h>             /* for nl_sock, nl_socket_alloc(), nl_socket_free() */

#include "logger.h"


static struct nl_sock *router_nlsk;     /**< Internal reference to the netlink socket. */


/**
 * Initialize the router by binding and connecting a socket to the NETLINK_ROUTE protocol.
 *
 * \return Returns 0 on success, otherwise -1 on error.
 */
int router_init() {

    if ((router_nlsk = nl_socket_alloc()) == NULL) {
        return -1;
    }

    /* nl_connect returns error number, can be used by nl_geterror(err) */
    if (nl_connect(router_nlsk,NETLINK_ROUTE) < 0) {
        return -1;
    }

    return 0;
}


/**
 * Cleanup the router. 
 */
void router_cleanup() {

    nl_close(router_nlsk);
    nl_socket_free(router_nlsk);
}


/**
 * Update a route in the kernel's routing table.  
 *
 * \param dst Address of the destination.
 * \param nh Address of the nexthop.  If NULL, remove the route to dst.
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
