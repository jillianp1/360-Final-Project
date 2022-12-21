#ifndef __READCAT_C__
#define __READCAT_C__

#include "open_close.c"
int read_file() {
    int fd = 0;
    int nbytes = 0;
    printf("Input a fd (file descriptor): \n");
    //fgets(line, 128, stdin);
    scanf("%d", &fd);
    printf("fd entered = %d\n", fd);
    if (!is_valid_fd(fd)) {
        printf("Error: Invalid fd\n");
        return -1;
    }
    printf("Input number of bytes to read: \n");
    scanf("%d", &nbytes);

    printf("running fd mode =  %d\n", running->fd[fd]->mode);

    if (running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2) {
        printf("Error: fd not open for R/RW\n");
        return -1;
    }

    char buf[BLKSIZE];
    return my_read(fd, buf, nbytes);
}
int my_read(int fd, char* buf, int nbytes) {
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->minodePtr;
    // bytes available in file
    int avil = mip->INODE.i_size - oftp->offset;
    int lbk, start, blk, count = 0;
    int ibuf[BLKSIZE];
    int jbuf[BLKSIZE];

    if (nbytes > avil)
        nbytes = avil;
    while (nbytes && avil) {
        // logical block
        lbk = oftp->offset / BLKSIZE;
        // start byte in block
        start = oftp->offset % BLKSIZE;
        // convert logical block number lbk to physical block number
        // page# 348
        // direct block
        if (lbk < 12) {
            // read block < 12
            blk = mip->INODE.i_block[lbk];
        }
        // indirect block
        else if (lbk >= 12 && lbk < 256 + 12) {
            // read block 12
            get_block(mip->dev, mip->INODE.i_block[12], ibuf);
            blk = ibuf[lbk-12];
        }
        // double indirect blocks
        else {
            // read block 13
            get_block(mip->dev, mip->INODE.i_block[13], jbuf);
            lbk = lbk - 256 - 12;
            // divide by length to get the block
            blk = jbuf[lbk / 256];
            get_block(mip->dev, blk, ibuf);
            blk = ibuf[lbk%256];
        }

       
        char kbuf[BLKSIZE];
        get_block(mip->dev, blk, kbuf);
        char *cp = kbuf + start;
        int remain = BLKSIZE - start;
        char *temp_buf = buf;
        if (nbytes > remain) {
            memcpy(temp_buf, cp, remain);
            cp += remain;
            buf += remain;
            oftp->offset += remain;
            count += remain;
            avil -= remain;
            nbytes -= remain;
            remain = 0;
        }
        else {
            memcpy(temp_buf, cp, nbytes);
            cp += nbytes;
            buf += nbytes;
            oftp->offset += nbytes;
            count += nbytes;
            avil -= nbytes;
            remain -= nbytes;
            nbytes = 0;
        }
    }
    //printf("**************************************\n");
    //printf("Exit my_read read %d char from file %d\n", count, fd);
    return count;
}

int my_cat(char* pathname) {
    printf("enter cat\n");
    char mybuf[BLKSIZE], temp[BLKSIZE];
    int n;
    strncpy(temp, pathname, 1024);
    int fd = open_file(temp, 0);
    while (n = my_read(fd, mybuf, BLKSIZE)) {
        mybuf[n] = 0;
        char *c = mybuf;
        while (*c != '\0') {
            if (*c == '\n') {
                printf("\n");
            }
            else {
                printf("%c", *c);
            }
            c++;
        }
    }
    my_close(fd);
    printf("exit cat\n");
    return 0; 
}


#endif