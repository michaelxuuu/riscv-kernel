#include "virtio.h"
#include "../include/disk.h"
#include "../include/types.h"
#include "../include/spinlk.h"
#include "../include/mmio.h"
#include "../include/kpanic.h"
#include "../include/pm.h"
#include "../include/util.h"
#include "../include/bio.h"
#include "../include/sync.h"

// Descriptor table
static struct {
    desc_t arr[NUMDESC];
    bool status[NUMDESC];
} desctbl;

driverq_t driverq;
deviceq_t deviceq;
// The device may process our requests
// faster than the kernel can process 
// its response due to the interruptiblity
// of the kernel routines. So, we need
// an extra idx to keep track of our 
// own progress in processing the device 
// response
uint16_t idx; // we have handled this much of reponses in the queue

// each requests will be assigned to an a `req_t` struct
// which is used in forming first descriptor as the "header",
// indicating which block to operate on and the operation type
req_t reqs[NUMDESC];

// Mutual exclusion locks between cores
static spinlk_t lk = SPINLK_INITIALIZER;

// Keep track of in-flight transactions
static struct {
  buf_t *bufs[NUMDESC];
  char status[NUMDESC];
} txns;

static void init();
static void rw(buf_t* b, bool w);
static void isr();

disk_t disk = {init, rw, isr};

// initilization process is described in spec's section 3.1
// for Virtqueue initialization, refer to spec's section 4.2.3.2
void init() {
    // does the system supports virtio disk?
    if (*REG(MMIO_MAGIC_VALUE) != 0x74726976 || 
        *REG(MMIO_VERSION) != 2 ||
        *REG(MMIO_DEVICE_ID) != 2 || 
        *REG(MMIO_VENDOR_ID) != 0x554d4551) 
            kpanic("virtio device not found: disk\n");


    // reset device
    *REG(MMIO_STATUS) = 0;

    // Indicates that the guest OS has found the device and recognized it as a valid virtio device.
    uint32_t status = DEVICE_STATUS_MSK_ACKNOWLEDGE;
    *REG(MMIO_STATUS) = status;

    // Indicates that the guest OS knows how to drive the device.
    status |= DEVICE_STATUS_MSK_DRIVER;
    *REG(MMIO_STATUS) = status;

    // negotiate features with the device (picking the interection)
    uint32_t features = *REG(MMIO_DEVICE_FEATURES);
    features &= ~(1 << BLK_FEATURE_BIT_RO);
    features &= ~(1 << BLK_FEATURE_BIT_SCSI);
    features &= ~(1 << BLK_FEATURE_BIT_CONFIG_WCE);
    features &= ~(1 << BLK_FEATURE_BIT_MQ);
    features &= ~(1 << QUEUE_FEATURE_BIT_ANY_LAYOUT);
    features &= ~(1 << QUEUE_FEATURE_BIT_EVENT_IDX);
    features &= ~(1 << QUEUE_FEATURE_BIT_INDIRECT_DESC);
    *REG(MMIO_DRIVER_FEATURES) = features;

    // single the device that the negotiation's complete
    status |= DEVICE_STATUS_MSK_FEATURES_OK;
    *REG(MMIO_STATUS) = status;

    // make sure DEVICE_STATUS_MSK_FEATURES_OK is set
    if (!(*REG(MMIO_STATUS) & DEVICE_STATUS_MSK_FEATURES_OK))
        kpanic("failed to set FEATURES_OK\n");
    
    // select queue 0 and initialize it
    *REG(MMIO_QUEUE_SEL) = 0;
    if(*REG(MMIO_QUEUE_READY))
        kpanic("virtio queue not ready\n");
    
    // check maximum queue size
    uint32_t max = *REG(MMIO_QUEUE_SIZE_MAX);
    if(max == 0)
        kpanic("virtio has no queue 0");
    if(max < NUMDESC)
        kpanic("virtio max queue too short");

    // inform virtio of the starting address of the memory blocks we allocated for queueing
    *REG(MMIO_DESC_TABLE_LOW) = (uint64_t)desctbl.arr;
    *REG(MMIO_DESC_TABLE_HIGH) = (uint64_t)desctbl.arr >> 32;
    *REG(MMIO_DEVICE_QUEUE_LOW) = (uint64_t)&deviceq;
    *REG(MMIO_DEVICE_QUEUE_HIGH) = (uint64_t)&deviceq >> 32;
    *REG(MMIO_DRIVER_QUEUE_LOW) = (uint64_t)&driverq;
    *REG(MMIO_DRIVER_QUEUE_HIGH) = (uint64_t)&driverq >> 32;

    // queue ready
    *REG(MMIO_QUEUE_READY) = 1;

    // all descriptors start unsued
    for (int i = 0; i < NUMDESC; i++)
        desctbl.status[i] = 1;

    // device ready
    status |= DEVICE_STATUS_MSK_DRIVER_OK;
    *REG(MMIO_STATUS) = status;
}

