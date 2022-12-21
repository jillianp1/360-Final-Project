/************* mkdir_creat.c file **************/

#ifndef __MKDIRCREAT_C__
#define __MKDIRCREAT_C__

// #include "type.h"
//#include "util.c"

char dirName[100];
char base[100];
char temp[100];
char temp2[100];
char buf[BLKSIZE];
extern dev;

//enters a [ino, name] as a new dir entry into a parent directory
int enter_name(MINODE *pip, int ino, char *name)
{  
    //printf("in enter_name\n");
    char *cp;
    INODE *ip;
    int blk; 

    //getting inode 
    ip = &pip->INODE;

    //getting length of name
    int namelen; 
    namelen = strlen(name);

    //for each data block of prent dir
    for(int i = 0; i < 12; i++)
    {
        if(ip->i_block[i] == 0) 
        {
           // printf("in break\n");
            break; 
        }
        //get parents data block into a buf
        blk = ip->i_block[i];

        //int ideal_length = 4*((8 + namelen + 3)/ 4 );

        int need_length = 4*(( 8 + namelen + 3)/4);

        //4
        //step to the last entry in data block
        get_block(pip->dev, ip->i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf; 

        while(cp + dp->rec_len < buf + BLKSIZE)
        {
            cp += dp->rec_len;
            dp = (DIR*)cp;
        }

        // dp now points at last entry in block
        int ideal_length = 4*((8 + dp->name_len + 3)/ 4 );
        int remain = dp->rec_len - ideal_length; 

        if(remain >= need_length)
        {
            //printf("in if\n");
            //enter new entry at the last entry 
            //and trim previous entry rec len to its ideal length 
            dp->rec_len = ideal_length; //trimming
            cp += dp->rec_len;  //go to end
            dp = (DIR *)cp; //dp is at new open space

            //add inode 
            dp->inode = ino;
            strcpy(dp->name, name);
            dp->name_len = namelen;
            dp->rec_len = remain;

            printf("%s\n", dp->name);

            //write data block to disk 
            put_block(dev, blk, buf);
            return 0; 
        }
        // 5 not enough space 
        else
        {
           // printf("in else\n");
            //increment parent size by bulk size 
            ip->i_size = BLKSIZE;
            //need to allocate a new data block 
            blk = balloc(dev);
            ip->i_block[i] = blk;
            //changed so mark dirty 
            pip->dirty= 1; 
            get_block(dev, blk, buf);
            dp = (DIR *)buf; 
            cp = buf; 

            //add inode
            dp->inode = ino;
            strcpy(dp->name, name);
            dp->name_len = namelen;
            dp->rec_len = BLKSIZE;

            //write data block to disk 
            put_block(dev, blk, buf);
            return 0; 
        }
    }
}

//to create a DIR
int my_mkdir(MINODE *pmip, char *name)
{
    //printf("in my_mkdir\n");
    char *cp; 
    int pino; //parent direcotry ino

    //allocate an INODE and a disk block
    int ino = ialloc(dev);
    printf("\n\n INODE:%d\n\n", ino);
    int blk = balloc(dev);

    //load INODE into a minode
    MINODE *mip = iget(dev, ino); 
    INODE *ip = &mip->INODE;

    ip->i_mode = 0x41ED;
    ip->i_uid = running->uid;
    ip->i_gid = running->gid;
    ip->i_size = BLKSIZE;
    ip->i_links_count = 2;  //links count is 2 becasue . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2; //linux blocks count in 512 byte chunks
    mip->refCount = 0;
   
    ip->i_block[0] = blk; //new dir has one data block
    for(int i = 1; i < 15; i++)
    {
        ip->i_block[i] = 0;
    }

     //mark minode modified(dirty)
    mip->dirty = 1; 
    //write INODE back to disk 
    iput(mip);

    //bzero(buf, BLKSIZE);

    get_block(dev, blk, buf);
    dp = (DIR *)buf;
    cp = buf; 

    //creating data block for new DIR containing . and .. entries
    //make data block 0 of INODE to contain . and .. entries
   // char buf[BLKSIZE];
    //bzero(buf, BLKSIZE);
    //DIR *dp = (DIR*)buf;

    printf("creating . and ..\n");
    //make . entry
    dp->inode = ino; 
    dp->rec_len = 12;
    dp->name_len = 1; 
    dp->name[0] = '.';

    cp+= dp->rec_len; 
    dp = (DIR *)cp;

    //make .. entry pino = parent DIR ino
    //blk = allocated block
    //dp = (char *)dp + 12; 
    //dp->inode = pino;
    dp->inode = pmip->ino;
    dp->rec_len = BLKSIZE-12;
    dp->name_len = 2; 
    dp->name[0] = '.';
    dp->name[1] = '.';

    //write to disk block blk
   // put_block(mip->dev, ip->i_block[0], buf);
    put_block(dev, blk, buf);

    //enters (ino, basename) as a dir_entry to the paretn INODE
    enter_name(pmip, ino, name);

    return 0;
}

int make_dir(char *pathname)
{
    MINODE *start; 
    //divide pathname into dirname an dbasename
    strcpy(temp, pathname);
    strcpy (base, basename(temp));
    printf("basename = %s\n", base);
    strcpy(temp2, pathname);
    strcpy(dirName, dirname(temp2));
    printf("dirname = %s\n", dirName);

    if(pathname[0] == '/')
    {
        start = root; 
        dev = root->dev;
    }
    else
    {
        start = running->cwd;
        dev = running->cwd->dev;
    }

    int pino = getino(dirName);
    printf("parent ino: %d\n", pino);
    if(pino == -1)
    {
        printf("error doesnt exist\n");
        return -1; 
    }

    //getting inode of parent 
    MINODE *pmip = iget(dev, pino);

    // check pmip INODE is a DIR
    if (!S_ISDIR(pmip->INODE.i_mode))
    {
         printf("error minode type not DIR\n");
         return -1;
    }
    //basename cannot exist in parent DIR
    if(search(pmip, base)!= 0) //must return 0
    {
        printf("error basename already exists in parent dir\n");
        return -1; 
    }

    my_mkdir(pmip, base);

    //increment parent INODE link count by 1 
    pmip->INODE.i_links_count++;
    //set time
    pmip->INODE.i_atime = time(0L);
    // mark pmip dirty 
    pmip->dirty = 1;

    iput(pmip);

    return 0;
}

int my_creat(MINODE *pip, char *name)
{
    MINODE *mip; 
     //allocate an INODE and a disk block
    int ino = ialloc(dev);
    int blk = balloc(dev);

    //load INODE into a minode
    mip = iget(dev, ino); 
    INODE *ip = &mip->INODE;

    // set mode to reg 
    ip->i_mode = 0x81A4;
    ip->i_uid = running->uid;
    ip->i_gid = running->gid;
    //file size is 0 
    ip->i_size = BLKSIZE;
    // link count is 1
    ip->i_links_count = 1;  
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2; //linux blocks count in 512 byte chunks
    ip->i_block[0] = 0; 

    for(int i = 1; i < 15; i++)
    {
        ip->i_block[i] = 0;
    }
     //mark minode modified(dirty)
    mip->dirty = 1; 
    //write INODE back to disk 
    iput(mip);

    enter_name(pip, ino, name);

    return 0;
}

int creat_file(char *pathname)
{
    // printf("in creat file\n");
    //int pino;
    //MINODE *pmip;
    MINODE *start; 

    //divide pathname into dirname an dbasename
    strcpy(temp, pathname);
    strcpy (base, basename(temp));
    printf("basename = %s\n", base);
    strcpy(temp2, pathname);
    strcpy(dirName, dirname(temp2));
    printf("dirname = %s\n", dirName);

    if(pathname[0] == '/')
    {
        start = root; 
        dev = root->dev;
    }
    else
    {
        start = running->cwd;
        dev = running->cwd->dev;
    }

    int pino = getino(dirName);

    if(pino == -1)
    {
        printf("error doesnt exist\n");
        return -1; 
    }

     //getting inode of parent 
    MINODE *pmip = iget(dev, pino);

    // check pmip INODE is a DIR
    if (!S_ISDIR(pmip->INODE.i_mode))
    {
         printf("error minode type not DIR\n");
         return -1;
    }
    //basename cannot exist in parent DIR

    if(search(pmip, base) != 0) //must return 0
    {
        printf("error basename already exists in parent dir\n");
        return -1; 
    }

    my_creat(pmip, base);
    //set time
    pmip->INODE.i_atime = time(0L);
    //mark as changed
    pmip->dirty = 1; 
    iput(pmip);
    return 0;
}

#endif