/*
 * Switch.c
 *
 *  Created on: 8/22/2024
 *      Author:
 */
#include <ti/devices/msp/msp.h>
#include "../inc/LaunchPad.h"
// LaunchPad.h defines all the indices into the PINCM table

//#define PB1INDEX  17 // UART0_TX  SPI1_CS2  TIMA1_C0  TIMA0_C2
//#define PB2INDEX  18 // UART0_RX  SPI1_CS3  TIMA1_C1  TIMA0_C2N
//#define PB3INDEX  19 // UART3_TX  UART2_CTS I2C1_SCL  TIMA0_C3  UART1_CTS TIMG6_C0  TIMA1_C0
//<tr><td>PB6INDEX<td>22<td>UART1_TX<td>SPI1_CS0<td>SPI0_CS1<td>TIMG8_C0<td>UART2_CTS<td>TIMG6_C0<td>TIMA1_C0N<td>
//<tr><td>PB7INDEX<td>23<td>UART1_RX<td>SPI1_POCI<td>SPI0_CS2<td>TIMG8_C1<td>UART2_RTS<td>TIMG6_C1<td>TIMA1_C1N<td>
//<tr><td>PB8INDEX<td>24<td>UART1_CTS<td>SPI1_PICO<td>TIMA0_C0<td>COMP1_OUT<td> <td> <td> <td>


void Switch_Init(void){
    // write this
    IOMUX->SECCFG.PINCM[PB13INDEX] = (uint32_t) 0x00040081; // GPIOB Pin 1
    IOMUX->SECCFG.PINCM[PB17INDEX] = (uint32_t) 0x00040081; // GPIOB Pin 2
    IOMUX->SECCFG.PINCM[PB19INDEX] = (uint32_t) 0x00040081; // GPIOB Pin 3
    //GPIOB->DOE31_0 &= ~(uint32_t)0x0E; // Enable all these inputs
}
// return current state of switches
uint32_t Switch_In(void){
    return (GPIOB->DIN31_0); // PIN 1 = top button/english select, PIN 2 = bottom button/spanish select,
                              // PIN 3 = right button/player shoot select
    // 1 = english
    // 2 = spanish
    // 4 = player shoot
}
