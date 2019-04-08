#ifndef _VANIR_TXRX_H
#define _VANIR_TXRX_H

#include <sys/select.h>
#include <libvchan.h>

int write_data(libvchan_t *vchan, char *buf, int size);
int real_write_message(libvchan_t *vchan, char *hdr, int size, char *data, int datasize);
int read_data(libvchan_t *vchan, char *buf, int size);
#define read_struct(vchan, x) read_data(vchan, (char*)&x, sizeof(x))
#define write_struct(vchan, x) write_data(vchan, (char*)&x, sizeof(x))
#define write_message(vchan,x,y) do {\
	x.untrusted_len = sizeof(y); \
	real_write_message(vchan, (char*)&x, sizeof(x), (char*)&y, sizeof(y)); \
    } while(0)
void wait_for_vchan_or_argfd(libvchan_t *vchan, int nfd, int *fd, fd_set * retset);
void vchan_register_at_eof(void (*new_vchan_at_eof)(void));
void wait_for_possible_dispvm_resume(void);

#endif /* _VANIR_TXRX_H */
