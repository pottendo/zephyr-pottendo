/**
 * @file drivers/stepper/adi/tmc_reg.h
 *
 * @brief TMC Registers
 *
 */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_REG_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_REG_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Common Registers for TMC5041 and TMC51XX */
#if defined(CONFIG_STEPPER_ADI_TMC5041)

#define TMC5XXX_WRITE_BIT        0x80U
#define TMC5XXX_ADDRESS_MASK     0x7FU

 #define TMC5XXX_CLOCK_FREQ_SHIFT 24

#define TMC5XXX_GCONF 0x00
#define TMC5XXX_GSTAT 0x01

#define TMC5XXX_RAMPMODE_POSITIONING_MODE       0
#define TMC5XXX_RAMPMODE_POSITIVE_VELOCITY_MODE 1
#define TMC5XXX_RAMPMODE_NEGATIVE_VELOCITY_MODE 2
#define TMC5XXX_RAMPMODE_HOLD_MODE              3

#define TMC5XXX_SG_MIN_VALUE -64
#define TMC5XXX_SG_MAX_VALUE 63
#define TMC5XXX_SW_MODE_SG_STOP_ENABLE BIT(10)

#define TMC5XXX_COOLCONF_SG2_THRESHOLD_VALUE_SHIFT 16

#define TMC5XXX_IHOLD_MASK  GENMASK(4, 0)
#define TMC5XXX_IHOLD_SHIFT 0
#define TMC5XXX_IHOLD(n)    (((n) << TMC5XXX_IHOLD_SHIFT) & TMC5XXX_IHOLD_MASK)

#define TMC5XXX_IRUN_MASK  GENMASK(12, 8)
#define TMC5XXX_IRUN_SHIFT 8
#define TMC5XXX_IRUN(n)    (((n) << TMC5XXX_IRUN_SHIFT) & TMC5XXX_IRUN_MASK)

#define TMC5XXX_IHOLDDELAY_MASK  GENMASK(19, 16)
#define TMC5XXX_IHOLDDELAY_SHIFT 16
#define TMC5XXX_IHOLDDELAY(n)    (((n) << TMC5XXX_IHOLDDELAY_SHIFT) & TMC5XXX_IHOLDDELAY_MASK)

#define TMC5XXX_CHOPCONF_DRV_ENABLE_MASK GENMASK(3, 0)
#define TMC5XXX_CHOPCONF_MRES_MASK       GENMASK(27, 24)
#define TMC5XXX_CHOPCONF_MRES_SHIFT      24

#define TMC5XXX_RAMPSTAT_INT_MASK  GENMASK(7, 4)
#define TMC5XXX_RAMPSTAT_INT_SHIFT 4

#define TMC5XXX_RAMPSTAT_POS_REACHED_EVENT_MASK BIT(7)
#define TMC5XXX_POS_REACHED_EVENT                                                                  \
	(TMC5XXX_RAMPSTAT_POS_REACHED_EVENT_MASK >> TMC5XXX_RAMPSTAT_INT_SHIFT)

#define TMC5XXX_RAMPSTAT_STOP_SG_EVENT_MASK BIT(6)
#define TMC5XXX_STOP_SG_EVENT                                                                      \
	(TMC5XXX_RAMPSTAT_STOP_SG_EVENT_MASK >> TMC5XXX_RAMPSTAT_INT_SHIFT)

#define TMC5XXX_RAMPSTAT_STOP_RIGHT_EVENT_MASK BIT(5)
#define TMC5XXX_STOP_RIGHT_EVENT                                                                   \
	(TMC5XXX_RAMPSTAT_STOP_RIGHT_EVENT_MASK >> TMC5XXX_RAMPSTAT_INT_SHIFT)

#define TMC5XXX_RAMPSTAT_STOP_LEFT_EVENT_MASK BIT(4)
#define TMC5XXX_STOP_LEFT_EVENT                                                                    \
	(TMC5XXX_RAMPSTAT_STOP_LEFT_EVENT_MASK >> TMC5XXX_RAMPSTAT_INT_SHIFT)

#define TMC5XXX_DRV_STATUS_STST_BIT        BIT(31)
#define TMC5XXX_DRV_STATUS_SG_RESULT_MASK  GENMASK(9, 0)
#define TMC5XXX_DRV_STATUS_SG_STATUS_MASK  BIT(24)
#define TMC5XXX_DRV_STATUS_SG_STATUS_SHIFT 24

#endif

#ifdef CONFIG_STEPPER_ADI_TMC5041

#define TMC5041_MOTOR_ADDR(m)     (0x20 << (m))
#define TMC5041_MOTOR_ADDR_DRV(m) ((m) << 4)
#define TMC5041_MOTOR_ADDR_PWM(m) ((m) << 3)

/**
 * @name TMC5041 module registers
 * @anchor TMC5041_REGISTERS
 *
 * @{
 */

#define TMC5041_GCONF_POSCMP_ENABLE_SHIFT 3
#define TMC5041_GCONF_TEST_MODE_SHIFT     7
#define TMC5041_GCONF_SHAFT_SHIFT(n)      ((n) ? 8 : 9)
#define TMC5041_LOCK_GCONF_SHIFT          10

#define TMC5041_PWMCONF(motor)    (0x10 | TMC5041_MOTOR_ADDR_PWM(motor))
#define TMC5041_PWM_STATUS(motor) (0x11 | TMC5041_MOTOR_ADDR_PWM(motor))

#define TMC5041_RAMPMODE(motor)   (0x00 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_XACTUAL(motor)    (0x01 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_VACTUAL(motor)    (0x02 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_VSTART(motor)     (0x03 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_A1(motor)         (0x04 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_V1(motor)         (0x05 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_AMAX(motor)       (0x06 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_VMAX(motor)       (0x07 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_DMAX(motor)       (0x08 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_D1(motor)         (0x0A | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_VSTOP(motor)      (0x0B | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_TZEROWAIT(motor)  (0x0C | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_XTARGET(motor)    (0x0D | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_IHOLD_IRUN(motor) (0x10 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_VCOOLTHRS(motor)  (0x11 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_VHIGH(motor)      (0x12 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_SWMODE(motor)     (0x14 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_RAMPSTAT(motor)   (0x15 | TMC5041_MOTOR_ADDR(motor))
#define TMC5041_XLATCH(motor)     (0x16 | TMC5041_MOTOR_ADDR(motor))

#define TMC5041_MSLUT0(motor)     (0x60 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT1(motor)     (0x61 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT2(motor)     (0x62 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT3(motor)     (0x63 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT4(motor)     (0x64 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT5(motor)     (0x65 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT6(motor)     (0x66 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUT7(motor)     (0x67 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUTSEL(motor)   (0x68 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSLUTSTART(motor) (0x69 | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSCNT(motor)      (0x6A | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_MSCURACT(motor)   (0x6B | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_CHOPCONF(motor)   (0x6C | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_COOLCONF(motor)   (0x6D | TMC5041_MOTOR_ADDR_DRV(motor))
#define TMC5041_DRVSTATUS(motor)  (0x6F | TMC5041_MOTOR_ADDR_DRV(motor))

#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_REG_H_ */