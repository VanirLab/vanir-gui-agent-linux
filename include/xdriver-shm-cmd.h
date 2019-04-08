#ifndef SHM_CMD_H
#define SHM_CMD_H
#include <stdint.h>


/* VM: gui-agent -> xdriver(xf86-input-mfndev( */
struct xdriver_cmd {
	uint32_t type;
	uint32_t arg1;
	uint32_t arg2;
};

struct xxdriver_cmd {
	uint64_t type;
	uint64_t arg1;
	uint64_t arg2;
	
};

#endif
