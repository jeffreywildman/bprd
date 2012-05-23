#include "router.h"

#include <stdio.h>
#include <sys/socket.h>  /* must come before linux/netlink.h so sa_family_t is defined */
                         /* http://groups.google.com/group/linux.kernel/browse_thread/thread/6de65a3145007ae5?pli=1 */
#include <linux/netlink.h>

#include <net/if.h>

#include <netlink/errno.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/route/link.h>
#include <netlink/route/neighbour.h>
#include <netlink/route/route.h>

#include "util.h"
#include "logger.h"


static enum router_state {
    ROUTER_UP = 0,
    ROUTER_DOWN = 1
} state;

static struct nl_sock *router_nlsk;
static sa_family_t router_family;


/**
 * Initialize the router by binding and connecting a socket to the NETLINK_ROUTE protocol.
 */
void router_init(int family) {

    int err;

    if (state != ROUTER_DOWN) {
        return;
    }

    if (family != AF_INET && family != AF_INET6) {
        DUBP_LOG_ERR("Address family not recognized");
    } else {
        router_family = family; 
    }

    if ((router_nlsk = nl_socket_alloc()) == NULL) {
        DUBP_LOG_ERR("Error creating netlink socket");    
    }

    if ((err = nl_connect(router_nlsk,NETLINK_ROUTE)) < 0) {
        DUBP_LOG_ERR("Error connecting netlink socket: %s", nl_geterror(err));
    }

    state = ROUTER_UP;

}


/**
 * Cleanup the router. 
 */
void router_cleanup() {

    if (state == ROUTER_UP) {
        nl_close(router_nlsk);
        state = ROUTER_DOWN;
    }
}


/**
 * Update a route in the kernel's routing table.  
 *
 * \param dst Address of the destination.
 * \param nexthop Address of the nexthop.  If NULL, remove the route to dst.
 * \param ifindex Index of the outgoing interface.
 */
void router_route_update(struct sockaddr *dst, struct sockaddr *nh, unsigned int ifindex) {

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
    if (router_family == AF_INET6) {
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
    rtnl_route_set_family(route,router_family);
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


//static struct nl_cache *router_nlcache;
//
//
///**
// * Update the internal cache of routes within the router. 
// */
//static void router_cache_update() {
//
//    int err;
//    if (state != ROUTER_UP) {
//        DUBP_LOG_ERR("Update requested from uninitialized router");
//    }
//
//    if ((err = rtnl_route_alloc_cache(sk, router_family, 0, &cache)) < 0) {
//        DUBP_LOG_ERR("Error allocating cache: %s\n", nl_geterror(err));
//    }
//}
//
//
///**
// * Free the internal cache of routes within the router.
// */
//static void router_cache_free() {
//
//    nl_cache_free(cache);
//
//}
//
//
///**
// * Print the internal cache of routes within the router.
// */
//static void router_cache_print() {
//
//    struct rtnl_route *filter;
//
//    /* create filter */
//    filter = rtnl_route_alloc();
//    rtnl_route_set_table(filter,rtnl_route_str2table("main"));
//    rtnl_route_set_type(filter,nl_str2rtntype("unicast"));
//    rtnl_route_set_family(filter,AF_INET);
//
//    /* iterate and print all routes matching filter */
//    nl_cache_foreach_filter(cache, (struct nl_object *)filter, print_rtnl_route_cb, NULL);
//
//    rtnl_route_put(filter);
//}
//
//
//static void print_rtnl_nexthop(struct rtnl_nexthop *nexthop) {
//
//    char ifname[IF_NAMESIZE];
//    char buf[128];
//
//    printf("\tgateway: %s\n", nl_addr2str(rtnl_route_nh_get_gateway(nexthop),buf,128));
//    printf("\t\tweight: %u\n", rtnl_route_nh_get_weight(nexthop));
//    printf("\t\tifindex: %d (%s)\n", rtnl_route_nh_get_ifindex(nexthop), if_indextoname(rtnl_route_nh_get_ifindex(nexthop),ifname));
//    printf("\t\tflags: %u (%s)\n", rtnl_route_nh_get_flags(nexthop), rtnl_route_nh_flags2str(rtnl_route_nh_get_flags(nexthop),buf,128));
//    printf("\t\trealms: %u (%s)\n", rtnl_route_nh_get_realms(nexthop), rtnl_realms2str(rtnl_route_nh_get_realms(nexthop),buf,128));
//
//}
//
//
//static void print_rtnl_nexthop_cb(struct rtnl_nexthop *nexthop, void *arg) {
//
//    print_rtnl_nexthop(nexthop);
//
//}
//
//
//static void print_rtnl_route(struct rtnl_route *route) {
//
//    char ifname[IF_NAMESIZE];
//    char buf[128];
//
//    printf("ROUTE:\n");
//    /* TODO: extra must be done to read in non-standard table names */
//    printf("\ttable: %u (%s)\n", rtnl_route_get_table(route), rtnl_route_table2str(rtnl_route_get_table(route),buf,128));
//    printf("\tscope: %u (%s)\n", rtnl_route_get_scope(route), rtnl_scope2str(rtnl_route_get_scope(route),buf,128));
//    printf("\ttos: %u\n", rtnl_route_get_tos(route));
//    printf("\tprotocol: %u (%s)\n", rtnl_route_get_protocol(route), rtnl_route_proto2str(rtnl_route_get_protocol(route),buf,128));
//    printf("\tpriority: %u\n", rtnl_route_get_priority(route));
//    printf("\tfamily: %u (%s)\n", rtnl_route_get_family(route), nl_af2str(rtnl_route_get_family(route),buf,128));
//    printf("\tdst: %s\n", nl_addr2str(rtnl_route_get_dst(route),buf,128));
//    printf("\tsrc: %s\n", nl_addr2str(rtnl_route_get_src(route),buf,128));
//    printf("\ttype: %u (%s)\n", rtnl_route_get_type(route), nl_rtntype2str(rtnl_route_get_type(route),buf,128));
//    printf("\tflags: %x\n", rtnl_route_get_flags(route));
//    /* TODO: print metrics! */
//    printf("\tpref src: %s\n", nl_addr2str(rtnl_route_get_pref_src(route),buf,128));
//    printf("\tiif: %d (%s)\n", rtnl_route_get_iif(route), if_indextoname(rtnl_route_get_iif(route),ifname));
//    rtnl_route_foreach_nexthop(route, print_rtnl_nexthop_cb, NULL); 
//}
//
//
//static void print_rtnl_route_cb(struct nl_object *obj, void *arg) {
//
//    print_rtnl_route((struct rtnl_route *)obj);
//
//}
