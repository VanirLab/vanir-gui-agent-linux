

#define SYSCALL(call) while (((call) == -1) && (errno == EINTR))

typedef struct _VanirDeviceRec
{
    char *device;
    int version;        /* Driver version */
    Atom* labels;
    int num_vals;
    int axes;
    unsigned int window_id; /* X Window ID for send_mfns callback */
} VanirDeviceRec, *VanirDevicePtr ;
