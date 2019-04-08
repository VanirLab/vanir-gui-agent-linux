#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include  <X11/Xlib.h>
#include <X11/Xlibint.h>

int print_x11_errors = 0;

int dummy_handler(Display * dpy, XErrorEvent * ev)
{
#define ERROR_BUF_SIZE 256
    char buf[ERROR_BUF_SIZE];
    char request[ERROR_BUF_SIZE];
    _XExtension *ext = NULL;

	if (!print_x11_errors)
		return 0;

    XGetErrorText(dpy, ev->error_code, buf, sizeof(buf));
    fprintf(stderr, "ErrorHandler: %s\n", buf);

    /* Find major opcode name */
    if (ev->request_code < 128) {
        snprintf(request, sizeof(request), "%d", ev->request_code);
        XGetErrorDatabaseText(dpy, "XRequest", request, "", buf,
                sizeof(buf));
    } else {
        for (ext = dpy->ext_procs;
                ext && ext->codes.major_opcode != ev->request_code;
                ext = ext->next);
        if (ext) {
            strncpy(buf, ext->name, sizeof(buf)-1);
            buf[sizeof(buf)-1] = '\0';
        } else {
            buf[0] = '\0';
        }
    }
    fprintf(stderr, "                 Major opcode: %d (%s)\n",
            ev->request_code, buf);

    /* Find minor opcode name */
    if (ev->request_code >= 128 && ext) {
        snprintf(request, sizeof(request), "%d", ev->request_code);
        snprintf(request, sizeof(request), "%s.%d",
                ext->name, ev->minor_code);
        XGetErrorDatabaseText(dpy, "XRequest", request, "", buf,
                sizeof(buf));
        fprintf(stderr, "                 Minor opcode: %d (%s)\n",
                ev->minor_code, buf);
    }

    /* Provide value information */
    switch (ev->error_code) {
        case BadValue:
            fprintf(stderr, "                 Value:        0x%x\n",
                    (int) ev->resourceid);
            break;
        case BadAtom:
            fprintf(stderr, "                 AtomID:       0x%x\n",
                    (int) ev->resourceid);
            break;
        default:
            fprintf(stderr, "                 ResourceID:   0x%x\n",
                    (int) ev->resourceid);
            break;
    }

    /* Provide serial number information */
    fprintf(stderr, "                 Failed serial number:  %ld\n",
            ev->serial);
    fprintf(stderr, "                 Current serial number: %ld\n",
            dpy->request);
    return 0;
}
