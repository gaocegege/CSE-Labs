#include "inode_manager.h"
#include <stdio.h>
// disk layer -----------------------------------------

disk::disk()
{
  bzero(blocks, sizeof(blocks));
}

void
disk::read_block(blockid_t id, char *buf)
{
  if (id < 0 || id >= BLOCK_NUM || buf == NULL)
    return;

  memcpy(buf, blocks[id], BLOCK_SIZE);
}

void
disk::write_block(blockid_t id, const char *buf)
{
  if (id < 0 || id >= BLOCK_NUM || buf == NULL)
    return;

  memcpy(blocks[id], buf, BLOCK_SIZE);
}

// block layer -----------------------------------------

// Allocate a free disk block.
blockid_t
block_manager::alloc_block()
{
  /*
   * your lab1 code goes here.
   * note: you should mark the corresponding bit in block bitmap when alloc.
   * you need to think about which block you can start to be allocated.
   */
  struct superblock *sbPointer = &sb;
  // The first data block
  blockid_t firstDataBlock = IBLOCK(sbPointer->ninodes, sbPointer->nblocks);
  // The last data bitmap block
  blockid_t maxNum = BBLOCK(sbPointer->nblocks);
  // the first data bitmap block
  blockid_t minNum = BBLOCK(firstDataBlock);
  // No new because don't need construt
  // save the bitmap in the block
  char *bitMapOneBlock = (char *)malloc(BLOCK_SIZE);
  // first time the bit position of block firstDataBlock's flag(numberd from the beginning of the block)
  uint32_t bufBit = firstDataBlock % (BLOCK_SIZE * 8);
  for (blockid_t bufBlock = minNum; bufBlock < maxNum; bufBlock++)
  {
    // read the bitmap block
    d->read_block(bufBlock, bitMapOneBlock);
    for (; bufBit < (BLOCK_SIZE * 8); bufBit++)
    {
      // byte position of the block's flag
      uint32_t bytePos = bufBit / 8;
      // bit position of the block's flag
      uint32_t bitPos = bufBit - (8 * bytePos);
      char bufByte = bitMapOneBlock[bytePos];
      bool flagBit = 0;
      flagBit = (bufByte >> (7 - bitPos)) & 0x1;
      if (flagBit == 1)
        continue;
      else
      {
        // flag bit become 1, others don't change
        bufByte = (bufByte) | (0x1 << (7 - bitPos));
        // write back
        bitMapOneBlock[bytePos] = bufByte;
        d->write_block(bufBlock, bitMapOneBlock);
        // end
        delete[] bitMapOneBlock;
        return (bufBlock - 2) * (8 * BLOCK_SIZE) + bufBit;
      }
    }

    // the next time should begin with 0 bit
    bufBit = 0;
  }
  delete[] bitMapOneBlock;
  return 0;
}

void
block_manager::free_block(uint32_t id)
{
  /* 
   * your lab1 code goes here.
   * note: you should unmark the corresponding bit in the block bitmap when free.
   */
  // free the block and set the flag to zero
  blockid_t bitmapBlock = BBLOCK(id);
  char *bitMapOneBlock = (char *)malloc(BLOCK_SIZE);
  d->read_block(bitmapBlock, bitMapOneBlock);
  uint32_t bufBit = id % (8 * BLOCK_SIZE);
  uint32_t bytePos = bufBit / 8;
  uint32_t bitPos = bufBit - (8 * bytePos);
  char bufByte = bitMapOneBlock[bytePos];
  bufByte = bufByte & (~(0x1 << (7 - bitPos)));
  bitMapOneBlock[bytePos] = bufByte;
  d->write_block(bitmapBlock, bitMapOneBlock);
  delete[] bitMapOneBlock;
  return;
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager()
{
  d = new disk();

  // format the disk
  // size of all the disk
  sb.size = BLOCK_SIZE * BLOCK_NUM;
  // number of all the block
  sb.nblocks = BLOCK_NUM;
  // number of inode(1024)
  sb.ninodes = INODE_NUM;

  // maybe have a bug
  // changed
  // write the superblock to the disk
  d->write_block(1, (char *)&sb);
}

void
block_manager::read_block(uint32_t id, char *buf)
{
  d->read_block(id, buf);
}

void
block_manager::write_block(uint32_t id, const char *buf)
{
  d->write_block(id, buf);
}

// inode layer -----------------------------------------

inode_manager::inode_manager()
{
  bm = new block_manager();
  uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
  if (root_dir != 1) {
    printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
    exit(0);
  }
}

/* Create a new file.
 * Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type)
{
  /* 
   * your lab1 code goes here.
   * note: the normal inode block should begin from the 2nd inode block.
   * so the IBLOCK is "+3"
   * the 1st is used for root_dir, see inode_manager::inode_manager().
   */
  // struct superblock* sbPointer = &sb;
  for (uint32_t i = 1; i < INODE_NUM; i++)
  {
    // uint32_t bufInodeI = IBLOCK(i, sbPointer->nblocks);

    // maybe Memory leap
    struct inode *node = get_inode(i);
    if (node == NULL)
    {
      struct inode buf;
      buf.type = type;
      buf.size = buf.atime = buf.mtime = buf.ctime = 0;
      put_inode(i, &buf);
      return i;
    }
    else
    {
      delete[] node;
      // printf("personal notice: inode_manager::alloc_inode -> node is NULL\n");
      continue;
    }
  }
  fprintf(stderr, "personal err: inode_manager::alloc_inode -> inode is all used(max size = 1024)\n");
  exit(0);
}

