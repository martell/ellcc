#ifndef _lwip_interface_h_
#define _lwip_interface_h_

#include "config.h"
#include "lwip/netif.h"

#if ELK_NAMESPACE
#define lwip_add_interface __elk_lwip_add_interface
#endif

/** Add an interface to the interface list.
 */
int lwip_add_interface(const char *name, netif_init_fn init,
                       netif_input_fn input, void *state);

#endif // _lwip_interface_h_
