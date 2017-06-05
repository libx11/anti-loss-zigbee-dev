/**************************************************************************************************
  Filename:       SampleApp.c
  Revised:        $Date: 2009-03-18 15:56:27 -0700 (Wed, 18 Mar 2009) $
  Revision:       $Revision: 19453 $

  Description:    Sample Application (no Profile).


  Copyright 2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED �AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/*********************************************************************
  This application isn't intended to do anything useful, it is
  intended to be a simple example of an application's structure.

  This application sends it's messages either as broadcast or
  broadcast filtered group messages.  The other (more normal)
  message addressing is unicast.  Most of the other sample
  applications are written to support the unicast message model.

  Key control:
    SW1:  Sends a flash command to all devices in Group 1.
    SW2:  Adds/Removes (toggles) this device in and out
          of Group 1.  This will enable and disable the
          reception of the flash command.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "SampleApp.h"
#include "SampleAppHw.h"

#include "OnBoard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "MT_UART.h"
#include "MT_APP.h"
#include "MT.h"
#include "stdlib.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
uint8 AppTitle[] = "ALD2530 LED"; //Ӧ�ó�������

// This list should be filled with Application specific Cluster IDs.
const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
  SAMPLEAPP_PERIODIC_CLUSTERID,
  SAMPLEAPP_FLASH_CLUSTERID,
  SAMPLEAPP_SINGLE_CLUSTERID
};

const SimpleDescriptionFormat_t SampleApp_SimpleDesc =
{
  SAMPLEAPP_ENDPOINT,              //  int Endpoint;
  SAMPLEAPP_PROFID,                //  uint16 AppProfId[2];
  SAMPLEAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  SAMPLEAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  SAMPLEAPP_FLAGS,                 //  int   AppFlags:4;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList,  //  uint8 *pAppInClusterList;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList   //  uint8 *pAppInClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in SampleApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t SampleApp_epDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
uint8 SampleApp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // SampleApp_Init() is called.
devStates_t SampleApp_NwkState;

uint8 SampleApp_TransID;  // This is the unique message ID (counter)

afAddrType_t SampleApp_Periodic_DstAddr;
afAddrType_t SampleApp_Single_DstAddr;
afAddrType_t SampleApp_Flash_DstAddr;

aps_Group_t SampleApp_Group;

uint8 SampleAppPeriodicCounter = 0;
uint8 SampleAppFlashCounter = 0;

uint16 device[32] = {0};
int sel_dev = 0;
int main_flag = 1;
int beep_flag = 0;
int time_flag = 0;
int distance_flag = 0;
int count = 0;
int period_count = 0;
int device_num = 0;
uint16 dst_dev = 0x0000;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void SampleApp_HandleKeys( uint8 shift, uint8 keys );
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void SampleApp_SendPeriodicMessage( void );
void SampleApp_SendFlashMessage( uint16 flashTime ,uint16 data);
void SApp_ProcessMsgCBs( zdoIncomingMsg_t *msgPtr );
void SampleApp_SendAddrMessage(uint8 dev[], int dev_num);
void SampleApp_SendBeepMessage(uint16 dst, uint8 s);



void InitT1(void);
void InitT3(void);
__interrupt void T3_ISR(void);
void beep0(void);
void beep1(void);
void beep2(void);


/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SampleApp_Init
 *
 * @brief   Initialization function for the Generic App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
#define BEEP P0_7  //��������



/****************************************************************************
* ��    ��: InitLed()
* ��    ��: ����LED����Ӧ��IO��
* ��ڲ���: ��
* ���ڲ���: ��
****************************************************************************/

/****************************************************************************
* ��    ��: InitT1()
* ��    ��: ��ʱ����ʼ����TICKSPD ��16 MHzϵͳ������ʱĬ����2��Ƶ����16MHz
* ��ڲ���: ��
* ���ڲ���: ��
****************************************************************************/