// haven't released the data block 
// two ideas: 1.  free the inode 
//            2.  node->type = 0
// now use idea 2
// maybe have problems
void
inode_manager::free_inode(uint32_t inum)
{
  /* 
   * your lab1 code goes here.
   * note: you need to check if the inode is already a freed one;
   * if not, clear it, and remember to write back to disk.
   */
  struct inode *node = get_inode(inum);
  if (node != NULL)
  {
    if (node->type == 1 || node->type == 2)
    {
      node->type = 0;
      node->size = node->atime = node->ctime  = node->mtime = 0;
      put_inode(inum, node);
    }
  }
  else
  {
    fprintf(stderr, "personal notice: inode_manager::free_inode -> node is NULL, the node is empty\n");
  }
  return;
}


/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode* 
inode_manager::get_inode(uint32_t inum)
{
  struct inode *ino, *ino_disk;
  char buf[BLOCK_SIZE];

  printf("\tim: get_inode %d\n", inum);

  if (inum < 0 || inum >= INODE_NUM) {
    printf("\tim: inum out of range\n");
    return NULL;
  }

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  // printf("%s:%d\n", __FILE__, __LINE__);

  ino_disk = (struct inode*)buf + inum%IPB;
  // what the fxxk~?
  if (ino_disk->type == 0) {
    printf("\tim: inode not exist\n");
    return NULL;
  }

  ino = (struct inode*)malloc(sizeof(struct inode));
  *ino = *ino_disk;

  return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino)
{
  char buf[BLOCK_SIZE];
  struct inode *ino_disk;

  printf("\tim: put_inode %d\n", inum);
  if (ino == NULL)
    return;

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  ino_disk = (struct inode*)buf + inum%IPB;
  *ino_disk = *ino;
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a,b) ((a)<(b) ? (a) : (b))

/* Get all the data of a file by inum. 
 * Return alloced data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size)//why use pointer~?fxxk
{
  /*
   * your lab1 code goes here.
   * note: read blocks related to inode number inum,
   * and copy them to buf_Out
   */
  // reference: http://blog.chinaunix.net/uid-7471615-id-83766.html
  // printf("start get\n");
  struct inode *node = get_inode(inum);
  uint32_t blockNum = 0;
  if (node == NULL)
  {
    fprintf(stderr, "personal err: inode_manager::read_file -> node is NULL\n");
    return;
  }
  *size = node->size;
  // printf("The reading size: %d\n", node->size);

  // calculate the blocks that the file has
  if ((*size) % BLOCK_SIZE == 0)
  {
    blockNum = (*size) / BLOCK_SIZE;
  }
  else
  {
    blockNum = (*size) / BLOCK_SIZE + 1;
  }
  *buf_out = (char *)malloc(blockNum * BLOCK_SIZE);

  // only use the direct blocks
  if (NDIRECT >= blockNum)
  {
    // deal with direct blocks(32)
    for (uint32_t i = 0; i < blockNum; i++)
    {
      bm->read_block(node->blocks[i], *buf_out + i * BLOCK_SIZE);
    }

    delete node;
    // printf("end get\n");
    return;
  }
  // use the indirect blocks
  else
  {
    // deal with direct blocks(32)
    for (uint32_t i = 0; i < NDIRECT; i++)
    {
      bm->read_block(node->blocks[i], *buf_out + i * BLOCK_SIZE);
    }

    // deal with indirect block(1)
    // save the block
    blockid_t *bufIndirect = (blockid_t *)malloc(BLOCK_SIZE);
    bm->read_block(node->blocks[NDIRECT], (char *)bufIndirect);
    for (uint32_t i = 0; i < blockNum - NDIRECT; i++)
    {
      bm->read_block(bufIndirect[i], *buf_out + NDIRECT * BLOCK_SIZE + i * BLOCK_SIZE);
    }

    // printf("end get\n");
    delete[] bufIndirect;
    delete node;
    return;
  }
}

