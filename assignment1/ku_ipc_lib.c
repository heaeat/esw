#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#include "ku_ipc.h"

#define SUCCESS		0
#define FAIL		-1

int ku_ipc_msgget(int key, int msgflg);				 
int ku_msgclose(int msgid);						 
int ku_msgsnd(int msgid, void* msgp, int msgsz, int msgflg);		 
int ku_msgrcv(int msgid, void* msgp, int msgz, long msgtyp, int msgflg); 

int ku_ipc_msgget(int key, int msgflg) {
	int msgqid = 0;
	int dev = open(DEV_DIR, O_RDWR);

	if(dev < 0) {
		return FAIL;
	}

	msgqid = ioctl(dev, IPC_IOCTL_MSQ_CHECK, &key);
	if(msgqid > 0) {
		if(msgflg == IPC_EXCL) {
			msgqid = FAIL;
		}
	} else {
		msgqid = ioctl(dev, IPC_IOCTL_MSQ_CREATE, &key);
	}

	close(dev);

	return msgqid;
}

int ku_msgclose(int msqid) {
	int rtn = -1;
	int dev = open(DEV_DIR, O_RDWR);

	rtn = ioctl(dev, IPC_IOCTL_MSQ_CHECK, &msqid);
	if(rtn > 0) {
		msqid = ioctl(dev, IPC_IOCTL_MSQ_CLOSE, &msqid); 
		rtn = 0;
	}

	close(dev);

	if(rtn < 0) {
		return FAIL;
	} else {
		return SUCCESS;

	}
}

int ku_msgsnd(int msqid, void* msgp, int msgsz, int msgflg) {
	int dev = open(DEV_DIR, O_RDWR);
	int rtn = -1;
	struct mybuf* buf = NULL;	
	
	if(ioctl(dev, IPC_IOCTL_MSQ_CHECK, &msqid) < 0) {
		close(dev);
		return FAIL;
	};

	buf = (struct mybuf*)malloc(sizeof(struct mybuf));	
	buf->msqid = msqid;
	buf->msgflg = msgflg;
	buf->msgsz = msgsz;
	buf->msgp = msgp;

	rtn = write(dev, buf, msgsz);
	if(rtn != 0) {
		if((msgflg & IPC_NOWAIT) == 0) {
			while(rtn != 0 && rtn != IPC_RES_NOMSQ) {
				rtn = write(dev, buf, msgsz);
			}

			free(buf);
			close(dev);

			if(rtn == IPC_RES_NOMSQ) {
				return FAIL;
			} else {
				return SUCCESS;
			}
		}	
	}

	free(buf);
	close(dev);
	return SUCCESS;
}

int ku_msgrcv(int msqid, void* msgp, int msgsz, long msgtype, int msgflg) {
	int dev = open(DEV_DIR, O_RDWR);
	int len = 0;
	struct mybuf* buf = NULL;

	if(ioctl(dev, IPC_IOCTL_MSQ_CHECK, &msqid) < 0) {
		close(dev);
		return FAIL;
	};

	buf = (struct mybuf*)malloc(sizeof(struct mybuf));	
	buf->msqid = msqid;
	buf->msgflg = msgflg;
	buf->msgsz = msgsz;
	buf->msgp = msgp;
	buf->msgp->type = msgtype;
	
	len = read(dev, buf, msgsz);
	if(len < 0) {
		if((msgflg & IPC_NOWAIT) == 0) {
			while(len < 0 && len != IPC_RES_NOMSQ && len != IPC_RES_SHORT) {
				len = read(dev, buf, msgsz);
			}

			if(len == IPC_RES_NOMSQ || len == IPC_RES_SHORT) {
				free(buf);
				close(dev);

				return FAIL;
			}
		} else {
			free(buf);
			close(dev);

			return FAIL;
		}
	}

	free(buf);
	close(dev);
	
	return len;
}