void InitT1(void)
{
    CLKCONCMD &= ~0x40;      //����ϵͳʱ��ԴΪ32MHZ����
    while(CLKCONSTA & 0x40); //�ȴ������ȶ�Ϊ32M
    CLKCONCMD &= ~0x07;      //����ϵͳ��ʱ��Ƶ��Ϊ32MHZ   
    CLKCONCMD |= 0x38;       //ʱ���ٶ�32 MHz ��ʱ������������[5:3]250kHz

    PERCFG |= 0x40;          //��ʱ��1 ��IOλ��   1:����λ��2 
    P2SEL &= ~0x10;          //��ʱ��1����
    P2DIR |= 0xC0;           //��1���ȼ�����ʱ��1ͨ��2-3

    P0DIR |= 0x80;           //�˿�1Ϊ���    
    P0SEL |= 0x80;           //timer1 ͨ��2ӳ���P1_0
    
    T1CC3H = 0x00;           //20%ռ�ձ�Ϊ200us
    T1CC3L = 0x00;           //�޸�T1CC2L�ɵ���led������
    T1CC0H = 0x00;           //1ms������ʱ��,Ƶ��Ϊ976.516HZ
    T1CC0L = 0x00; 
    T1CCTL3 = 0x34;          // ģʽѡ�� ͨ��2�Ƚ�ģʽ
    T1CTL |= 0x03;            //250KHz 1��Ƶ
  
}

void InitT3(void)
{     
    T3CTL |= 0x08 ;          //������ж�     
    T3IE = 1;                //�����жϺ�T3�ж�
    T3CTL |= 0xE0;           //128��Ƶ,128/16000000*N=0.5S,N=62500
    T3CTL &= ~0x03;          //�Զ���װ 00��>0xff  62500/255=245(��)
    T3CTL |= 0x10;           //����
    EA = 1;                  //�����ж�
}
#pragma vector = T3_VECTOR 
__interrupt void T3_ISR(void) 
{ 
  
  
    IRCON = 0x00;            //���жϱ�־, Ҳ����Ӳ���Զ���� 
    
    
    if(period_count++ > 150)
    {
      	period_count = 0;
    }
      
      
    if(count++ > 2)          //245���жϺ�LEDȡ������˸һ�֣�ԼΪ0.5 ��ʱ�䣩 
    {                        //����ʾ��������ȷ����ȷ
        count = 0;          //��������          //�ı�LED1��״̬
	
	if(beep_flag == 1)
		if(time_flag == 0)
		{
			time_flag = 1;
			beep1();
		}
		else
		{
			time_flag = 0;
			beep0();
		}
    } 
}



void beep0(void)
{
    T1CC0H = 0x00;           //0ms������ʱ��,Ƶ��Ϊ976.516HZ
    T1CC0L = 0x00; 
}
void beep1(void)
{
    T1CC0H = 0x00;           //1ms������ʱ��,Ƶ��Ϊ976.516HZ
    T1CC0L = 0xff; 
    
}
void beep2(void)
{
    T1CC0H = 0x01;           //2ms������ʱ��,Ƶ��Ϊ976.516HZ
    T1CC0L = 0xff;
}



void SampleApp_Init( uint8 task_id )
{ 
  SampleApp_TaskID = task_id;   //osal���������ID�����û���������������ı�
  SampleApp_NwkState = DEV_INIT;//�豸״̬�趨ΪZDO���ж���ĳ�ʼ��״̬
  SampleApp_TransID = 0;        //��Ϣ����ID������Ϣʱ��˳��֮�֣�
  
  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().

 #if defined ( BUILD_ALL_DEVICES )
  // The "Demo" target is setup to have BUILD_ALL_DEVICES and HOLD_AUTO_START
  // We are looking at a jumper (defined in SampleAppHw.c) to be jumpered
  // together - if they are - we will start up a coordinator. Otherwise,
  // the device will start as a router.
  if ( readCoordinatorJumper() )
    zgDeviceLogicalType = ZG_DEVICETYPE_COORDINATOR;
  else
    zgDeviceLogicalType = ZG_DEVICETYPE_ROUTER;
#endif // BUILD_ALL_DEVICES

//�öε���˼�ǣ����������HOLD_AUTO_START�궨�壬����������оƬ��ʱ�����ͣ����
//���̣�ֻ���ⲿ�����Ժ�Ż�����оƬ����ʵ������Ҫһ����ť���������������̡�  
#if defined ( HOLD_AUTO_START )
  // HOLD_AUTO_START is a compile option that will surpress ZDApp
  //  from starting the device and wait for the application to
  //  start the device.
  ZDOInitDevice(0);
#endif

  // Setup for the periodic message's destination address ���÷������ݵķ�ʽ��Ŀ�ĵ�ַѰַģʽ
  // Broadcast to everyone ����ģʽ:�㲥����
  SampleApp_Periodic_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;//�㲥
  SampleApp_Periodic_DstAddr.endPoint = SAMPLEAPP_ENDPOINT; //ָ���˵��
  SampleApp_Periodic_DstAddr.addr.shortAddr = 0xFFFF;//ָ��Ŀ�������ַΪ�㲥��ַ

  // Setup for the flash command's destination address - Group 1 �鲥����
  SampleApp_Flash_DstAddr.addrMode = (afAddrMode_t)afAddrGroup; //��Ѱַ
  SampleApp_Flash_DstAddr.endPoint = SAMPLEAPP_ENDPOINT; //ָ���˵��
  SampleApp_Flash_DstAddr.addr.shortAddr = SAMPLEAPP_FLASH_GROUP;//���0x0001
  
  SampleApp_Single_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
  SampleApp_Single_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_Single_DstAddr.addr.shortAddr = 0x0000;

  // Fill out the endpoint description. ���屾�豸����ͨ�ŵ�APS��˵�������
  SampleApp_epDesc.endPoint = SAMPLEAPP_ENDPOINT; //ָ���˵��
  SampleApp_epDesc.task_id = &SampleApp_TaskID;   //SampleApp ������������ID
  SampleApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;//SampleApp��������
  SampleApp_epDesc.latencyReq = noLatencyReqs;    //��ʱ����

  // Register the endpoint description with the AF
  afRegister( &SampleApp_epDesc );    //��AF��Ǽ�������

  // Register for all key events - This app will handle all key events
  RegisterForKeys( SampleApp_TaskID ); // �Ǽ����еİ����¼�

  // By default, all devices start out in Group 1
  SampleApp_Group.ID = 0x0001;//���
  osal_memcpy( SampleApp_Group.name, "Group 1", 7  );//�趨����
  aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );//�Ѹ���Ǽ���ӵ�APS��

