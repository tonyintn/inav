/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "platform.h"

#include "build/build_config.h"

#include "common/axis.h"

#include "drivers/logging.h"

#include "drivers/io.h"
#include "drivers/system.h"
#include "drivers/exti.h"

#include "drivers/sensor.h"

#include "drivers/accgyro.h"
#include "drivers/accgyro_adxl345.h"
#include "drivers/accgyro_bma280.h"
#include "drivers/accgyro_fake.h"
#include "drivers/accgyro_l3g4200d.h"
#include "drivers/accgyro_mma845x.h"
#include "drivers/accgyro_mpu.h"
#include "drivers/accgyro_mpu3050.h"
#include "drivers/accgyro_mpu6050.h"
#include "drivers/accgyro_mpu6500.h"
#include "drivers/accgyro_l3gd20.h"
#include "drivers/accgyro_lsm303dlhc.h"

#include "drivers/bus_spi.h"
#include "drivers/accgyro_spi_mpu6000.h"
#include "drivers/accgyro_spi_mpu6500.h"
#include "drivers/accgyro_spi_mpu9250.h"
#include "drivers/gyro_sync.h"

#include "drivers/barometer.h"
#include "drivers/barometer_bmp085.h"
#include "drivers/barometer_bmp280.h"
#include "drivers/barometer_fake.h"
#include "drivers/barometer_ms5611.h"

#include "drivers/pitotmeter.h"
#include "drivers/pitotmeter_ms4525.h"
#include "drivers/pitotmeter_fake.h"

#include "drivers/compass.h"
#include "drivers/compass_ak8963.h"
#include "drivers/compass_ak8975.h"
#include "drivers/compass_fake.h"
#include "drivers/compass_hmc5883l.h"
#include "drivers/compass_mag3110.h"

#include "drivers/sonar_hcsr04.h"
#include "drivers/sonar_srf10.h"

#include "fc/runtime_config.h"

#include "config/config.h"
#include "config/feature.h"

#include "io/gps.h"

#include "sensors/sensors.h"
#include "sensors/acceleration.h"
#include "sensors/barometer.h"
#include "sensors/pitotmeter.h"
#include "sensors/gyro.h"
#include "sensors/compass.h"
#include "sensors/rangefinder.h"
#include "sensors/initialisation.h"

#ifdef USE_HARDWARE_REVISION_DETECTION
#include "hardware_revision.h"
#endif

extern baro_t baro;
extern mag_t mag;
extern sensor_align_e gyroAlign;
extern pitot_t pitot;

uint8_t detectedSensors[SENSOR_INDEX_COUNT] = { GYRO_NONE, ACC_NONE, BARO_NONE, MAG_NONE, RANGEFINDER_NONE, PITOT_NONE };


const extiConfig_t *selectMPUIntExtiConfig(void)
{
#if defined(MPU_INT_EXTI)
    static const extiConfig_t mpuIntExtiConfig = { .tag = IO_TAG(MPU_INT_EXTI) };
    return &mpuIntExtiConfig;
#elif defined(USE_HARDWARE_REVISION_DETECTION)
    return selectMPUIntExtiConfigByHardwareRevision();
#else
    return NULL;
#endif
}

