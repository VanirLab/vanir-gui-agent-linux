#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <linux/input.h>
#include <linux/types.h>

#include <xf86_OSproc.h>

#include <unistd.h>

#include <xf86.h>
#include <xf86Xinput.h>
#include <exevents.h>
#include <xorgVersion.h>
#include <xkbsrv.h>


#include <windowstr.h>

#ifdef HAVE_PROPERTIES
#include <xserver-properties.h>
/* 1.6 has properties, but no labels */
#ifdef AXIS_LABEL_PROP
#define HAVE_LABELS
#else
#undef HAVE_LABELS
#endif

#endif

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 23
#define HAVE_THREADED_INPUT 1
#else
#undef HAVE_THREADED_INPUT
#endif


#include <stdio.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <xorg-server.h>
#include <xorgVersion.h>
#include <xf86Module.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <vanir-gui-protocol.h>
#include "xdriver-shm-cmd.h"
#include "vanir.h"

#include "../../xf86-vanir-common/include/xf86-vanir-common.h"

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 12
static int VanirPreInit(InputDriverPtr drv, InputInfoPtr pInfo,
                        int flags);
#else
static InputInfoPtr VanirPreInit(InputDriverPtr drv, IDevPtr dev,
                                 int flags);
#endif

static void VanirUnInit(InputDriverPtr drv, InputInfoPtr pInfo,
                        int flags);
static pointer VanirPlug(pointer module, pointer options, int *errmaj,
                         int *errmin);
static void VanirUnplug(pointer p);
static void VanirReadInput(InputInfoPtr pInfo);
static int VanirControl(DeviceIntPtr device, int what);
static int _vanir_init_buttons(DeviceIntPtr device);
static int _vanir_init_axes(DeviceIntPtr device);
#if HAVE_THREADED_INPUT
static void VanirBlockHandler(void *arg, void *timeout);
static void VanirWakeupHandler(void *arg, int result);
#endif



_X_EXPORT InputDriverRec VANIR = {
    1,
    "vanir",
    NULL,
    VanirPreInit,
    VanirUnInit,
    NULL,
    0
};

static XF86ModuleVersionInfo VanirVersionRec = {
    "vanir",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    PACKAGE_VERSION_MAJOR, PACKAGE_VERSION_MINOR,
    PACKAGE_VERSION_PATCHLEVEL,
    ABI_CLASS_XINPUT,
    ABI_XINPUT_VERSION,
    MOD_CLASS_XINPUT,
    {0, 0, 0, 0}
};

_X_EXPORT XF86ModuleData vanirModuleData = {
    &VanirVersionRec,
    &VanirPlug,
    &VanirUnplug
};

static void VanirUnplug(pointer p)
{
};

static pointer
VanirPlug(pointer module, pointer options, int *errmaj, int *errmin)
{
    xf86AddInputDriver(&VANIR, module, 0);
    return module;
};


#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 12
static int VanirPreInit(InputDriverPtr drv,
                        InputInfoPtr pInfo, int flags)
#else
static InputInfoPtr VanirPreInit(InputDriverPtr drv,
                                 IDevPtr dev, int flags)
#endif
{
    VanirDevicePtr pVanir;
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 12
    InputInfoPtr pInfo;

    if (!(pInfo = xf86AllocateInput(drv, 0)))
        return NULL;
#endif

    pVanir = calloc(1, sizeof(VanirDeviceRec));
    if (!pVanir) {
        pInfo->private = NULL;
        xf86DeleteInput(pInfo, 0);
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 12
        return BadAlloc;
#else
        return NULL;
#endif
    }

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 12
    pInfo->name = xstrdup(dev->identifier);
    pInfo->flags = 0;
    pInfo->conf_idev = dev;
#endif

    pInfo->private = pVanir;
    pInfo->type_name = XI_MOUSE;    /* see XI.h */
    pInfo->read_input = VanirReadInput;    /* new data avl */
    pInfo->switch_mode = NULL;    /* toggle absolute/relative mode */
    pInfo->device_control = VanirControl;    /* enable/disable dev */
    /* process driver specific options */
    pVanir->device = xf86SetStrOption(
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 12
            pInfo->options,
#else
            dev->commonOptions,
#endif
            "Device", "/var/run/xf86-vanir-socket");

    xf86Msg(X_INFO, "%s: Using device %s.\n", pInfo->name,
            pVanir->device);

    /* process generic options */
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 12
    xf86CollectInputOptions(pInfo, NULL);
#else
    xf86CollectInputOptions(pInfo, NULL, NULL);
#endif
    xf86ProcessCommonOptions(pInfo, pInfo->options);
    /* Open sockets, init device files, etc. */
    pInfo->fd = -1;

#if HAVE_THREADED_INPUT
    if (!RegisterBlockAndWakeupHandlers(VanirBlockHandler,
                                        VanirWakeupHandler,
                                        (void *) pInfo)) {
        xf86Msg(X_ERROR, "%s: Failed to register block/wakeup handler\n",
                pInfo->name);
        VanirUnInit(drv, pInfo, flags);
        return BadAlloc;
    }
#endif

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 12
    return Success;
#else
    pInfo->flags |= XI86_OPEN_ON_INIT;
    pInfo->flags |= XI86_CONFIGURED;
    return pInfo;
#endif
}

