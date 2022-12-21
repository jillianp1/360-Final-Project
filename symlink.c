/************* symlink.c file **************/

#ifndef __SYMLINK_C__
#define __SYMLINK_C__

#include "mkdir_creat.c"


int symlink_file(char *old_file, char *new_file)
{
  MINODE *mip; 
  //setting
  if(old_file[0] == '/')
  {
      dev = root->dev;
  }
  else
  {
      dev = running->cwd->dev;
  }

  //check old_file must exist 
  //pathname: symlink old_file new_file
  int oino = getino(old_file);
  if(oino == -1)
  {
    printf("old file does not exist\n");
    return -1; 
  }

  //setting new
  if(new_file[0] == '/')
  {
      dev = root->dev;
  }
  else
  {
      dev = running->cwd->dev;
  }
   //creat new file
  creat_file(new_file);
  //and new file does not exist 
  int nino = getino(new_file);
  if(nino == -1)
  {
      printf("error new file does not exist\n");
      return -1; 
  }
  //creat new file
  //creat_file(new_file);
  //change new file to LNK type
  // check you created file
  
  mip = iget(dev, nino);
  //change new file LNK type
  mip->INODE.i_mode = 0xA1FF;
  mip->dirty = 1; 
  
  //store old file name in new files block area
  strncpy(mip->INODE.i_block, old_file, 84);

  //set file size to length of old file name
  mip->INODE.i_size = strlen(old_file) + 1;
  
  //mark new files minode dirty 
  mip->dirty = 1; 
  // iput new files minode 
  iput(mip);

  

}

int my_readlink(char *pathname)
{
    return 1;
}

#endif