/*
    1. Notes on virtio and how it works:

    This kernel implementation chooses virtio as the interface
    to interact with virtual devices.

    Virtio is a protocol/ specification that defines the way
    to interact with virtualized devices. It can be seen a 
    virtual counterpart of the ATA or IDE standard when it
    comes to storage devices, such as disks. Specifically, it
    defines a set of memory mapped registers that the
    driver can write to or read from to issue have the device
    perform certain tasks.

    The way to interact with virtual device through the legacy
    virtio MMIO interface that qemu implements is as follows:

    driver_queue [1,2,3,4,5,6...]
    device_queue [1,2,3,4,...]

    The driver writes requests to the avail_queue while the 
    devices reads from it and when processes the requests.
    When the devices is done and writes to the used_queue
    to try to keep up with the driver's writing to the avail_queue.
    The device stops reading new requests when it sees the current
    index of the used_queue is equal to that of the drievr_queue,
    signifying no newer incoming requests from the driver side.l
    Optionally, the device, when it finishes processing a request,
    say a disk read, can issue an interrupt through plic to inform
    the cpu the completion of the request.

    In addition, these two queues are circular and whereby are 
    sometimes refered to as rings.

    Each element in the driver queue are simply integers. These
    integers are indices that index a certain slot in a struture
    called a virtq descriptor table:

    [4,1,6,3,1,2...] driver queue
       |
       |            descriptor table:
       |            +------------------------+
       |            |                        | descriptor 0
       |            +------------------------+
       +----------->|                        | descriptor 1
                    +------------------------+
                    |                        | descriptor 2
                    +------------------------+

    where each descriptor describes a unit of work to be done. What
    the device would do after reading off an index from the driver
    queue is to use that index to find the corresponding descriptor
    in the descriptor table and see (say it is a disk read) which
    block the driver is requesting and to which memory address should
    the disk controller write the content of that block to.

    When the devie's done. It writes a struct to the device queue and
    updates the asscociated index for the device queue. That struct
    in turn contains index of the descriptor associated to the
    corresponding request and the content length.

    In particular, for the legacy MMIO interface that qemu implements,
    one disk access requires three chained descriptors; thus, each
    unit of work is described by not one but three descriptors. To chain
    them, in the interface-defined descriptor struct, there is a NEXT
    field for holding the next descriptor's index in the descriptor
    table, and another filed, FLAGS, setting whose DESC_FLG_MSK_NEXT bit
    indicates that there is a subsequent descriptor that follows or
    the end of chain otherwise.

    2.
    This header file defines macros and structs useful for interacting
    with the virtual devices and is included by disk.c only, whereby it is 
    not placed in the common include directory

    3.
    The macros and structs are defined based on the virtio specification can be found here 
    https://docs.oasis-open.org/virtio/virtio/v1.1/cs01/virtio-v1.1-cs01.html#x1-1950001

    4.
    Although QEMU doc spcifically recommends virtio at
    https://www.qemu.org/2021/01/19/virtio-blk-scsi-configuration/
    it also supports ATA or IDE for which drivers can be written accordingly

*/


#ifndef _virtio_h_
#define _virtio_h_

#include "../include/types.h"

// virtio base address
// from https://github.com/qemu/qemu/blob/master/hw/riscv/virt.c
#define VIRTIO_BASE 0x10001000

// Use the below macro to access registers
#define REG(offset) ((volatile uint32_t*)(VIRTIO_BASE + offset))

// MMIO Device Register Layout
// for more details, see section 4.2.2 in sepc
#define MMIO_MAGIC_VALUE          0x000
#define MMIO_VERSION              0x004
#define MMIO_DEVICE_ID            0x008
#define MMIO_VENDOR_ID            0x00c
#define MMIO_DEVICE_FEATURES      0x010
#define MMIO_DRIVER_FEATURES      0x020
#define MMIO_QUEUE_SEL            0x030 
#define MMIO_QUEUE_SIZE_MAX       0x034 // renamed from QueueNumMax
#define MMIO_QUEUE_SIZE           0x038 // renamed from QueueNum
#define MMIO_QUEUE_READY          0x044
#define MMIO_QUEUE_NOTIFY         0x050
#define MMIO_INTR_STATUS          0x060
#define MMIO_INTR_ACK             0x064
#define MMIO_STATUS               0x070
#define MMIO_DESC_TABLE_LOW       0x080 // renamed from QueueDescLow
#define MMIO_DESC_TABLE_HIGH      0x084
#define MMIO_DRIVER_QUEUE_LOW     0x090 // renamed from QueueDriverLow
#define MMIO_DRIVER_QUEUE_HIGH    0x094
#define MMIO_DEVICE_QUEUE_LOW     0x0a0 // renamed from QueueDeviceLow
#define MMIO_DEVICE_QUEUE_HIGH    0x0a4

// Device Status Field
// For more details, see section 2.1 in sepc
// During device initialization by a driver, the driver follows the sequence of steps specified in 3.1.
#define DEVICE_STATUS_MSK_ACKNOWLEDGE 1 // Indicates that the guest OS has found the device and recognized it as a valid virtio device.
#define DEVICE_STATUS_MSK_DRIVER      2 // Indicates that the guest OS knows how to drive the device.
#define DEVICE_STATUS_MSK_DRIVER_OK   4 // Indicates that the driver is set up and ready to drive the device.
#define DEVICE_STATUS_MSK_FEATURES_OK 8 // Indicates that the driver has acknowledged all the features it understands, and feature negotiation is complete.

// Block Device Feature Bits
// Refer to section 4.2 for more details about block device fearure bits
// and section 2.2 about feature bits in general
#define BLK_FEATURE_BIT_RO              5
#define BLK_FEATURE_BIT_SCSI            7
#define BLK_FEATURE_BIT_CONFIG_WCE      11
#define BLK_FEATURE_BIT_MQ              12
#define QUEUE_FEATURE_BIT_ANY_LAYOUT    27
#define QUEUE_FEATURE_BIT_INDIRECT_DESC 28
#define QUEUE_FEATURE_BIT_EVENT_IDX     29

#define NUMDESC 8 // number of descriptors in the table

#define DESC_FLG_MSK_NEXT     0x1
#define DESC_FLG_MSK_WRITE    0x2
#define DESC_FLG_MSK_INDIRECT 0x4

// The Virtqueue Descriptor Table Entry Struct, section 2.4.5
typedef struct desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flgs;
    uint16_t next;
} __attribute__((packed, aligned(16))) desc_t;

// The Virtqueue Available Ring, section 2.4.6
typedef struct driverq {
  uint16_t flgs;
  uint16_t idx; // driver-updated
  uint16_t ring[NUMDESC];
  uint16_t dontcare;
} __attribute__((packed, aligned(16))) driverq_t;

// The Virtqueue Used Ring, section 2.4.8
typedef struct deviceq {
  uint16_t flgs;
  uint16_t idx; // device-updated (this is try to keep up with the driver queue)
  struct { uint32_t id, len; } ring [NUMDESC];
  uint16_t dontcare;
} __attribute__((packed, aligned(16))) deviceq_t;

#define BLK_OP_R 0 // read the disk
#define BLK_OP_W 1 // write the disk
// Device Operation, section 5.2.6
typedef struct req {
  uint32_t op; // read or write
  uint32_t reserved;
  uint64_t sector;
} req_t;

#endif