static void VanirUnInit(InputDriverPtr drv, InputInfoPtr pInfo, int flags)
{
    VanirDevicePtr pVanir = pInfo->private;

#if HAVE_THREADED_INPUT
    RemoveBlockAndWakeupHandlers(VanirBlockHandler,
                                 VanirWakeupHandler,
                                 (void *) pInfo);
#endif

    if (pVanir->device) {
        free(pVanir->device);
        pVanir->device = NULL;
        /* Common error - pInfo->private must be NULL or valid memoy before
         * passing into xf86DeleteInput */
        pInfo->private = NULL;
    }
    xf86DeleteInput(pInfo, 0);
}

static int _vanir_init_kbd(DeviceIntPtr device)
{
    InitKeyboardDeviceStruct(device, NULL, NULL, NULL);
    return Success;
}

static int _vanir_init_buttons(DeviceIntPtr device)
{
    InputInfoPtr pInfo = device->public.devicePrivate;
    VanirDevicePtr pVanir = pInfo->private;
    CARD8 *map;
    int i;
    int ret = Success;
    const int num_buttons = 7;

    map = calloc(num_buttons+1, sizeof(CARD8));

    xf86Msg(X_INFO, "%s: num_buttons=%d\n", pInfo->name, num_buttons);

    for (i = 1; i <= num_buttons; i++)
        map[i] = i;

    pVanir->labels = calloc(num_buttons, sizeof(Atom));

    if (!InitButtonClassDeviceStruct(device, num_buttons,
                                     pVanir->labels, map)) {
        xf86Msg(X_ERROR, "%s: Failed to register buttons.\n",
                pInfo->name);
        ret = BadAlloc;
    }

    free(map);
    return ret;
}

static void VanirInitAxesLabels(VanirDevicePtr pVanir, int natoms,
                                Atom * atoms)
{
#ifdef HAVE_LABELS
    Atom atom;
    int axis;
    char **labels;
    int labels_len = 0;
    char *misc_label;

    labels = rel_labels;
    labels_len = ArrayLength(rel_labels);
    misc_label = AXIS_LABEL_PROP_REL_MISC;

    memset(atoms, 0, natoms * sizeof(Atom));

    /* Now fill the ones we know */
    for (axis = 0; axis < labels_len; axis++) {
        if (pVanir->axis_map[axis] == -1)
            continue;

        atom = XIGetKnownProperty(labels[axis]);
        if (!atom)    /* Should not happen */
            continue;

        atoms[pVanir->axis_map[axis]] = atom;
    }
#endif
}


static int _vanir_init_axes(DeviceIntPtr device)
{
    InputInfoPtr pInfo = device->public.devicePrivate;
    VanirDevicePtr pVanir = pInfo->private;
    int i;
    const int num_axes = 2;
    Atom *atoms;

    pVanir->num_vals = num_axes;
    atoms = malloc(pVanir->num_vals * sizeof(Atom));

    VanirInitAxesLabels(pVanir, pVanir->num_vals, atoms);
    if (!InitValuatorClassDeviceStruct(device, num_axes,
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 7
                                       atoms,
#endif
                                       GetMotionHistorySize(), 0))
        return BadAlloc;

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 12
    pInfo->dev->valuator->mode = Relative;
#endif
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 13
    if (!InitAbsoluteClassDeviceStruct(device))
        return BadAlloc;
#endif

    for (i = 0; i < pVanir->axes; i++) {
        xf86InitValuatorAxisStruct(device, i, *pVanir->labels, -1,
                -1, 1, 1, 1
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 12
                , Relative
#endif
                );
        xf86InitValuatorDefaults(device, i);
    }
    free(atoms);
    return Success;
}
int connect_unix_socket(VanirDevicePtr pVanir);
int connect_unix_socket(VanirDevicePtr pVanir)
{
    int s, len;
    struct sockaddr_un remote;

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }


    remote.sun_family = AF_UNIX;
    strncpy(remote.sun_path, pVanir->device, sizeof(remote.sun_path));
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(s, (struct sockaddr *) &remote, len) == -1) {
        perror("connect");
        close(s);
        return -1;
    }
    return s;
}

