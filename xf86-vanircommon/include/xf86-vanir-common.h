#ifndef XF86_VANIR_COMMON_H
#define XF86_VANIR_COMMON_H

// Order is important!
#include <xorg-server.h>
#include <xf86.h>

// Metadata of mapped grant pages
struct xf86_vanir_pixmap {
    size_t pages; // Number of pages
    uint32_t *refs; // Pointer to grant references
    uint8_t *data; // Local mapping
};

// Only intended for use in the Vanir xorg modules.

extern _X_EXPORT Bool xf86_vanir_pixmap_register_private(void);

extern _X_EXPORT void xf86_vanir_pixmap_set_private(
        PixmapPtr pixmap,
        struct xf86_vanir_pixmap *priv);

extern _X_EXPORT struct xf86_vanir_pixmap *xf86_vanir_pixmap_get_private(
        PixmapPtr pixmap);

// xenctrl and xorg headeres are not compatible. So define the requires
// constants here.
#ifndef XC_PAGE_SHIFT
#define XC_PAGE_SHIFT           12
#define XC_PAGE_SIZE            (1UL << XC_PAGE_SHIFT)
#define XC_PAGE_MASK            (~(XC_PAGE_SIZE-1))
#endif

#endif
