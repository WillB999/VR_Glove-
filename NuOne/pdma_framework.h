
#define E_DRVPDMA_ERR_PORT_INVALID 1 //	_SYSINFRA_ERRCODE(TRUE, MODULE_ID_DRVPDMA, 2)

typedef enum
{
	eDRVPDMA_DISABLE = 0,
	eDRVPDMA_ENABLE	 = 1
}E_DRVPDMA_OPERATION;

typedef enum
{
	eDRVPDMA_CHANNEL_0	=	0,
	eDRVPDMA_CHANNEL_1 	=	1,
	eDRVPDMA_CHANNEL_2	=	2,
	eDRVPDMA_CHANNEL_3	=	3
}E_DRVPDMA_CHANNEL_INDEX;

typedef enum
{
	eDRVPDMA_TARGET_SOURCE      = 0,
	eDRVPDMA_TARGET_DESTINATION = 1
}E_DRVPDMA_TARGET;



typedef enum
{
	eDRVPDMA_TABORT_FLAG            = 1,
	eDRVPDMA_BLKD_FLAG              = 2,
	eDRVPDMA_WRA_EMPTY_FLAG         = 0x100,	          	    
	eDRVPDMA_WRA_THREE_FOURTHS_FLAG = 0x200,	  
	eDRVPDMA_WRA_HALF_FLAG          = 0x400,             
	eDRVPDMA_WRA_QUARTER_FLAG       = 0x800           
}E_DRVPDMA_INT_FLAG;

typedef enum
{
	eDRVPDMA_WRA_WRAP_INT = 0x01,	          	    
	eDRVPDMA_WRA_HALF_INT  = 0x04,             
	eDRVPDMA_WRA_WRAP_HALF_INT  = 0x05,             
	eDRVPDMA_WRA_NO_INT    = 0x00           
}E_DRVPDMA_WRA_INT_EN;

typedef enum
{
	eDRVPDMA_DIRECTION_INCREMENTED 	=	0,
	eDRVPDMA_DIRECTION_DECREMENTED	=   1,
	eDRVPDMA_DIRECTION_FIXED      	=	2,
	eDRVPDMA_DIRECTION_WRAPAROUND	=	3,
}E_DRVPDMA_DIRECTION_SELECT;

typedef enum
{
	eDRVPDMA_WIDTH_32BITS = 0,    
	eDRVPDMA_WIDTH_8BITS  = 1,
	eDRVPDMA_WIDTH_16BITS = 2
}E_DRVPDMA_TRANSFER_WIDTH;

typedef enum
{
	eDRVPDMA_TABORT = 1,
	eDRVPDMA_BLKD   = 2,
	eDRVPDMA_WAR    = 4			
}E_DRVPDMA_INT_ENABLE;


typedef enum
{
	eDRVPDMA_SPI0	=	0,    
	eDRVPDMA_DPWM	=	1,  
	eDRVPDMA_UART0	=	2,	  
	eDRVPDMA_I2S	=	3,
	eDRVPDMA_ADC	=	4						  
}E_DRVPDMA_APB_DEVICE;

typedef enum
{
	eDRVPDMA_READ_APB	= 0,    
	eDRVPDMA_WRITE_APB  = 1
}E_DRVPDMA_APB_RW;


typedef enum
{
	eDRVPDMA_MODE_MEM2MEM	=	0,    
	eDRVPDMA_MODE_APB2MEM	=	1,    
	eDRVPDMA_MODE_MEM2APB	=	2
}E_DRVPDMA_MODE;									


typedef struct {
    uint32_t u32Addr;
    E_DRVPDMA_DIRECTION_SELECT eAddrDirection;
}S_DRVPDMA_CH_ADDR_SETTING;


typedef struct DRVPDMA_STRUCT
{
    S_DRVPDMA_CH_ADDR_SETTING sSrcAddr;    		/* Source Address */
	S_DRVPDMA_CH_ADDR_SETTING sDestAddr;		/* Destination Address */
    E_DRVPDMA_TRANSFER_WIDTH  u8TransWidth;		/* Transfer Width */
    E_DRVPDMA_MODE 			  u8Mode;			/* Operation Mode */
	E_DRVPDMA_WRA_INT_EN	  u8WrapBcr;		/* Wrap Mode */
	int32_t              	  i32ByteCnt;		/* Byte Count */
}STR_PDMA_T;

//void FrmWrk_PdmaCtrl_Init(void);
int32_t FrmWrk_PDMA_Open(E_DRVPDMA_CHANNEL_INDEX eChannel, STR_PDMA_T *sParam);
//void PdmaCtrl_Stop(E_DRVPDMA_CHANNEL_INDEX eChannel);//uint32_t u32Ch);
//void PdmaCtrl_Close(E_DRVPDMA_CHANNEL_INDEX eChannel);//uint32_t u32Ch);