static void close_device_fd(InputInfoPtr pInfo) {
    if (pInfo->fd >= 0) {
        xf86RemoveEnabledDevice(pInfo);
        close(pInfo->fd);
        pInfo->fd = -1;
    }
}

static void
VanirPtrCtrlProc (DeviceIntPtr device, PtrCtrl *ctrl)
{
    /* This function intentionally left blank */
}

static int VanirControl(DeviceIntPtr device, int what)
{
    InputInfoPtr pInfo = device->public.devicePrivate;
    VanirDevicePtr pVanir = pInfo->private;

    switch (what) {
    case DEVICE_INIT:
        device->public.on = FALSE;
        _vanir_init_buttons(device);
        _vanir_init_axes(device);
        _vanir_init_kbd(device);
        InitPtrFeedbackClassDeviceStruct(device, VanirPtrCtrlProc);
        break;

        /* Switch device on.  Establish socket, start event delivery.  */
    case DEVICE_ON:
        xf86Msg(X_INFO, "%s: On.\n", pInfo->name);
        if (device->public.on)
            break;
        do {
            pInfo->fd = connect_unix_socket(pVanir);
            if (pInfo->fd < 0) {
                xf86Msg(X_ERROR,
                        "%s: cannot open device; sleeping...\n",
                        pInfo->name);
                sleep(1);
            }
        } while (pInfo->fd < 0);

        xf86FlushInput(pInfo->fd);
        xf86AddEnabledDevice(pInfo);
        device->public.on = TRUE;
        break;
    case DEVICE_OFF:
        xf86Msg(X_INFO, "%s: Off.\n", pInfo->name);
        if (!device->public.on)
            break;
        close_device_fd(pInfo);
        device->public.on = FALSE;
        break;
    case DEVICE_CLOSE:
        /* free what we have to free */
        break;
    }
    return Success;
}

/* The following helper is copied from Xen sources */
static int write_exact(int fd, const void *data, size_t size);

static int write_exact(int fd, const void *data, size_t size)
{
    size_t offset = 0;
    ssize_t len;

    while ( offset < size )
    {
        len = write(fd, (const char *)data + offset, size - offset);
        if ( (len == -1) && (errno == EINTR) )
            continue;
        if ( len <= 0 )
            return -1;
        offset += len;
    }

    return 0;
}

static WindowPtr id2winptr(unsigned int xid)
{
    int ret;
    WindowPtr result = NULL;
    ret = dixLookupResourceByClass((void**)&result, xid, RC_DRAWABLE, 0, 0);
    if (ret == Success)
        return result;
    else
        return NULL;
}

static void dump_window_grant_refs(int window_id, int fd);

