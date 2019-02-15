#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

void customHidInitialize(Service* service);
void* customHidGetSharedmemAddr();
extern Service hid_iappletresource;

#ifdef __cplusplus
}
#endif