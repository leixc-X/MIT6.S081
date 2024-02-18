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
#include <stdint.h>

#define NBUCKET 13
uint extern ticks;

int hash(int n ) {
  int ret = n % NBUCKET;
  return ret;
}
struct {
  struct spinlock lock[NBUCKET];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;
  for(int i = 0; i < NBUCKET; i++) {
    initlock(&bcache.lock[i], "bcache");
  }
  
  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  bcache.head[0].next = &bcache.buf[0];
  for(b = bcache.buf; b < bcache.buf+NBUF - 1; b++){
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    b->next = b + 1;
    initsleeplock(&b->lock, "buffer");
    // bcache.head.next->prev = b;
    // bcache.head.next = b;
  }
}

int can_lock(int id, int i) {
  int num = NBUCKET / 2;
  if(id <= num) {
    if(i > id && i <= (num + id)) {
      return 0;
    }
  } else {
    if((id< i && i < NBUCKET) || (i <= (id + num)%NBUCKET)) {
      return 0;
    }
  }
  return 1;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b; 
  struct buf *selected;
  int id = hash(blockno);

  acquire(&bcache.lock[id]);
  b = bcache.head[id].next;

  // Is the block already cached?
  while (b) {
    if(b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      if(holding(&bcache.lock[id]))
        release(&bcache.lock[id]);
      acquiresleep(&b->lock);
      return b;
    }
    b = b->next;
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  int index = -1;
  int small_tick = UINT32_MAX;

  for(int i = 0; i < NBUCKET; i++) {
    if(i != id && can_lock(id, i)) {
      acquire(&bcache.lock[i]);
    } else if(!can_lock(id, i)) {
      continue;
    }
    b = bcache.head[i].next;
    while (b) {
      if(b->refcnt == 0) {
        if(b->time < small_tick) {
          small_tick = b->time;
          if(index != -1 && index != i && holding(&bcache.lock[index])) release(&bcache.lock[index]);
          index = i;
        }
      }
      b = b->next;
    }
    if(i != id && i != index && holding(&bcache.lock[i])) release(&bcache.lock[i]);
  }

  if(index == -1)
    panic("bget: no buffers");

  b = &bcache.head[index];
  while (b) {
    if(b->next->refcnt == 0 && b->next->time == small_tick) {
      selected = b->next;
      b->next = b->next->next;
      break;
    }
    b = b->next;
  }
  if (index != id && holding(&bcache.lock[index])) release(&bcache.lock[index]);
  b = &bcache.head[id];
  while (b->next) {
    b = b->next;
  }
  b->next = selected;
  selected->next = 0;
  selected->dev = dev;
  selected->blockno = blockno;
  selected->valid = 0;
  selected->refcnt = 1;
  if (holding(&bcache.lock[id]))
      release(&bcache.lock[id]);
  acquiresleep(&selected->lock);

  return selected;
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

  releasesleep(&b->lock);

  int id = hash(b->blockno);

  acquire(&bcache.lock[id]);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->time = ticks;
  }
  
  release(&bcache.lock[id]);
}

void
bpin(struct buf *b) {
  int id = hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt++;
  release(&bcache.lock[id]);
}

void
bunpin(struct buf *b) {
  int id = hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt--;
  release(&bcache.lock[id]);
}