#if defined ( LCD_SUPPORTED )
  HalLcdWriteString( "SampleApp", HAL_LCD_LINE_1 ); //���֧��LCD����ʾ��ʾ��Ϣ
#endif
  
 // register for end device annce and simple descriptor responses
 ZDO_RegisterForZDOMsg( SampleApp_TaskID, Device_annce );
  
  
  InitT1();
  InitT3();
  
  #if defined(ZDO_COORDINATOR)                      //Э�����̵�ַΪ0000
  {
    device[0] = 0x0000;
  }
  #endif
}

/*********************************************************************
 * @fn      SampleApp_ProcessEvent
 *
 * @brief   Generic Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  none
 */
//�û�Ӧ��������¼�������
uint16 SampleApp_ProcessEvent( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG ) //����ϵͳ��Ϣ�ٽ����ж�
  {
    //�������ڱ�Ӧ������SampleApp����Ϣ����SampleApp_TaskID���
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        // Received when a key is pressed
        case KEY_CHANGE://�����¼�
          SampleApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        // Received when a messages is received (OTA) for this endpoint
        case AF_INCOMING_MSG_CMD://���������¼�,���ú���AF_DataRequest()��������
          SampleApp_MessageMSGCB( MSGpkt );//���ûص��������յ������ݽ��д���
          break;
	  
	case ZDO_CB_MSG:
	  SApp_ProcessMsgCBs( (zdoIncomingMsg_t *)MSGpkt);
	  break;

        // Received whenever the device changes state in the network
        case ZDO_STATE_CHANGE:
	  
	  
	  
          //ֻҪ����״̬�����ı䣬��ͨ��ZDO_STATE_CHANGE�¼�֪ͨ���е�����
          //ͬʱ��ɶ�Э������·�������ն˵�����
          SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
	  
	  
	 /**********************************************\
	  
          if ( (SampleApp_NwkState == DEV_ZB_COORD)//ʵ����Э����ֻ������������ȡ�������¼�
          if ( (SampleApp_NwkState == DEV_ROUTER) || (SampleApp_NwkState == DEV_END_DEVICE) )
          //{
            // Start sending the periodic message in a regular interval.
            //�����ʱ��ֻ��Ϊ����������Ϣ�����ģ��豸������ʼ��������￪ʼ
            //������һ��������Ϣ�ķ��ͣ�Ȼ���ܶ���ʼ��ȥ
	  
	  \***************************************************/
	  
	  
            osal_start_timerEx( SampleApp_TaskID,
                              SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
                              SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT );
          
	    
	    
	   /**********************************************\
	    
	  }  
          else
          {
            // Device is no longer in the network
          }
	    
	    \***************************************************/
	   
          break;

        default:
          break;
      }

      // Release the memory �¼��������ˣ��ͷ���Ϣռ�õ��ڴ�
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next - if one is available ָ��ָ����һ�����ڻ������Ĵ�������¼���
      //����while ( MSGpkt )���´����¼���ֱ��������û�еȴ������¼�Ϊֹ
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    }

    // return unprocessed events ����δ������¼�
    return (events ^ SYS_EVENT_MSG);
  }

  // Send a message out - This event is generated by a timer
  //  (setup in SampleApp_Init()).
  if ( events & SAMPLEAPP_SEND_PERIODIC_MSG_EVT )
  {
    // Send the periodic message �����������¼���
    //����SampleApp_SendPeriodicMessage()�����굱ǰ���������¼���Ȼ��������ʱ��
    //������һ�����������飬����һ��ѭ����ȥ��Ҳ��������˵���������¼��ˣ�
    //������Ϊ��������ʱ�ɼ����ϴ�����
    SampleApp_SendPeriodicMessage();

    // Setup to send message again in normal period (+ a little jitter)
    osal_start_timerEx( SampleApp_TaskID, SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
        (SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT + (osal_rand() & 0x00FF)) );

    // return unprocessed events ����δ������¼�
    return (events ^ SAMPLEAPP_SEND_PERIODIC_MSG_EVT);
  }

  // Discard unknown events
  return 0;
}


