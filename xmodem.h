#ifndef _XMODEM_H_
#define _XMODEM_H_

int xmodem_init(void);
void xmodem_close(void);

int xmodem_read(int fd, void *privdata);

#endif /* _XMODEM_H_ */
