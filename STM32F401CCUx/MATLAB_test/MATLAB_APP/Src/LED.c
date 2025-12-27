/*
 * File: LED.c
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

#include "LED.h"
#include "rtwtypes.h"

/* Block states (default storage) */
DW_LED_T LED_DW;

/* External outputs (root outports fed by signals with default storage) */
ExtY_LED_T LED_Y;

/* Real-time model */
static RT_MODEL_LED_T LED_M_;
RT_MODEL_LED_T *const LED_M = &LED_M_;

/* Model step function */
void LED_step(void)
{
    boolean_T rtb_Delay;

    /* Delay: '<Root>/Delay' */
    rtb_Delay = LED_DW.Delay_DSTATE[0];

    /* Outport: '<Root>/LED' incorporates:
     *  Delay: '<Root>/Delay'
     */
    LED_Y.LED_e = LED_DW.Delay_DSTATE[0];

    /* Update for Delay: '<Root>/Delay' incorporates:
     *  Logic: '<Root>/NOT'
     */
    LED_DW.Delay_DSTATE[0] = LED_DW.Delay_DSTATE[1];
    LED_DW.Delay_DSTATE[1] = !rtb_Delay;
}

/* Model initialize function */
void LED_initialize(void)
{
    /* 初始化LED状态 - 确保LED从关闭状态开始 */
    LED_DW.Delay_DSTATE[0] = false;
    LED_DW.Delay_DSTATE[1] = false;
    LED_Y.LED_e = false;
}

/* Model terminate function */
void LED_terminate(void)
{
    /* (no terminate code required) */
}

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
