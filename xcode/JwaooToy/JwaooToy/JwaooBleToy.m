//
//  JwaooToy.m
//  TestBle
//
//  Created by 曹福昂 on 16/7/7.
//  Copyright © 2016年 曹福昂. All rights reserved.
//

#import "JwaooBleToy.h"
#import "CavanHexFile.h"
#import "Mpu6050Sensor.h"

@implementation JwaooBleToy

- (int) freq {
    return mParser.freq;
}

- (int) depth {
    return mParser.depth;
}

- (JwaooBleToy *)initWithDelegate:(id<JwaooBleToyDelegate>)delegate {
    if (self = [super initWithName:@"JwaooToy" uuid:JWAOO_TOY_UUID_SERVICE]) {
        mDelegate = delegate;
        mSensor = [Mpu6050Sensor new];

        mParser = [[JwaooToyParser alloc] initWithValueFuzz:JWAOO_TOY_VALUE_FUZZ withTimeFuzz:JWAOO_TOY_TIME_FUZZ];
        [mParser setDepthSelector:@selector(onDepthChanged:) withTarget:self];
        [mParser setFreqSelector:@selector(onFreqChanged:) withTarget:self];
    }

    return self;
}

- (void)onFreqChanged:(NSNumber *)freq {
    if ([mDelegate respondsToSelector:@selector(didFreqChanged:)]) {
        [mDelegate didFreqChanged:freq.intValue];
    }
}

- (void)onDepthChanged:(NSNumber *)depth {
    if ([mDelegate respondsToSelector:@selector(didDepthChanged:)]) {
        [mDelegate didDepthChanged:depth.intValue];
    }
}

- (void)onEventReceived:(CavanBleChar *)bleChar {
    NSData *event = bleChar.data;
    NSUInteger length = event.length;

    if (length > 0) {
        const uint8_t *bytes = event.bytes;

        switch (bytes[0]) {
            case JWAOO_TOY_EVT_KEY_STATE:
                if (length < 3) {
                    break;
                }

                if ([mDelegate respondsToSelector:@selector(didKeyStateChanged:value:)]) {
                    [mDelegate didKeyStateChanged:bytes[1] value:bytes[2]];
                } else {
                    NSLog(@"KeyStateChanged: code = %d, value = %d", bytes[1], bytes[2]);
                }
                break;

            case JWAOO_TOY_EVT_KEY_CLICK:
                if (length < 3) {
                    break;
                }

                if ([mDelegate respondsToSelector:@selector(didKeyClicked:count:)]) {
                    [mDelegate didKeyClicked:bytes[1] count:bytes[2]];
                } else {
                    NSLog(@"KeyClicked: code = %d, count = %d", bytes[1], bytes[2]);
                }
                break;

            case JWAOO_TOY_EVT_KEY_LONG_CLICK:
                if (length < 2) {
                    break;
                }

                if ([mDelegate respondsToSelector:@selector(didKeyLongClicked:)]) {
                    [mDelegate didKeyLongClicked:bytes[1]];
                } else {
                    NSLog(@"KeyLongClicked: code = %d", bytes[1]);
                }
                break;

            default:
                NSLog(@"unknown event%d", bytes[0]);
        }
    }
}

- (void)onSensorDataReceived:(CavanBleChar *)bleChar {
    [mSensor putBytes:bleChar.bytes];
    [mParser putSensorData:mSensor];

    if ([mDelegate respondsToSelector:@selector(didSensorDataReceived:)]) {
        [mDelegate didSensorDataReceived:bleChar];
    }
}

- (void)onConnectStateChanged:(BOOL)connected {
    if ([mDelegate respondsToSelector:@selector(didConnectStateChanged:)]) {
        [mDelegate didConnectStateChanged:connected];
    } else {
        [self onConnectStateChanged:connected];
    }
}