/* alloc/free blocks if needed */
void
inode_manager::write_file(uint32_t inum, const char *buf, int size)
{
  /*
   * your lab1 code goes here.
   * note: write buf to blocks of inode inum.
   * you need to consider the situation when the size of buf 
   * is larger or smaller than the size of original inode
   */
  struct inode *node = get_inode(inum);
  uint32_t newNum = 0;
  uint32_t originalNum = 0;
  if (node == NULL)
  {
    fprintf(stderr, "personal err: inode_manager::write_file -> node is NULL\n");
    return;
  }

  //calculate the size of the old file
  if (node->size % BLOCK_SIZE == 0)
  {
    originalNum = node->size / BLOCK_SIZE;
  }
  else
  {
    originalNum = node->size / BLOCK_SIZE + 1;
  }

  // calculate the size of the new file
  if (size % BLOCK_SIZE == 0)
  {
    newNum = size / BLOCK_SIZE;
  }
  else
  {
    newNum = size / BLOCK_SIZE + 1;
  }
  // fprintf(stderr, "buf: %s, newNum: %d, originalNum: %d\n", buf, newNum, originalNum);
  // the size < the inode's size
  if (newNum <= originalNum)
  {
    if (originalNum <= NDIRECT)
    {
      for (uint32_t i = 0; i < originalNum - newNum; i++)
      {
        bm->free_block(node->blocks[newNum + i]);
      }

      //write blocks
      for (uint32_t i = 0; i < newNum; i++)
      {
        bm->write_block(node->blocks[i], buf + i * BLOCK_SIZE);
      }
      node->size = size;
      put_inode(inum, node);
      delete node;
      return;
    }
    else
    {
      // originalNum > NDIRECT && newNum <= NDIRECT
      if (newNum <= NDIRECT)
      {
        for (uint32_t i = 0; i < NDIRECT - newNum; i++)
        {
          bm->free_block(node->blocks[newNum + i]);
        }

        blockid_t *bufIndirect = (blockid_t *)malloc(BLOCK_SIZE);
        bm->read_block(node->blocks[NDIRECT], (char *)bufIndirect);
        for (uint32_t i = 0; i < originalNum - NDIRECT; i++)
        {
          bm->free_block(bufIndirect[i]);
        }
        bm->free_block(node->blocks[NDIRECT]);

         //write blocks
        for (uint32_t i = 0; i < newNum; i++)
        {
          bm->write_block(node->blocks[i], buf + i * BLOCK_SIZE);
        }
        node->size = size;
        put_inode(inum, node);
        delete node;
        return;
      }
      // originalNum > NDIRECT && newNum > NDIRECT
      else
      {
        blockid_t *bufIndirect = (blockid_t *)malloc(BLOCK_SIZE);
        bm->read_block(node->blocks[NDIRECT], (char *)bufIndirect);
        for (uint32_t i = 0; i < originalNum - newNum; i++)
        {
          bm->free_block(bufIndirect[newNum + i]);
        }

        // write direct blocks
        for (uint32_t i = 0; i < NDIRECT; i++)
        {
          bm->write_block(node->blocks[i], buf + i * BLOCK_SIZE);
        }
        for (uint32_t i = 0; i < newNum - NDIRECT; i++)
        {
          bm->write_block(bufIndirect[i], buf + (NDIRECT * BLOCK_SIZE) + i * BLOCK_SIZE);
        }

        bm->write_block(node->blocks[NDIRECT], (char *)bufIndirect);
        node->size = size;
        put_inode(inum, node);
        delete node;
        return;
      }
    }
  }
  // the size > the node's size
  else
  {
    // only use the direct blocks
    if (NDIRECT >= newNum)
    {
      // deal with direct blocks(32)
      for (uint32_t i = 0; i < newNum - originalNum; i++)
      {
        node->blocks[originalNum + i] = bm->alloc_block();
      }

      // write blocks
      for (uint32_t i = 0; i < newNum; i++)
      {
        bm->write_block(node->blocks[i], buf + i * BLOCK_SIZE);
      }
      node->size = size;
      put_inode(inum, node);
      delete node;
      return;
    }
    // use the indirect blocks
    else
    {
      // newNum >= NDIRECT && originalNum <= NDIRECT
      if (NDIRECT >= originalNum)
      {
        for (uint32_t i = 0; i < NDIRECT - originalNum; i++)
        {
          node->blocks[originalNum + i] = bm->alloc_block();
        }

        blockid_t *bufIndirect = (blockid_t *)malloc(BLOCK_SIZE);
        node->blocks[NDIRECT] = bm->alloc_block();
        for (uint32_t i = 0; i < newNum - NDIRECT; i++)
        {
          bufIndirect[i] = bm->alloc_block();
        }

        // write direct blocks
        for (uint32_t i = 0; i < NDIRECT; i++)
        {
          bm->write_block(node->blocks[i], buf + i * BLOCK_SIZE);
        }
        for (uint32_t i = 0; i < newNum - NDIRECT; i++)
        {
          bm->write_block(bufIndirect[i], buf + (NDIRECT * BLOCK_SIZE) + i * BLOCK_SIZE);
        }

        bm->write_block(node->blocks[NDIRECT], (char *)bufIndirect);
        node->size = size;
        put_inode(inum, node);
        delete[] bufIndirect;
        delete node;
        return;
      }
      // newNum >= NDIRECT && originalNum >= NDIRECT
      else
      {
        blockid_t *bufIndirect = (blockid_t *)malloc(BLOCK_SIZE);
        for (uint32_t i = 0; i < newNum - originalNum; i++)
        {
          bufIndirect[originalNum + i] = bm->alloc_block();
        }

        // write direct blocks
        for (uint32_t i = 0; i < NDIRECT; i++)
        {
          bm->write_block(node->blocks[i], buf + i * BLOCK_SIZE);
        }
        for (uint32_t i = 0; i < newNum - NDIRECT; i++)
        {
          bm->write_block(bufIndirect[i], buf + (NDIRECT * BLOCK_SIZE) + i * BLOCK_SIZE);
        }

        bm->write_block(node->blocks[NDIRECT], (char *)bufIndirect);
        node->size = size;
        put_inode(inum, node);
        delete[] bufIndirect;
        delete node;
        return;
      }
    }
  }
  return;
}