static void dump_window_grant_refs(int window_id, int fd)
{
    ScreenPtr screen;
    PixmapPtr pixmap;
    struct msg_window_dump_hdr wd_hdr;
    size_t wd_msg_len = 0; // 0 means error
    struct xf86_qubes_pixmap *priv = NULL;
    WindowPtr x_window = id2winptr(window_id);
    if (x_window == NULL)
        // This error condition (window not found) can happen when
        // the window is destroyed before the driver sees the req
        goto send_response;

    screen = x_window->drawable.pScreen;
    pixmap = (*screen->GetWindowPixmap) (x_window);

    priv = xf86_qubes_pixmap_get_private(pixmap);
    if (priv == NULL) {
        xf86Msg(X_ERROR, "can't dump window without grant table allocation\n");
        goto send_response;
    }

    wd_hdr.type = WINDOW_DUMP_TYPE_GRANT_REFS;
    wd_hdr.width = pixmap->drawable.width;
    wd_hdr.height = pixmap->drawable.height;
    wd_hdr.bpp = pixmap->drawable.bitsPerPixel;

    if (wd_hdr.width > MAX_WINDOW_WIDTH ||
        wd_hdr.height > MAX_WINDOW_HEIGHT ||
        priv->pages > MAX_GRANT_REFS_COUNT) {
        xf86Msg(X_ERROR,
                "window has invalid dimensions %ix%i (%i bpp), %zu grant pages\n",
                wd_hdr.width,
                wd_hdr.height,
                wd_hdr.bpp,
                priv->pages);
        goto send_response;
    }

    // We don't have any custom arguments except the variable length refs list.
    assert(sizeof(struct msg_window_dump_grant_refs) == 0);

    wd_msg_len = MSG_WINDOW_DUMP_HDR_LEN + priv->pages * SIZEOF_GRANT_REF;

send_response:
    if (write_exact(fd, &wd_msg_len, sizeof(wd_msg_len)) == -1) {
        char errbuf[128];
        if (strerror_r(errno, errbuf, sizeof(errbuf)) == 0)
            xf86Msg(X_ERROR,
                    "failed write to gui-agent: %s\n", errbuf);
        return;
    }

    if (wd_msg_len == 0)
        // error case
        return;

    if (write_exact(fd, &wd_hdr, sizeof(wd_hdr)) == -1) {
        char errbuf[128];
        if (strerror_r(errno, errbuf, sizeof(errbuf)) == 0)
            xf86Msg(X_ERROR,
                    "failed write to gui-agent: %s\n", errbuf);
        return;
    }

    if (write_exact(fd, &priv->refs[0], priv->pages * SIZEOF_GRANT_REF) == -1) {
        char errbuf[128];
        if (strerror_r(errno, errbuf, sizeof(errbuf)) == 0)
            xf86Msg(X_ERROR,
                    "failed write to gui-agent: %s\n", errbuf);
        return;
    }
}

static void process_window_dump_request(InputInfoPtr pInfo) {
    VanirDevicePtr pVanir = pInfo->private;

    if (pVanir->window_id != 0) {
        dump_window_grant_refs(pVanir->window_id, pInfo->fd);
        pVanir->window_id = 0;
    }
}

#if HAVE_THREADED_INPUT
static void VanirBlockHandler(void *arg, void *timeout) {
    InputInfoPtr pInfo = arg;

    input_lock();
    process_window_dump_request(pInfo);
    input_unlock();
}

static void VanirWakeupHandler(void *arg, int result) {
    // Nothing to do.
}
#endif

static void process_request(int fd, InputInfoPtr pInfo)
{
    VanirDevicePtr pVanir = pInfo->private;
    int ret;
    struct xdriver_cmd cmd;


    ret = read(fd, &cmd, sizeof(cmd));
    if (ret == 0) {
        xf86Msg(X_INFO, "randdev: unix closed\n");
        close_device_fd(pInfo);
        return;
    }
    if (ret == -1) {
        xf86Msg(X_INFO, "randdev: unix error\n");
        close_device_fd(pInfo);
        return;
    }

    write_exact(fd, "0", 1); // acknowledge the request has been received

    switch (cmd.type) {
    case 'W':
        pVanir->window_id = cmd.arg1;
#if HAVE_THREADED_INPUT
        // We need to handle the window in the main thread, see
        // VanirBlockHandler(). The mutex is already locked when VanirReadInput
        // is called. The input thread loop will also wake up the main thread
        // for us. vanir-gui will wait for our answer, therefore there's no
        // risk that we will get other events from it before we have send the
        // answer in the main thread.
#else
        // In the classical case we can process the window directly.
        process_window_dump_request(pInfo);
#endif
        break;
    case 'B':
        xf86PostButtonEvent(pInfo->dev, 0, cmd.arg1, cmd.arg2, 0,0);
        break;
    case 'M':
        xf86PostMotionEvent(pInfo->dev, 1, 0, 2, cmd.arg1, cmd.arg2);
        break;
    case 'K':
        xf86PostKeyboardEvent(pInfo->dev, cmd.arg1, cmd.arg2);
        break;
    default:
        xf86Msg(X_INFO, "randdev: unknown command %u\n", cmd.type);
    }
}

static void VanirReadInput(InputInfoPtr pInfo)
{
    while (xf86WaitForInput(pInfo->fd, 0) > 0) {
        process_request(pInfo->fd, pInfo);
    }
}
