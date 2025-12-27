/*
 * File: LED.h
 *
 * Code generated for Simulink model 'LED'.
 *
 * Model version                  : 1.2
 * Simulink Coder version         : 25.2 (R2025b) 28-Jul-2025
 * C/C++ source code generated on : Fri Oct  3 10:53:31 2025
 *
 * Target selection: ert.tlc
 * Embedded hardware selection: Custom Processor->Custom Processor
 * Code generation objectives: Unspecified
 * Validation result: Not run
 */

#ifndef LED_h_
#define LED_h_
#ifndef LED_COMMON_INCLUDES_
#define LED_COMMON_INCLUDES_
#include "rtwtypes.h"
#include "math.h"
#endif                                 /* LED_COMMON_INCLUDES_ */

/* Macros for accessing real-time model data structure */
#ifndef rtmGetErrorStatus
#define rtmGetErrorStatus(rtm)         ((rtm)->errorStatus)
#endif

#ifndef rtmSetErrorStatus
#define rtmSetErrorStatus(rtm, val)    ((rtm)->errorStatus = (val))
#endif

/* Forward declaration for rtModel */
typedef struct tag_RTM_LED_T RT_MODEL_LED_T;

/* Block states (default storage) for system '<Root>' */
typedef struct {
    boolean_T Delay_DSTATE[2];         /* '<Root>/Delay' */
} DW_LED_T;

/* External outputs (root outports fed by signals with default storage) */
typedef struct {
    boolean_T LED_e;                   /* '<Root>/LED' */
} ExtY_LED_T;

/* Real-time Model Data Structure */
struct tag_RTM_LED_T {
    const char_T * volatile errorStatus;
};

/* Block states (default storage) */
extern DW_LED_T LED_DW;

/* External outputs (root outports fed by signals with default storage) */
extern ExtY_LED_T LED_Y;

/* Model entry point functions */
extern void LED_initialize(void);
extern void LED_step(void);
extern void LED_terminate(void);

/* Real-time Model object */
extern RT_MODEL_LED_T *const LED_M;

/*-
 * These blocks were eliminated from the model due to optimizations:
 *
 * Block '<Root>/Scope' : Unused code path elimination
 */

/*-
 * The generated code includes comments that allow you to trace directly
 * back to the appropriate location in the model.  The basic format
 * is <system>/block_name, where system is the system number (uniquely
 * assigned by Simulink) and block_name is the name of the block.
 *
 * Use the MATLAB hilite_system command to trace the generated code back
 * to the model.  For example,
 *
 * hilite_system('<S3>')    - opens system 3
 * hilite_system('<S3>/Kp') - opens and selects block Kp which resides in S3
 *
 * Here is the system hierarchy for this model
 *
 * '<Root>' : 'LED'
 */
#endif                                 /* LED_h_ */

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