static int alloc_desc() {
    for (int i = 0; i < NUMDESC; i++)
        if (desctbl.status[i]) {
            desctbl.status[i] = 0;
            return i;
        }
    return -1;
}

static void free_desc(int idx) {
    desctbl.status[idx] = 1;
    desctbl.arr[idx].addr = 0;
    desctbl.arr[idx].len = 0;
    desctbl.arr[idx].flgs = 0;
    desctbl.arr[idx].next = 0;
    // wake up the processes waiting on the completion of the work described by descriptor
    // to be completed...
}

static int alloc3_desc(int indices[3])
{
    for(int i = 0; i < 3; i++){
        indices[i] = alloc_desc();
        if(indices[i] < 0){
        // free all previously allocated descriptors on a failure 
        for(int j = 0; j < i; j++)
            free_desc(indices[j]);
        return -1;
      }
    }
    return 0;
}


static void free3_desc(int idx) {
    for (int i = 0; i < 3; i++) {
        int old_idx = idx;
        int idx = desctbl.arr[idx].next;
        free_desc(old_idx);
    }
}

// disk read and write
static void rw(buf_t* b, bool w) {

    uint64_t sect = b->blockno * (BSIZE / 512);


    int indices[3];
    while (1) {
        if (!alloc3_desc(indices))
            break;
        // sleep
    }

    uint16_t idx1 = indices[0];
    uint16_t idx2 = indices[1];
    uint16_t idx3 = indices[2];

    reqs[idx1].op = w ? BLK_OP_W : BLK_OP_R;
    reqs[idx1].sector = sect;
    reqs[idx1].reserved = 0;

    desc_t* desc1 = &desctbl.arr[idx1];
    desc_t* desc2 = &desctbl.arr[idx2];
    desc_t* desc3 = &desctbl.arr[idx3];

    desc1->addr = (uint64_t)&reqs[idx1];
    desc1->len = sizeof(req_t);
    desc1->flgs = DESC_FLG_MSK_NEXT;
    desc1->next = idx2;

    desc2->addr = (uint64_t)b->data;
    desc2->len = BSIZE;
    desc2->flgs = DESC_FLG_MSK_NEXT | (w ? 0 : DESC_FLG_MSK_WRITE);
    desc2->next = idx3;

    desc3->addr = (uint64_t)&txns.status[idx1];
    desc3->len = 1;
    desc3->flgs = DESC_FLG_MSK_WRITE;
    desc3->next = 0; // last in chain

    // record the transation
    txns.status[idx1] = 1;
    b->disk = 1;
    txns.bufs[idx1] = b;

    // write driver queue
    driverq.ring[driverq.idx % NUMDESC] = idx1;

    // update driver queue index
    sync();
    driverq.idx ++;
    sync();

    *REG(MMIO_QUEUE_NOTIFY) = 0;

    // wait for the disk finish the work
    while(b->disk == 1) {
        // sleep
    }

    txns.bufs[idx1] = 0;
    free3_desc(idx1);
}

static void isr() {
    spinlk_acquire(&lk);

    *REG(MMIO_INTR_ACK) = *REG(MMIO_INTR_STATUS) & 0x3;

    sync();
    while (idx != deviceq.idx) {
        sync();
        int id = deviceq.ring[deviceq.idx % NUMDESC].id;
        if (txns.status[id])
            kpanic("incorrect status\n");
        txns.bufs[id]->disk = 0;
        // wakeup(inflight[id].b)
        idx ++;
    }

    spinlk_release(&lk);
}