void
inode_manager::getattr(uint32_t inum, extent_protocol::attr &a)
{
  /*
   * your lab1 code goes here.
   * note: get the attributes of inode inum.
   * you can refer to "struct attr" in extent_protocol.h
   */
  // struct extent_protocol::attr result = new extent_protocol::attr;

  struct inode *node = get_inode(inum);
  if (node != NULL)
  {
    a.type = node->type;
    a.atime = node->atime;
    a.mtime = node->mtime;
    a.ctime = node->ctime;
    a.size = node->size;
    delete node;
  }
  else
  {
    a.type = 0;
    a.size = 0;
    printf("getAttr personal notice: node is NULL, the inode is empty\n");
  }
  return;
}

void
inode_manager::remove_file(uint32_t inum)
{
  /*
   * your lab1 code goes here
   * note: you need to consider about both the data block and inode of the file
   */
  struct inode *node = get_inode(inum);
  uint32_t blockNum = 0;

  if (node->size % BLOCK_SIZE == 0)
  {
    blockNum = node->size / BLOCK_SIZE;
  }
  else
  {
    blockNum = node->size / BLOCK_SIZE + 1;
  }

  // don't have the indirect block
  if (blockNum <= NDIRECT)
  {
    free_inode(inum);
  }
  else
  {
    blockid_t *buf = (blockid_t *)malloc(BLOCK_SIZE);
    bm->read_block(node->blocks[NDIRECT], (char *)buf);
    for (uint32_t i = 0; i < blockNum - NDIRECT; i++)
    {
      bm->free_block(buf[i]);
    }
  }
  return;
}
