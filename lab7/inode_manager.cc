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
  // may have a bug
  blockid_t firstDataBlock = IBLOCK(sbPointer->ninodes, sbPointer->nblocks) + 1;
  // The last data bitmap block
  blockid_t maxNum = BBLOCK(sbPointer->nblocks);
  // the first data bitmap block
  blockid_t minNum = BBLOCK(firstDataBlock);
  // No new because don't need construt
  // save the bitmap in the block
  char *bitMapOneBlock = (char *)malloc(BLOCK_SIZE);
  // first time the bit position of block firstDataBlock's flag(numberd from the beginning of the block)
  uint32_t bufBit = firstDataBlock % (BLOCK_SIZE * 8);
  for (blockid_t bufBlock = minNum; bufBlock <= maxNum; bufBlock++)
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
// maybe wrong
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
  for (uint32_t i = 1; i <= INODE_NUM; i++)
  {
    // uint32_t bufInodeI = IBLOCK(i, sbPointer->nblocks);

    // maybe Memory leap
    struct inode *node = get_inode(i);
    if (node == NULL)
    {
      struct inode buf;
      buf.type = type;
      buf.size = 0;
      time_t currentTime;
      buf.ctime = time(&currentTime);
      buf.mtime = time(&currentTime);
      buf.atime = time(&currentTime);
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
// err
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size)//why use pointer~?fxxk
{
  /*
   * your lab1 code goes here.
   * note: read blocks related to inode number inum,
   * and copy them to buf_Out
   */


  //calculate the direct num and the indirect num
  struct inode *node = get_inode(inum);
  uint32_t blockNum = 0;
  if (node == NULL)
  {
    fprintf(stderr, "personal err: inode_manager::read_file -> node is NULL\n");
    return;
  }
  *size = node->size;
  // printf("The reading size: %d\n", node->size);

  *buf_out = (char *)malloc(node->size);

  // only use the direct blocks
  if (node->size <= NDIRECT * BLOCK_SIZE)
  {
    // deal with direct blocks(32)
    char *bp = (char*)malloc(BLOCK_SIZE);
    uint32_t off = 0;
    for (; off < (node->size); off += BLOCK_SIZE)
    {
      bm->read_block(node->blocks[off / BLOCK_SIZE], bp);
      memcpy(*buf_out + off, bp, MIN(BLOCK_SIZE, node->size - off));
    }

    delete[] bp;
    delete node;
    return;
  }
  // use the indirect blocks
  else
  {
    // deal with direct blocks(32)
    uint32_t off = 0;
    char *bp = (char *)malloc(BLOCK_SIZE);
    for (; off < NDIRECT * BLOCK_SIZE; off += BLOCK_SIZE)
    {
      bm->read_block(node->blocks[off / BLOCK_SIZE], bp);
      memcpy(*buf_out + off, bp, MIN(BLOCK_SIZE, node->size - off));
    }

    // deal with indirect block(1)
    // save the block
    blockid_t *bufIndirect = (blockid_t *)malloc(BLOCK_SIZE);
    bm->read_block(node->blocks[NDIRECT], (char *)bufIndirect);
    off = NDIRECT * BLOCK_SIZE;
    for (; off < (node->size); off += BLOCK_SIZE)
    {
      bm->read_block(bufIndirect[(off - NDIRECT * BLOCK_SIZE) / BLOCK_SIZE], bp);
      memcpy(*buf_out + off, bp, MIN(BLOCK_SIZE, node->size - off));
    }

    delete[] bp;
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
  uint32_t blockNum;
  if (node->size % BLOCK_SIZE == 0)
  {
    blockNum = node->size / BLOCK_SIZE;
  }
  else
  {
    blockNum = node->size / BLOCK_SIZE + 1;
  }

  // free all
  if (blockNum <= NDIRECT)
  {
    for (uint32_t i = 0; i < blockNum; i++)
    {
      bm->free_block(node->blocks[i]);
      node->blocks[i] = 0;
    }
  }
  else
  {
    for (uint32_t i = 0; i < NDIRECT; i++)
    {
      bm->free_block(node->blocks[i]);
      node->blocks[i] = 0;
    }
    blockid_t *bufIndirect = (blockid_t *)malloc(BLOCK_SIZE);
    bm->read_block(node->blocks[NDIRECT], (char *)bufIndirect);
    for (uint32_t i = 0; i < blockNum - NDIRECT; i++)
    {
      bm->free_block(bufIndirect[i]);
    }
    bm->free_block(node->blocks[NDIRECT]);
    node->blocks[NDIRECT] = 0;
  }

  //only use direct blocks
  if (size <= NDIRECT * BLOCK_SIZE)
  {
    char *bp = (char*)malloc(BLOCK_SIZE);
    uint32_t off = 0;
    for (; off < size; off += BLOCK_SIZE)
    {
      node->blocks[off / BLOCK_SIZE] = bm->alloc_block();
      bzero(bp,BLOCK_SIZE);
      memcpy(bp, buf + off, MIN(BLOCK_SIZE, size - off));
      bm->write_block(node->blocks[off / BLOCK_SIZE], bp);
    }
    delete[] bp;
  }
  //use indirect block
  else
  {
    char *bp = (char*)malloc(BLOCK_SIZE);
    uint32_t off = 0;
    for (; off < NDIRECT * BLOCK_SIZE; off += BLOCK_SIZE)
    {
      node->blocks[off / BLOCK_SIZE] = bm->alloc_block();
      bzero(bp,BLOCK_SIZE);
      memcpy(bp, buf + off, BLOCK_SIZE);
      bm->write_block(node->blocks[off / BLOCK_SIZE], bp);
    }

    blockid_t *bufIndirect = (blockid_t *)malloc(BLOCK_SIZE);
    node->blocks[NDIRECT] = bm->alloc_block();
    for (; off < size; off += BLOCK_SIZE)
    {
      bufIndirect[(off - NDIRECT * BLOCK_SIZE) / BLOCK_SIZE] = bm->alloc_block();
      bzero(bp,BLOCK_SIZE);
      memcpy(bp, buf + off, MIN(BLOCK_SIZE, size - off));
      bm->write_block(bufIndirect[(off - NDIRECT * BLOCK_SIZE) / BLOCK_SIZE], bp);
    }
    bm->write_block(node->blocks[NDIRECT], (char *)bufIndirect);
    delete[] bp;
  }
  node->size = size;
  time_t currentTime;
  node->ctime = time(&currentTime);
  node->mtime = time(&currentTime);
  put_inode(inum, node);
  delete node;
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

void inode_manager::setattr(uint32_t inum, extent_protocol::attr &a)
{
  struct inode *node = get_inode(inum);
  if (node != NULL)
  {
    node->type = a.type;
    node->atime = a.atime;
    node->mtime = a.mtime;
    node->ctime = a.ctime;
    node->size = a.size;
    put_inode(inum, node);
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
    for (uint32_t i = 0; i < blockNum; i++)
    {
      bm->free_block(node->blocks[i]);
    }
  }
  else
  {
    for (uint32_t i = 0; i < NDIRECT; i++)
    {
      bm->free_block(node->blocks[i]);
    }
    blockid_t *buf = (blockid_t *)malloc(BLOCK_SIZE);
    bm->read_block(node->blocks[NDIRECT], (char *)buf);
    for (uint32_t i = 0; i < blockNum - NDIRECT; i++)
    {
      bm->free_block(buf[i]);
    }
    bm->free_block(node->blocks[NDIRECT]);
    delete[] buf;
  }
  free_inode(inum);
  return;
}
