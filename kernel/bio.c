// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct bucket {
  struct spinlock lock;
  struct buf head;
};

struct {
  struct spinlock lock;

  struct buf buf[NBUF];

  struct bucket bucket[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;
  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  for (int i = 0; i < NBUCKET; ++i) {
    initlock(&bcache.bucket[i].lock, "bucket");
    bcache.bucket[i].head.next = &(bcache.bucket[i].head);
    for (b = bcache.buf + i; b < bcache.buf + NBUF; b += NBUCKET) {
      initsleeplock(&b->lock, "buffer");
      b->next = bcache.bucket[i].head.next;
      bcache.bucket[i].head.next = b;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id = blockno % NBUCKET;

  acquire(&bcache.bucket[id].lock);

  // Is the block already cached?
  for (b = bcache.bucket[id].head.next; b != &bcache.bucket[id].head; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.bucket[id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  for (b = bcache.bucket[id].head.next; b != &bcache.bucket[id].head; b = b->next) {
    if (b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.bucket[id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bucket[id].lock);

  // No buffers this bucket
  acquire(&bcache.lock);
  for (int j = 1; j < NBUCKET; ++j) {
    int i = (id + j) % NBUCKET;
    acquire(&bcache.bucket[i].lock);
    for (b = &bcache.bucket[i].head; b->next != &bcache.bucket[i].head; b = b->next) {
      if (b->next->refcnt == 0) {
        struct buf *tmp = b->next;
        b->next = tmp->next;
        release(&bcache.bucket[i].lock);

        acquire(&bcache.bucket[id].lock);
        tmp->next = bcache.bucket[id].head.next;
        bcache.bucket[id].head.next = tmp;
        tmp->dev = dev;
        tmp->blockno = blockno;
        tmp->valid = 0;
        tmp->refcnt = 1;
        release(&bcache.bucket[id].lock);
        acquiresleep(&tmp->lock);
        release(&bcache.lock);
        return tmp;
      }
    }
    release(&bcache.bucket[i].lock);
  }
  release(&bcache.lock);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  int id = b->blockno % NBUCKET;
  releasesleep(&b->lock);


  acquire(&bcache.bucket[id].lock);
  b->refcnt--;
  release(&bcache.bucket[id].lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


