#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUTOBAUD                  0xffffffff


typedef enum _EPortType
{
  PIO_NOT_A_PORT=-1,
  PIO_GPIO=0,
} EPortType ;

typedef enum _EPioType
{
  PIO_NOT_A_PIN=-1,
  PIO_EXTINT=0,
  PIO_ANALOG,
  PIO_TIMER,
  PIO_DIGITAL,
  PIO_INPUT,
  PIO_INPUT_PULLUP,
  PIO_OUTPUT,
  PIO_EXTERNAL,

  PIO_PWM=PIO_TIMER,
} EPioType ;


/* Types used for the table below */
typedef struct _PinDescription
{
  EPortType       ulPort ;
  uint32_t        ulPin ;
  EPioType        ulPinType ;
} PinDescription ;

/* Pins table to be instantiated into variant.cpp */
extern const PinDescription g_APinDescription[] ;

#ifdef __cplusplus
} // extern "C"
#endif