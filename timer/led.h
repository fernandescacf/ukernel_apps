/**
 * @file        led.h
 * @author      Carlos Fernandes
 * @version     1.0
 * @date        28 June, 2020
 * @brief       Sunxi H3 Led Driver header file
*/

#ifndef _LED_H_
#define _LED_H_

#ifdef __cplusplus
    extern "C" {
#endif


/* Includes ----------------------------------------------- */
#include <types.h>


/* Exported types ----------------------------------------- */



/* Exported constants ------------------------------------- */
#define LED_OFF         0x0
#define LED_ON          0x1

/* Exported macros ---------------------------------------- */


/* Exported functions ------------------------------------- */

int32_t LedDriverInit(void);

void LedDriverClose(void);

int32_t LedSetState(int32_t state);


#ifdef __cplusplus
    }
#endif

#endif