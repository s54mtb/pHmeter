/**
 * @file     ad7715.h
 * @brief    AD7715 driver Header File
 * @version  V0.00
 * @date     18. October 2016
 * @copyrigt s54mtb
 * @note
 *
 */

#ifndef ___AD7715_H_
#define ___AD7715_H_


#define AD7715_ERROR	0x01
#define AD7715_OK			0x00


/** \brief AD7715 register selection 
 RS1  RS0			Register 
	0		0				Communications register
	0		1				Setup register
	1		0				Test register
	1		1				Data register
*/
#define AD7715_REG_COMM			0
#define AD7715_REG_SETUP		1
#define AD7715_REG_TEST			2
#define AD7715_REG_DATA			3


/** \brief AD7715 Gain selection 
 G1  G0			PGA Gain 
	0		0			1
	0		1			2
	1		0			32
	1		1			128
*/
#define AD7715_GAIN_1			0
#define AD7715_GAIN_2			1
#define AD7715_GAIN_32		2
#define AD7715_GAIN_128		3

/** \brief AD7715 Power up/down STBY bit */
#define AD7715_STBY_POWERUP		0
#define AD7715_STBY_POWERDOWN	1

/** \brief AD7715 Register read/write bit */
#define AD7715_RW_READ		1
#define AD7715_RW_WRITE		0

	
/** \brief  Union type for the structure of COMMUNICATIONS REGISTER 
 */
typedef union
{
	struct
	{
		uint8_t Gain		:2;			/*!< bits 1:0   : Gain Select */
		uint8_t STBY		:1;			/*!< bit  2     : Standby */
		uint8_t RW			:1;			/*!< bit  3     : Read/Write Select */
		uint8_t RS			:2;			/*!< bits 5:4   : Register Selection */ 
		uint8_t Zero 		:1;  		/*!< bit  6 	  : must be zero! */
		uint8_t DRDY		:1;  		/*!< bit  7 	  : DRDY bit */
	} b;
	uint8_t B;
} AD7715_CommReg_t;



/** \brief AD7715 Operating Modes 
 MD1  MD0		Operating Mode
	0		0			Normal mode
	0		1			Self-calibration
	1		0			Zero-scale system calibration
	1		1			Full-scale system calibration
*/
#define AD7715_MODE_NORMAL			0
#define AD7715_MODE_SELFCAL			1
#define AD7715_MODE_ZEROCAL			2
#define AD7715_MODE_FSCAL				3


/** \brief AD7715 Operating frequency select */
#define AD7715_CLK_1MHZ					0
#define AD7715_CLK_2_4576MHZ		1


/** \brief AD7715 Update rate 
    Note: the rate depends on CLK bit ! */
		/** 1MHz clock */
#define AD7715_FS_20HZ					0
#define AD7715_FS_25HZ					1
#define AD7715_FS_100HZ					2
#define AD7715_FS_200HZ					3
		/** 2.4576MHz clock */
#define AD7715_FS_50HZ					0
#define AD7715_FS_60HZ					1
#define AD7715_FS_250HZ					2
#define AD7715_FS_500HZ					3


/** \brief AD7715 Polarity select */
#define AD7715_BU_UNIPOLAR				1
#define AD7715_BU_BIPOLAR					0

/** \brief AD7715 Buffer bypass */
#define AD7715_BUF_ACTIVE				1
#define AD7715_BUF_BYPASSED			0


/** \brief  Union type for the structure of SETUP REGISTER 
 */
typedef union
{
	struct
	{
		uint8_t FSYNC		:1;  		/*!< bit  0 	  : filter synchronization */
		uint8_t BUF    	:1;  		/*!< bit  1 	  : buffer control */
		uint8_t BU			:1;			/*!< bit  2     : bipolar/unipolar  */
		uint8_t FS			:2;			/*!< bits 4:3   : output update rate */ 
		uint8_t CLK		  :1;			/*!< bit  5     : master clock selection */
		uint8_t MD 		  :2;			/*!< bits 7:6   : Mode select */
	} b;
	uint8_t B;
} AD7715_SetupReg_t;


#endif 