bool detectGyro(void)
{
    gyroSensor_e gyroHardware = GYRO_DEFAULT;

    gyroAlign = ALIGN_DEFAULT;

    switch(gyroHardware) {
        case GYRO_DEFAULT:
            ; // fallthrough
        case GYRO_MPU6050:
#ifdef USE_GYRO_MPU6050
            if (mpu6050GyroDetect(&gyro)) {
                gyroHardware = GYRO_MPU6050;
#ifdef GYRO_MPU6050_ALIGN
                gyroAlign = GYRO_MPU6050_ALIGN;
#endif
                break;
            }
#endif
            ; // fallthrough
        case GYRO_L3G4200D:
#ifdef USE_GYRO_L3G4200D
            if (l3g4200dDetect(&gyro)) {
                gyroHardware = GYRO_L3G4200D;
#ifdef GYRO_L3G4200D_ALIGN
                gyroAlign = GYRO_L3G4200D_ALIGN;
#endif
                break;
            }
#endif
            ; // fallthrough

        case GYRO_MPU3050:
#ifdef USE_GYRO_MPU3050
            if (mpu3050Detect(&gyro)) {
                gyroHardware = GYRO_MPU3050;
#ifdef GYRO_MPU3050_ALIGN
                gyroAlign = GYRO_MPU3050_ALIGN;
#endif
                break;
            }
#endif
            ; // fallthrough

        case GYRO_L3GD20:
#ifdef USE_GYRO_L3GD20
            if (l3gd20Detect(&gyro)) {
                gyroHardware = GYRO_L3GD20;
#ifdef GYRO_L3GD20_ALIGN
                gyroAlign = GYRO_L3GD20_ALIGN;
#endif
                break;
            }
#endif
            ; // fallthrough

        case GYRO_MPU6000:
#ifdef USE_GYRO_SPI_MPU6000
            if (mpu6000SpiGyroDetect(&gyro)) {
                gyroHardware = GYRO_MPU6000;
#ifdef GYRO_MPU6000_ALIGN
                gyroAlign = GYRO_MPU6000_ALIGN;
#endif
                break;
            }
#endif
            ; // fallthrough

        case GYRO_MPU6500:
#if defined(USE_GYRO_MPU6500) || defined(USE_GYRO_SPI_MPU6500)
#ifdef USE_GYRO_SPI_MPU6500
            if (mpu6500GyroDetect(&gyro) || mpu6500SpiGyroDetect(&gyro)) {
#else
            if (mpu6500GyroDetect(&gyro)) {
#endif
                gyroHardware = GYRO_MPU6500;
#ifdef GYRO_MPU6500_ALIGN
                gyroAlign = GYRO_MPU6500_ALIGN;
#endif

                break;
            }
#endif
            ; // fallthrough

    case GYRO_MPU9250:
#ifdef USE_GYRO_SPI_MPU9250
        if (mpu9250SpiGyroDetect(&gyro))
        {
            gyroHardware = GYRO_MPU9250;
#ifdef GYRO_MPU9250_ALIGN
            gyroAlign = GYRO_MPU9250_ALIGN;
#endif

            break;
        }
#endif
        ; // fallthrough
        case GYRO_FAKE:
#ifdef USE_FAKE_GYRO
            if (fakeGyroDetect(&gyro)) {
                gyroHardware = GYRO_FAKE;
                break;
            }
#endif
            ; // fallthrough
        case GYRO_NONE:
            gyroHardware = GYRO_NONE;
    }

    addBootlogEvent6(BOOT_EVENT_GYRO_DETECTION, BOOT_EVENT_FLAGS_NONE, gyroHardware, 0, 0, 0);

    if (gyroHardware == GYRO_NONE) {
        return false;
    }

    detectedSensors[SENSOR_INDEX_GYRO] = gyroHardware;
    sensorsSet(SENSOR_GYRO);

    return true;
}

static bool detectAcc(accelerationSensor_e accHardwareToUse)
{
    accelerationSensor_e accHardware;

#ifdef USE_ACC_ADXL345
    drv_adxl345_config_t acc_params;
#endif

retry:
    accAlign = ALIGN_DEFAULT;

    switch (accHardwareToUse) {
        case ACC_DEFAULT:
            ; // fallthrough
        case ACC_ADXL345: // ADXL345
#ifdef USE_ACC_ADXL345
            acc_params.useFifo = false;
            acc_params.dataRate = 800; // unused currently
#ifdef NAZE
            if (hardwareRevision < NAZE32_REV5 && adxl345Detect(&acc_params, &acc)) {
#else
            if (adxl345Detect(&acc_params, &acc)) {
#endif
#ifdef ACC_ADXL345_ALIGN
                accAlign = ACC_ADXL345_ALIGN;
#endif
                accHardware = ACC_ADXL345;
                break;
            }
#endif
            ; // fallthrough
        case ACC_LSM303DLHC:
#ifdef USE_ACC_LSM303DLHC
            if (lsm303dlhcAccDetect(&acc)) {
#ifdef ACC_LSM303DLHC_ALIGN
                accAlign = ACC_LSM303DLHC_ALIGN;
#endif
                accHardware = ACC_LSM303DLHC;
                break;
            }
#endif
            ; // fallthrough
        case ACC_MPU6050: // MPU6050
#ifdef USE_ACC_MPU6050
            if (mpu6050AccDetect(&acc)) {
#ifdef ACC_MPU6050_ALIGN
                accAlign = ACC_MPU6050_ALIGN;
#endif
                accHardware = ACC_MPU6050;
                break;
            }
#endif
            ; // fallthrough
        case ACC_MMA8452: // MMA8452
#ifdef USE_ACC_MMA8452
#ifdef NAZE
            // Not supported with this frequency
            if (hardwareRevision < NAZE32_REV5 && mma8452Detect(&acc)) {
#else
            if (mma8452Detect(&acc)) {
#endif
#ifdef ACC_MMA8452_ALIGN
                accAlign = ACC_MMA8452_ALIGN;
#endif
                accHardware = ACC_MMA8452;
                break;
            }
#endif
            ; // fallthrough
        case ACC_BMA280: // BMA280
#ifdef USE_ACC_BMA280
            if (bma280Detect(&acc)) {
#ifdef ACC_BMA280_ALIGN
                accAlign = ACC_BMA280_ALIGN;
#endif
                accHardware = ACC_BMA280;
                break;
            }
#endif
            ; // fallthrough
        case ACC_MPU6000:
#ifdef USE_ACC_SPI_MPU6000
            if (mpu6000SpiAccDetect(&acc)) {
#ifdef ACC_MPU6000_ALIGN
                accAlign = ACC_MPU6000_ALIGN;
#endif
                accHardware = ACC_MPU6000;
                break;
            }
#endif
            ; // fallthrough
        case ACC_MPU6500:
#if defined(USE_ACC_MPU6500) || defined(USE_ACC_SPI_MPU6500)
#ifdef USE_ACC_SPI_MPU6500
            if (mpu6500AccDetect(&acc) || mpu6500SpiAccDetect(&acc)) {
#else
            if (mpu6500AccDetect(&acc)) {
#endif
#ifdef ACC_MPU6500_ALIGN
                accAlign = ACC_MPU6500_ALIGN;
#endif
                accHardware = ACC_MPU6500;
                break;
            }
#endif
        case ACC_MPU9250:
#ifdef USE_ACC_SPI_MPU9250
            if (mpu9250SpiAccDetect(&acc)) {
#ifdef ACC_MPU9250_ALIGN
                accAlign = ACC_MPU9250_ALIGN;
#endif
                accHardware = ACC_MPU9250;
                break;
            }
#endif
            ; // fallthrough
        case ACC_FAKE:
#ifdef USE_FAKE_ACC
            if (fakeAccDetect(&acc)) {
                accHardware = ACC_FAKE;
                break;
            }
#endif
            ; // fallthrough
        case ACC_NONE: // disable ACC
            accHardware = ACC_NONE;
            break;

    }

    // Found anything? Check if error or ACC is really missing.
    if (accHardware == ACC_NONE && accHardwareToUse != ACC_DEFAULT && accHardwareToUse != ACC_NONE) {
        // Nothing was found and we have a forced sensor that isn't present.
        accHardwareToUse = ACC_DEFAULT;
        goto retry;
    }

    addBootlogEvent6(BOOT_EVENT_ACC_DETECTION, BOOT_EVENT_FLAGS_NONE, accHardware, 0, 0, 0);

    if (accHardware == ACC_NONE) {
        return false;
    }

    detectedSensors[SENSOR_INDEX_ACC] = accHardware;
    sensorsSet(SENSOR_ACC);
    return true;
}

#ifdef BARO
static bool detectBaro(baroSensor_e baroHardwareToUse)
{
    // Detect what pressure sensors are available. baro->update() is set to sensor-specific update function

    baroSensor_e baroHardware = baroHardwareToUse;

#ifdef USE_BARO_BMP085

    const bmp085Config_t *bmp085Config = NULL;

#if defined(BARO_XCLR_GPIO) && defined(BARO_EOC_GPIO)
    static const bmp085Config_t defaultBMP085Config = {
        .xclrIO = IO_TAG(BARO_XCLR_PIN),
        .eocIO = IO_TAG(BARO_EOC_PIN),
    };
    bmp085Config = &defaultBMP085Config;
#endif

#ifdef NAZE
    if (hardwareRevision == NAZE32) {
        bmp085Disable(bmp085Config);
    }
#endif

#endif

    switch (baroHardware) {
        case BARO_DEFAULT:
            ; // fallthough

        case BARO_BMP085:
#ifdef USE_BARO_BMP085
            if (bmp085Detect(bmp085Config, &baro)) {
                baroHardware = BARO_BMP085;
                break;
            }
#endif
        ; // fallthough
        case BARO_MS5611:
#ifdef USE_BARO_MS5611
            if (ms5611Detect(&baro)) {
                baroHardware = BARO_MS5611;
                break;
            }
#endif
            ; // fallthough
        case BARO_BMP280:
#if defined(USE_BARO_BMP280) || defined(USE_BARO_SPI_BMP280)
            if (bmp280Detect(&baro)) {
                baroHardware = BARO_BMP280;
                break;
            }
#endif
            ; // fallthrough
        case BARO_FAKE:
#ifdef USE_FAKE_BARO
            if (fakeBaroDetect(&baro)) {
                baroHardware = BARO_FAKE;
                break;
            }
#endif
            ; // fallthrough
        case BARO_NONE:
            baroHardware = BARO_NONE;
            break;
    }

    addBootlogEvent6(BOOT_EVENT_BARO_DETECTION, BOOT_EVENT_FLAGS_NONE, baroHardware, 0, 0, 0);

    if (baroHardware == BARO_NONE) {
        return false;
    }

    detectedSensors[SENSOR_INDEX_BARO] = baroHardware;
    sensorsSet(SENSOR_BARO);
    return true;
}
#endif // BARO

#ifdef PITOT
static bool detectPitot(uint8_t pitotHardwareToUse)
{
    pitotSensor_e pitotHardware = pitotHardwareToUse;

    switch (pitotHardware) {
        case PITOT_DEFAULT:
            ; // Fallthrough

        case PITOT_MS4525:
#ifdef USE_PITOT_MS4525
            if (ms4525Detect(&pitot)) {
                pitotHardware = PITOT_MS4525;
                break;
            }
#endif
            ; // Fallthrough

        case PITOT_FAKE:
#ifdef USE_PITOT_FAKE
            if (fakePitotDetect(&pitot)) {
                pitotHardware = PITOT_FAKE;
                break;
            }
#endif
            ; // Fallthrough

        case PITOT_NONE:
            pitotHardware = PITOT_NONE;
            break;
    }

    addBootlogEvent6(BOOT_EVENT_PITOT_DETECTION, BOOT_EVENT_FLAGS_NONE, pitotHardware, 0, 0, 0);

    if (pitotHardware == PITOT_NONE) {
        sensorsClear(SENSOR_PITOT);
        return false;
    }

    detectedSensors[SENSOR_INDEX_PITOT] = pitotHardware;
    sensorsSet(SENSOR_PITOT);
    return true;
}
#endif

#ifdef MAG
static bool detectMag(magSensor_e magHardwareToUse)
{
    magSensor_e magHardware = MAG_NONE;

#ifdef USE_MAG_HMC5883
    const hmc5883Config_t *hmc5883Config = 0;

#ifdef NAZE // TODO remove this target specific define
    static const hmc5883Config_t nazeHmc5883Config_v1_v4 = {
            .intTag = IO_TAG(PB12) /* perhaps disabled? */
    };
    static const hmc5883Config_t nazeHmc5883Config_v5 = {
            .intTag = IO_TAG(MAG_INT_EXTI)
    };
    if (hardwareRevision < NAZE32_REV5) {
        hmc5883Config = &nazeHmc5883Config_v1_v4;
    } else {
        hmc5883Config = &nazeHmc5883Config_v5;
    }
#endif

#ifdef MAG_INT_EXTI
    static const hmc5883Config_t extiHmc5883Config = {
        .intTag = IO_TAG(MAG_INT_EXTI)
    };

    hmc5883Config = &extiHmc5883Config;
#endif

#endif

    magAlign = ALIGN_DEFAULT;

    switch(magHardwareToUse) {
        case MAG_DEFAULT:
            ; // fallthrough

        case MAG_HMC5883:
#ifdef USE_MAG_HMC5883
            if (hmc5883lDetect(&mag, hmc5883Config)) {
#ifdef MAG_HMC5883_ALIGN
                magAlign = MAG_HMC5883_ALIGN;
#endif
                magHardware = MAG_HMC5883;
                break;
            }
#endif
            ; // fallthrough

        case MAG_AK8975:
#ifdef USE_MAG_AK8975
            if (ak8975Detect(&mag)) {
#ifdef MAG_AK8975_ALIGN
                magAlign = MAG_AK8975_ALIGN;
#endif
                magHardware = MAG_AK8975;
                break;
            }
#endif
            ; // fallthrough

        case MAG_AK8963:
#ifdef USE_MAG_AK8963
            if (ak8963Detect(&mag)) {
#ifdef MAG_AK8963_ALIGN
                magAlign = MAG_AK8963_ALIGN;
#endif
                magHardware = MAG_AK8963;
                break;
            }
#endif
            ; // fallthrough

        case MAG_GPS:
#ifdef GPS
            if (gpsMagDetect(&mag)) {
#ifdef MAG_GPS_ALIGN
                magAlign = MAG_GPS_ALIGN;
#endif
                magHardware = MAG_GPS;
                break;
            }
#endif
            ; // fallthrough

        case MAG_MAG3110:
#ifdef USE_MAG_MAG3110
            if (mag3110detect(&mag)) {
#ifdef MAG_MAG3110_ALIGN
                magAlign = MAG_MAG3110_ALIGN;
#endif
                magHardware = MAG_MAG3110;
                break;
            }
#endif
            ; // fallthrough

        case MAG_FAKE:
#ifdef USE_FAKE_MAG
            if (fakeMagDetect(&mag)) {
                magHardware = MAG_FAKE;
                break;
            }
#endif
            ; // fallthrough

        case MAG_NONE:
            magHardware = MAG_NONE;
            break;
    }

    addBootlogEvent6(BOOT_EVENT_MAG_DETECTION, BOOT_EVENT_FLAGS_NONE, magHardware, 0, 0, 0);

    // If not in autodetect mode and detected the wrong chip - disregard the compass even if detected
    if ((magHardwareToUse != MAG_DEFAULT && magHardware != magHardwareToUse) || (magHardware == MAG_NONE)) {
        return false;
    }

    detectedSensors[SENSOR_INDEX_MAG] = magHardware;
    sensorsSet(SENSOR_MAG);
    return true;
}
#endif // MAG

#ifdef SONAR
/*
 * Detect which rangefinder is present
 */
static rangefinderType_e detectRangefinder(void)
{
    rangefinderType_e rangefinderType = RANGEFINDER_NONE;
    if (feature(FEATURE_SONAR)) {
        // the user has set the sonar feature, so assume they have an HC-SR04 plugged in,
        // since there is no way to detect it
        rangefinderType = RANGEFINDER_HCSR04;
    }
#ifdef USE_SONAR_SRF10
    if (srf10_detect()) {
        // if an SFR10 sonar rangefinder is detected then use it in preference to the assumed HC-SR04
        rangefinderType = RANGEFINDER_SRF10;
    }
#endif

    addBootlogEvent6(BOOT_EVENT_RANGEFINDER_DETECTION, BOOT_EVENT_FLAGS_NONE, rangefinderType, 0, 0, 0);

    detectedSensors[SENSOR_INDEX_RANGEFINDER] = rangefinderType;

    if (rangefinderType != RANGEFINDER_NONE) {
        sensorsSet(SENSOR_SONAR);
    }

    return rangefinderType;
}
#endif

static void reconfigureAlignment(const sensorAlignmentConfig_t *sensorAlignmentConfig)
{
    if (sensorAlignmentConfig->gyro_align != ALIGN_DEFAULT) {
        gyroAlign = sensorAlignmentConfig->gyro_align;
    }
    if (sensorAlignmentConfig->acc_align != ALIGN_DEFAULT) {
        accAlign = sensorAlignmentConfig->acc_align;
    }
    if (sensorAlignmentConfig->mag_align != ALIGN_DEFAULT) {
        magAlign = sensorAlignmentConfig->mag_align;
    }
}

bool sensorsAutodetect(sensorAlignmentConfig_t *sensorAlignmentConfig,
        uint8_t accHardwareToUse,
        uint8_t magHardwareToUse,
        uint8_t baroHardwareToUse,
        uint8_t pitotHardwareToUse,
        int16_t magDeclinationFromConfig,
        uint32_t looptime,
        uint8_t gyroLpf,
        uint8_t gyroSync,
        uint8_t gyroSyncDenominator)
{
    memset(&acc, 0, sizeof(acc));
    memset(&gyro, 0, sizeof(gyro));

#if defined(USE_GYRO_MPU6050) || defined(USE_GYRO_MPU3050) || defined(USE_GYRO_MPU6500) || defined(USE_GYRO_SPI_MPU6500) || defined(USE_GYRO_SPI_MPU6000) || defined(USE_ACC_MPU6050) || defined(USE_GYRO_SPI_MPU9250)
    const extiConfig_t *extiConfig = selectMPUIntExtiConfig();
    detectMpu(extiConfig);
#endif

    if (!detectGyro()) {
        return false;
    }

    // this is safe because either mpu6050 or mpu3050 or lg3d20 sets it, and in case of fail, we never get here.
    gyro.targetLooptime = gyroSetSampleRate(looptime, gyroLpf, gyroSync, gyroSyncDenominator);    // Set gyro sample rate before initialisation
    gyro.init(gyroLpf); // driver initialisation
    gyroInit(); // sensor initialisation

    if (detectAcc(accHardwareToUse)) {
        acc.acc_1G = 256; // set default
        acc.init(&acc);
    #ifdef ASYNC_GYRO_PROCESSING
        /*
         * ACC will be updated at its own rate
         */
        accInit(getAccUpdateRate());
    #else
        /*
         * acc updated at same frequency in taskMainPidLoop in mw.c
         */
        accInit(gyro.targetLooptime);
    #endif
    }

#ifdef BARO
    detectBaro(baroHardwareToUse);
#else
    UNUSED(baroHardwareToUse);
#endif

#ifdef PITOT
    detectPitot(pitotHardwareToUse);
#else
    UNUSED(pitotHardwareToUse);
#endif

    // FIXME extract to a method to reduce dependencies, maybe move to sensors_compass.c
    magneticDeclination = 0.0f; // TODO investigate if this is actually needed if there is no mag sensor or if the value stored in the config should be used.
#ifdef MAG
    if (detectMag(magHardwareToUse)) {
        // calculate magnetic declination
        if (!compassInit(magDeclinationFromConfig)) {
            addBootlogEvent2(BOOT_EVENT_MAG_INIT_FAILED, BOOT_EVENT_FLAGS_ERROR);
            sensorsClear(SENSOR_MAG);
        }
    }
#else
    UNUSED(magHardwareToUse);
    UNUSED(magDeclinationFromConfig);
#endif

#ifdef SONAR
    const rangefinderType_e rangefinderType = detectRangefinder();
    rangefinderInit(rangefinderType);
#endif
    reconfigureAlignment(sensorAlignmentConfig);

    return true;
}