/*********************************************************************
 * Event Generation Functions
 */
/*********************************************************************
 * @fn      SampleApp_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
void SampleApp_HandleKeys( uint8 shift, uint8 keys ) //��ʵ��û���õ��������ٷ���
{
  (void)shift;  // Intentionally unreferenced parameter
  
  if ( keys & HAL_KEY_SW_1 )//key2
  {
    
    		
	//SampleApp_SendFlashMessage( SAMPLEAPP_FLASH_DURATION, 2);
	  	
    	switch(sel_dev)
	{
	case 0:
	  sel_dev = 1;
	  break;
	case 1:
	  sel_dev = 2;
	  break;
	case 2:
	  sel_dev = 3;
	  break;
	case 3:
	  sel_dev = 4;
	  break;
	case 4:
	  sel_dev = 5;
	  break;
	case 5:
	  sel_dev = 6;
	  break;
	case 6:
	  sel_dev = 7;
	  break;
	case 7:
	  sel_dev = 8;
	  break;
	case 8:
	  sel_dev = 9;
	  break;
	case 9:
	  sel_dev = 0;
	  break;
	}
    HalLcdWriteStringValue("sel_dev:", sel_dev, 10, 6);
    
    if(sel_dev < device_num)
    {
      	dst_dev = device[sel_dev];
    }
       
    
    
    /* This key sends the Flash Command is sent to Group 1.
     * This device will not receive the Flash Command from this
     * device (even if it belongs to group 1).
     */
    
	
    
  }

  if ( keys & HAL_KEY_SW_6 )//key1
  {
    
    
//    	#if defined(ZDO_COORDINATOR) //Э�����յ�"D1"��,����"D1"���նˣ����ն�Led1Ҳ��˸
    
    
    	/**********************************************\
    		
    		if(key_flag == 0)
		{
		 InitT1();
		 flag = 1;
		}
		else
		{
		  InitT11();
		  flag = 0;
		}
    \***************************************************/
			
//	#else
		
//		SampleApp_SendFlashMessage( SAMPLEAPP_FLASH_DURATION, 1);
		if(main_flag == 0)
		{
		  main_flag = 1;
		}
		else if(main_flag == 1)
		{
		  main_flag = 2;
		  SampleApp_SendBeepMessage(dst_dev, 1);
		}
		else if(main_flag == 2)
		{
		  main_flag = 3;
		  SampleApp_SendBeepMessage(dst_dev, 0);
		}
		else if(main_flag == 3)
		{
		  main_flag = 0;
		}
		
		HalLcdWriteStringValue("model:", main_flag, 10, 5);
	  	
//	#endif
    
    
    
    /* The Flashr Command is sent to Group 1.
     * This key toggles this device in and out of group 1.
     * If this device doesn't belong to group 1, this application
     * will not receive the Flash command sent to group 1.
     */
    
    
    /***************************************************************\
      
    aps_Group_t *grp;
    grp = aps_FindGroup( SAMPLEAPP_ENDPOINT, SAMPLEAPP_FLASH_GROUP );
    if ( grp )
    {
      // Remove from the group
      aps_RemoveGroup( SAMPLEAPP_ENDPOINT, SAMPLEAPP_FLASH_GROUP );
    }
    else
    {
      // Add to the flash group
      aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );
    }
    \****************************************************************/
    
  }
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      SampleApp_MessageMSGCB
 *
 * @brief   Data message processor callback.  This function processes
 *          any incoming data - probably from other devices.  So, based
 *          on cluster ID, perform the intended action.
 *
 * @param   none
 *
 * @return  none
 */
