#include <linux/kernel.h>
#include <mach/nvt_type.h>

#define BLK_SZ512 512
#define BLK_SZ1024 1024

/*
   USB FIFO number 0~7
   */
typedef enum {
	USB_FIFO0,
	USB_FIFO1,
	USB_FIFO2,
	USB_FIFO3,
	USB_FIFO4,
	USB_FIFO5,
	USB_FIFO6,
	USB_FIFO7,

	USB_FIFO_MAX,
	USB_FIFO_NOT_USE = -1,
	ENUM_DUMMY4WORD(USB_FIFO_NUM)
} USB_FIFO_NUM;

typedef enum {
	USB_EP0,                                    ///< USB End Point number 0
	USB_EP1,                                    ///< USB End Point number 1
	USB_EP2,                                    ///< USB End Point number 2
	USB_EP3,                                    ///< USB End Point number 3

	USB_EP_MAX,
	USB_EP_NOT_USE = -1,
	ENUM_DUMMY4WORD(USB_EP)
} USB_EP;

typedef enum {
	EP_TYPE_NOT_USE,                            ///< Endpoint transfer type not set
	EP_TYPE_ISOCHRONOUS,                        ///< Endpoint transfer type is isochronous
	EP_TYPE_BULK,                               ///< Endpoint transfer type is bulk
	EP_TYPE_INTERRUPT,                          ///< Endpoint transfer type is interrupt
	ENUM_DUMMY4WORD(USB_EP_TYPE)
} USB_EP_TYPE;

typedef enum {
	BLKNUM_NOT_USE,                             ///< block not used
	BLKNUM_SINGLE,                              ///< use single block
	BLKNUM_DOUBLE,                              ///< use double block
	BLKNUM_TRIPLE,                              ///< use triple block
	ENUM_DUMMY4WORD(USB_EP_BLKNUM)
} USB_EP_BLKNUM;

typedef enum {
	USB_FIFO_OUT,                       // FIFO direction OUT
	USB_FIFO_IN,                        // FIFO direction IN
	USB_FIFO_BI,                        // FIFO bi-direction
	USB_FIFO_NOT,                       // FIFO not used
	ENUM_DUMMY4WORD(USB_FIFO_DIR)
} USB_FIFO_DIR;


/* saving EP & FIFO mapping information */

// record EP maps to which FIFO for below cases
UINT32  gEPMap[FOTG210_MAX_NUM_EP - 1] = {
	USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE
};

// record IN FIFO maps to which EP
UINT32  gFIFOInMap[FOTG210_MAX_FIFO_NUM] = {
	USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE
};

// record OUT FIFO maps to which EP
UINT32  gFIFOOutMap[FOTG210_MAX_FIFO_NUM] = {
	USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE
};

UINT32  gEPBlkNo[FOTG210_MAX_NUM_EP - 1] = {
	BLKNUM_DOUBLE, BLKNUM_DOUBLE, BLKNUM_DOUBLE, BLKNUM_DOUBLE
};

UINT32  gEPBlkSz[FOTG210_MAX_NUM_EP - 1] = {
	BLK_SZ512, BLK_SZ512, BLK_SZ512, BLK_SZ512
};