- (BOOL)doInitialize {
    if (![mService.UUID isEqualTo:JWAOO_TOY_UUID_SERVICE]) {
        NSLog(@"Invalid service uuid: %@", mService.UUID);
        return false;
    }

    mCharCommand = mCharEvent = mCharFlash = mCharSensor = nil;

    for (CBCharacteristic *characteristic in mService.characteristics) {
        if ([characteristic.UUID isEqual:JWAOO_TOY_UUID_COMMAND]) {
            mCharCommand = [self createBleChar:characteristic];
            NSLog(@"mCharCommand = %@", characteristic.UUID);
        } else if ([characteristic.UUID isEqual:JWAOO_TOY_UUID_EVENT]) {
            mCharEvent = [self createBleChar:characteristic];
            [mCharEvent enableNotifyWithSelector:@selector(onEventReceived:) withTarget:self];
            NSLog(@"mCharEvent = %@", characteristic.UUID);
        } else if ([characteristic.UUID isEqual:JWAOO_TOY_UUID_FLASH]) {
            mCharFlash = [self createBleChar:characteristic];
            NSLog(@"mCharFlash = %@", characteristic.UUID);
        } else if ([characteristic.UUID isEqual:JWAOO_TOY_UUID_SENSOR]) {
            mCharSensor = [self createBleChar:characteristic];
            [mCharSensor enableNotifyWithSelector:@selector(onSensorDataReceived:) withTarget:self];
            NSLog(@"mCharSensor = %@", characteristic.UUID);
        } else {
            NSLog(@"Unknown characteristic = %@", characteristic.UUID);
            return false;
        }
    }

    if (mCharCommand == nil) {
        NSLog(@"Command characteristic not found: uuid = %@", JWAOO_TOY_UUID_COMMAND);
        return false;
    }

    if (mCharEvent == nil) {
        NSLog(@"Event characteristic not found: uuid = %@", JWAOO_TOY_UUID_EVENT);
        return false;
    }

    if (mCharFlash == nil) {
        NSLog(@"Flash characteristic not found: uuid = %@", JWAOO_TOY_UUID_FLASH);
        return false;
    }

    if (mCharSensor == nil) {
        NSLog(@"Sensor characteristic not found: uuid = %@", JWAOO_TOY_UUID_SENSOR);
        return false;
    }

    mCommand = [[JwaooToyCommand alloc] initWithBleChar:mCharCommand];

    NSString *identify = [self doIdentify];
    if (identify == nil) {
        NSLog(@"Failed to doIdentify");
        return false;
    }

    NSLog(@"identify = %@", identify);

    if (![identify isEqualToString:JWAOO_TOY_IDENTIFY]) {
        NSLog(@"Invalid identify");
        return false;
    }

    return [mDelegate doInitialize:self];
}

// ================================================================================

- (NSString *)doIdentify {
    return [mCommand readTextWithType:JWAOO_TOY_CMD_IDENTIFY];
}

- (NSString *)readBuildDate {
    return [mCommand readTextWithType:JWAOO_TOY_CMD_BUILD_DATE];
}

- (uint32_t)readVersion {
    return [mCommand readValueWithType32:JWAOO_TOY_CMD_VERSION];
}

- (BOOL)doReboot {
    return [mCommand readBoolWithType:JWAOO_TOY_CMD_REBOOT];
}

- (BOOL)setSensorEnable:(BOOL)enable {
    return [mCommand readBoolWithType:JWAOO_TOY_CMD_SENSOR_ENABLE withBool:enable];
}

- (BOOL)setSensorEnable:(BOOL)enable
              withDelay:(uint32_t)delay {
    return [mCommand readBoolWithType:JWAOO_TOY_CMD_SENSOR_ENABLE withBool:enable withDelay32:delay];
}

- (uint32_t)getFlashId {
    return [mCommand readValueWithType32:JWAOO_TOY_CMD_FLASH_ID];
}

- (uint32_t)getFlashSize {
    return [mCommand readValueWithType32:JWAOO_TOY_CMD_FLASH_SIZE];
}

- (uint32_t)getFlashPageSize {
    return [mCommand readValueWithType32:JWAOO_TOY_CMD_FLASH_PAGE_SIZE];
}

- (BOOL)setFlashWriteEnable:(BOOL)enable {
    return [mCommand readBoolWithType:JWAOO_TOY_CMD_FLASH_WRITE_ENABLE withBool:enable];
}

- (BOOL)eraseFlash {
    return [mCommand readBoolWithType:JWAOO_TOY_CMD_FLASH_ERASE];
}

- (BOOL)startFlashUpgrade {
    return [mCommand readBoolWithType:JWAOO_TOY_CMD_FLASH_WRITE_START];
}

- (BOOL)finishFlashUpgrade:(uint16_t)length {
    uint8_t command[] = { JWAOO_TOY_CMD_FLASH_WRITE_FINISH, mFlashCrc, length & 0xFF, length >> 8 };
    return [mCommand readBoolWithBytes:command length:sizeof(command)];
}

- (BOOL)writeFlash:(const void *)data
              size:(int)size
      withProgress:(CavanProgressManager *)progress {
    if (mCharFlash == nil) {
        return false;
    }

    if ([mCharFlash writeData:data length:size withProgress:progress]) {
        const uint8_t *p, *p_end;

        for (p = data, p_end = p + size; p < p_end; p++) {
            mFlashCrc ^= *p;
        }

        return true;
    }

    return false;
}