//�������ݣ�����Ϊ���յ�������
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  byte buf[3];
//  HalLcdWriteStringValue("GROUP:", group, 10, 4);

  switch ( pkt->clusterId ) //�жϴ�ID
  {
    case SAMPLEAPP_PERIODIC_CLUSTERID: //�յ��㲥����
      osal_memset(buf, 0 , 3);
      osal_memcpy(buf, pkt->cmd.Data, 3); //�������ݵ���������
      
      if(buf[0]=='D' && buf[1]=='1')      //�ж��յ��������Ƿ�Ϊ"D1"         
      {
	
//	if(group != buf[2])
//	  break;
	
	
	HalLcdWriteStringValue("rssi:", abs(pkt->rssi), 10, 6);
	HalLcdWriteStringValue("src addr:", pkt->srcAddr.addr.shortAddr, 16, 5);
	
	
	
	if(main_flag == 0)                	//model 0
	{
	  	beep0();
		
	}
	else if(main_flag == 1)
	{
	  	
	  	if(pkt->rssi < -75)
		{
		  if(count < 3)
		    	count++;
		  else
		  {
		    	beep1();
			count = 0;
		  }
		}
		else if(pkt->rssi > -55)
		{
		  	count = 0;
		  	beep0();
		}
		else
			count = 0;
	}
	else
	{
	  	
	}
	  
	  
      }
      else if(buf[0] == 0x00)
      {
		osal_memcpy(device, pkt->cmd.Data, pkt->cmd.DataLength);
		device_num = pkt->cmd.DataLength/2;
		HalLcdWriteStringValue("dev_num:", device_num, 10, 4);
      }
      else
      {
          	HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);                   
      }
      break;

    case SAMPLEAPP_FLASH_CLUSTERID: //�յ��鲥����
      
      
      if(pkt->cmd.Data[3] == 1)
      {
	
      }
      else if(pkt->cmd.Data[3] == 3)
      {
	
      }
      break;
      
    case SAMPLEAPP_SINGLE_CLUSTERID:
      HalLcdWriteStringValue("beep info:", pkt->srcAddr.addr.shortAddr, 16, 5);
      osal_memset(buf, 0 , 3);
      osal_memcpy(buf, pkt->cmd.Data, 3); //�������ݵ���������
      
      if(buf[0]=='B' && buf[1]=='P') 
      {
	if(buf[2] == 1)
	  beep_flag = 1;
	else
	  beep_flag = 0;
      }
	
  }
}


//����ZDO�㴫������Ϣ
void SApp_ProcessMsgCBs( zdoIncomingMsg_t *msgPtr )
{
#if defined(ZDO_COORDINATOR)
{
  
  int i = 0;
  char flag = 0;
  uint16 nwk_addr;
  byte buf[10]; 
//  HalLcdWriteStringValue("GROUP:", group, 10, 4);

  switch ( msgPtr->clusterID ) //�жϴ�ID
  {
    case Device_annce:

        osal_memset(buf, 0 , 10);
        osal_memcpy(buf, msgPtr->asdu, 10); //�������ݵ���������
	nwk_addr = buf[1]*0x100 + buf[0];
	HalLcdWriteStringValue("DevAnnce:", nwk_addr, 16, 4);
	
	
	  	for(i = 0; i < device_num; i++)
		{
		 	if(device[i] == nwk_addr)
			{
			  flag = 1;
			  break;
			}
		}
		if(flag == 0)
		{
	  		device[++device_num] = nwk_addr;
			SampleApp_SendAddrMessage( (uint8 *)device, device_num );
		}
	
      
      
//      flashTime = BUILD_UINT16(pkt->cmd.Data[1], pkt->cmd.Data[2] );
//      HalLedBlink( HAL_LED_4, 4, 50, (flashTime / 4) );
        break;
      
      
  }
}
#endif	
  
}



