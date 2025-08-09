#ifndef _TUSB_OPTION_H_
#define _TUSB_OPTION_H_

//--------------------------------------------------------------------+
// Mode and Speed
//--------------------------------------------------------------------+

#include "usb.h"

#ifndef CFG_TUSB_MEM_DCACHE_LINE_SIZE
#ifndef CFG_TUSB_MEM_DCACHE_LINE_SIZE_DEFAULT
#define CFG_TUSB_MEM_DCACHE_LINE_SIZE_DEFAULT 1
#endif

#define CFG_TUSB_MEM_DCACHE_LINE_SIZE CFG_TUSB_MEM_DCACHE_LINE_SIZE_DEFAULT
#endif

#endif /* _TUSB_OPTION_H_ */