- (BOOL)writeFlashHeader:(uint16_t)length {
    length = (length + 7) & (~7);

    uint8_t header[8] = { 0x70, 0x50, 0x00, 0x00, 0x00, 0x00, length >> 8, length & 0xFF };

    return [self writeFlash:header size:sizeof(header) withProgress:nil];
}

- (BOOL)upgradeFirmwareSafe:(nonnull const char *)pathname
               withProgress:(nullable CavanProgressManager *)progress {
    uint32_t flashId = [self getFlashId];
    uint32_t flashSize = [self getFlashSize];
    uint32_t flashPageSize = [self getFlashPageSize];

    [progress setProgressRange:99];

    NSLog(@"ID = 0x%08x, size = %d, page_size = %d", flashId, flashSize, flashPageSize);

    CavanHexFile *file = [[CavanHexFile alloc] initWithPath:pathname mode:nil];
    if (file == nil) {
        return FALSE;
    }

    [progress addProgress];

    NSLog(@"readBinData");

    NSData *data = [file readBinData];
    if (data == nil) {
        NSLog(@"Failed to readBinData");
        return FALSE;
    }

    [progress addProgress];

    NSLog(@"length = %ld = 0x%08lx", data.length, data.length);
    NSLog(@"setFlashWriteEnable true");

    if (![self setFlashWriteEnable:true]) {
        NSLog(@"Failed to setWriteEnable true");
        return false;
    }

    [progress addProgress];

    NSLog(@"startFlashUpgrade");

    if (![self startFlashUpgrade]) {
        NSLog(@"Failed to startUpgrade");
        return false;
    }

    [progress addProgress];

    NSLog(@"doFlashErase");

    if (![self eraseFlash]) {
        NSLog(@"Failed to doErase");
        return false;
    }

    [progress addProgress];

    mFlashCrc = 0xFF;

    NSLog(@"writeFlashHeader");

    if (![self writeFlashHeader:data.length]) {
        NSLog(@"Failed to write flash header");
        return false;
    }

    [progress addProgress];

    NSLog(@"writeFlash data");

    if (![self writeFlash:data.bytes size:(int)data.length withProgress:progress]) {
        NSLog(@"Failed to write flash data");
        return false;
    }

    NSLog(@"finishFlashUpgrade");

    if (![self finishFlashUpgrade:(data.length + 8)]) {
        NSLog(@"Failed to finishUpgrade");
        return false;
    }

    NSLog(@"setFlashWriteEnable false");

    [self setFlashWriteEnable:false];

    [progress setProgressMax:100];
    [progress finish];

    return true;
}

- (BOOL)upgradeFirmware:(nonnull const char *)pathname
           withProgress:(nullable CavanProgressManager *)progress {
    if (mUpgradeBusy) {
        NSLog(@"upgrade busy");
        return false;
    }

    mUpgradeBusy = true;
    BOOL result = [self upgradeFirmwareSafe:pathname withProgress:progress];
    mUpgradeBusy = false;

    return result;
}

- (NSData *)readBdAddress {
    NSData *data = [mCommand readDataWithType:JWAOO_TOY_CMD_FLASH_READ_BD_ADDR];
    if (data == nil || data.length != 6) {
        return nil;
    }

    return data;
}

- (BOOL)writeBdAddress:(const uint8_t *)bd_addr {
    if (![self setFlashWriteEnable:true]) {
        return false;
    }

    if (![mCommand readBoolWithType:JWAOO_TOY_CMD_FLASH_WRITE_BD_ADDR withBytes:bd_addr length:6]) {
        return false;
    }

    return [self setFlashWriteEnable:false];
}

- (BOOL)setKeyClickEnable:(BOOL)enable {
    return [mCommand readBoolWithType:JWAOO_TOY_CMD_KEY_CLICK_ENABLE withBool:enable];
}

- (BOOL)setKeyLongClickEnable:(BOOL)enable {
    return [mCommand readBoolWithType:JWAOO_TOY_CMD_KEY_LONG_CLICK_ENABLE withBool:enable];
}

- (BOOL)setKeyLongClickEnable:(BOOL)enable
                 withDelay:(uint16_t)delay {
    return [mCommand readBoolWithType:JWAOO_TOY_CMD_KEY_LONG_CLICK_ENABLE withBool:enable withDelay16:delay];
}

- (BOOL)setKeyMultiClickEnable:(BOOL)enable {
    return [mCommand readBoolWithType:JWAOO_TOY_CMD_KEY_MULTI_CLICK_ENABLE withBool:enable];
}

- (BOOL)setKeyMultiClickEnable:(BOOL)enable
                  withDelay:(uint16_t)delay {
    return [mCommand readBoolWithType:JWAOO_TOY_CMD_KEY_MULTI_CLICK_ENABLE withBool:enable withDelay16:delay];
}

@end