void SampleApp_SendBeepMessage(uint16 dst, uint8 s)
{	
//#if defined(ZDO_COORDINATOR)
  SampleApp_Single_DstAddr.addr.shortAddr = dst;
  byte SendData[3] = "BP";
  SendData[2] = s;
  HalLcdWriteStringValue("beep info:", SampleApp_Single_DstAddr.addr.shortAddr, 16, 4);
  // ����AF_DataRequest���������߹㲥��ȥ
  if( AF_DataRequest( &SampleApp_Single_DstAddr,   //����Ŀ�ĵ�ַ���˵��ַ�ʹ���ģʽ
                       &SampleApp_epDesc,            //Դ(�𸴻�ȷ��)�ն˵��������������ϵͳ������ID�ȣ�ԴEP
                       SAMPLEAPP_SINGLE_CLUSTERID, //��Profileָ������Ч�ļ�Ⱥ��
                       3,                // �������ݳ���
                       SendData,                          // �������ݻ�����
                       &SampleApp_TransID,           // ����ID��
                       AF_DISCV_ROUTE,               // ��Чλ����ķ���ѡ��
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )  //����������ͨ������ΪAF_DEFAULT_RADIUS
  {
  }
  else
  {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
    // Error occurred in request to send.
  }
//#else
  
//#endif
}



void SampleApp_SendAddrMessage(uint8 dev[], int dev_num)
{	
//#if defined(ZDO_COORDINATOR)
  
  // ����AF_DataRequest���������߹㲥��ȥ
  if( AF_DataRequest( &SampleApp_Periodic_DstAddr,   //����Ŀ�ĵ�ַ���˵��ַ�ʹ���ģʽ
                       &SampleApp_epDesc,            //Դ(�𸴻�ȷ��)�ն˵��������������ϵͳ������ID�ȣ�ԴEP
                       SAMPLEAPP_PERIODIC_CLUSTERID, //��Profileָ������Ч�ļ�Ⱥ��
                       (dev_num+1)*2,                // �������ݳ���
                       dev,                          // �������ݻ�����
                       &SampleApp_TransID,           // ����ID��
                       AF_DISCV_ROUTE,               // ��Чλ����ķ���ѡ��
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )  //����������ͨ������ΪAF_DEFAULT_RADIUS
  {
  }
  else
  {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
    // Error occurred in request to send.
  }
//#else
  
//#endif
}



/*********************************************************************
 * @fn      SampleApp_SendPeriodicMessage
 *
 * @brief   Send the periodic message.
 *
 * @param   none
 *
 * @return  none
 */
//��������������Ϣ
void SampleApp_SendPeriodicMessage( void )
{	
//#if defined(ZDO_COORDINATOR)
  
  byte SendData[3]="D1";

  // ����AF_DataRequest���������߹㲥��ȥ
  if( AF_DataRequest( &SampleApp_Periodic_DstAddr,//����Ŀ�ĵ�ַ���˵��ַ�ʹ���ģʽ
                       &SampleApp_epDesc,//Դ(�𸴻�ȷ��)�ն˵��������������ϵͳ������ID�ȣ�ԴEP
                       SAMPLEAPP_PERIODIC_CLUSTERID, //��Profileָ������Ч�ļ�Ⱥ��
                       3,       // �������ݳ���
                       SendData,// �������ݻ�����
                       &SampleApp_TransID,     // ����ID��
                       AF_DISCV_ROUTE,      // ��Чλ����ķ���ѡ��
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )  //����������ͨ������ΪAF_DEFAULT_RADIUS
  {
  }
  else
  {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
    // Error occurred in request to send.
  }
//#else
  
//#endif
}

/*********************************************************************
 * @fn      SampleApp_SendFlashMessage
 *
 * @brief   Send the flash message to group 1.
 *
 * @param   flashTime - in milliseconds
 *
 * @return  none
 */
void SampleApp_SendFlashMessage( uint16 flashTime ,uint16 data) //��ʵ��û���õ��������ٷ���
{
  uint8 buffer[4];
  buffer[0] = (uint8)(SampleAppFlashCounter++);
  buffer[1] = data;
  buffer[2] = data;
  buffer[3] = data;

  if ( AF_DataRequest( &SampleApp_Flash_DstAddr, &SampleApp_epDesc,
                       SAMPLEAPP_FLASH_CLUSTERID,
                       4,
                       buffer,
                       &SampleApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
  }
  else
  {
    // Error occurred in request to send.
  }
}

/*********************************************************************
*********************************************************************/
