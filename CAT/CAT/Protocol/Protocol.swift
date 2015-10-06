//
//  Protocol.swift
//  CAT
//
//  Created by Kevin Lovette on 7/18/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit
import CoreBluetooth



class Protocol {

    var device: Device?

    init(device: Device) {
        self.device = device
        // register with the device
         self.device!.registerProtocol(self)
    }

    
    //--------------------------------------------------------------------------------
    //
    // API
    //
    // Each of these calls should be atomic
    //

    func matrixPlay(rows: UInt8, completionHandler: (() -> Void)? = nil) {
        matrixEnable(0x01, rows: rows, completionHandler: completionHandler)
    }
    
    func matrixInteractive(rows: UInt8, completionHandler: (() -> Void)? = nil) {
        matrixEnable(0x00, rows: rows, completionHandler: completionHandler)
    }

    func matrixEnable(shifterLen32: UInt8, rows: UInt8, completionHandler: (() -> Void)? = nil) {
        let command = CommandEnable(shifterLen32: shifterLen32, rows: rows)
        // send command
        self.device!.sendCommand(command, completionHandler: completionHandler)
    }


    func matrixWriteFile(fps: Float, shifterLen32: UInt8, rows: UInt8, completionHandler: (() -> Void)? = nil) {
        let command = CommandWriteFile(fps: fps, shifterLen32: shifterLen32, rows: rows)
        // send command
        self.device!.sendCommand(command, completionHandler: completionHandler)
    }

    func matrixFrame(frame: NSData, completionHandler: (() -> Void)? = nil) {
        let command = CommandFrame(frame: frame)
        // send command
        self.device!.sendCommand(command, completionHandler: completionHandler)
    }


    //
    // Device Control
    //
    func getDeviceInfo(version: Int, completionHandler: (() -> Void)? = nil) {
        let command = CommandGetDeviceInfo(version: UInt16(version))
        // send command
        self.device!.sendCommand(command, completionHandler: completionHandler)
    }

    func setBrightnessLevel(level: Int, completionHandler: (() -> Void)? = nil) {
        let command = CommandSetBrightnessLevel(level: UInt8(level))
        // send command
        self.device!.sendCommand(command, completionHandler: completionHandler)
    }


    //
    // function to handle an incoming command message
    //
    func handleCommandMessage(data: NSData) {
        // parse the message to create a command
        if let command = Command.parseMessage(data) {
            command.dump()
            // handle command
            command.handle(self)
        }
    }

    func sendAck() {
        // send ACK
        self.device!.sendAck()
    }
}


//--------------------------------------------------------------------------------
//
// Command protocol
//

class Command {
    enum IncomingMessageType: UInt8 {
        case ESTABLISH_CONNECTION                = 0x00
        case REPORT_DIGITAL_IN_STATUS            = 0x04
        case REPORT_PERIODIC_DIGITAL_IN_STATUS   = 0x05
        case REPORT_ANALOG_IN_FORMAT             = 0x0C
        case REPORT_ANALOG_IN_STATUS             = 0x0B
        case UART_REPORT_TX_STATUS               = 0x0F
        case UART_DATA                           = 0x0E
        case SPI_REPORT_TX_STATUS                = 0x12
        case SPI_DATA                            = 0x11
        case I2C_RESULT                          = 0x14
        case I2C_REPORT_TX_STATUS                = 0x15
        case CHECK_INTERFACE_RESPONSE            = 0x02
        case UART_STATUS                         = 0x0D
        case SPI_STATUS                          = 0x10
        case I2C_STATUS                          = 0x13
        case ICSP_RESULT                         = 0x17
        case ICSP_REPORT_RX_STATUS               = 0x16
        case INCAP_STATUS                        = 0x1B
        case INCAP_REPORT                        = 0x1C
        case SOFT_CLOSE                          = 0x1D
            // API2
        case API2_OUT_GET_DEVICE_INFO            = 0x21
    }
    enum OutgoingMessageType: UInt8 {
        case HARD_RESET                          = 0x00
        case SOFT_RESET                          = 0x01
        case SET_PIN_DIGITAL_OUT                 = 0x03
        case SET_DIGITAL_OUT_LEVEL               = 0x04
        case SET_PIN_DIGITAL_IN                  = 0x05
        case SET_CHANGE_NOTIFY                   = 0x06
        case REGISTER_PERIODIC_DIGITAL_SAMPLING  = 0x07
        case SET_PIN_PWM                         = 0x08
        case SET_PWM_DUTY_CYCLE                  = 0x09
        case SET_PWM_PERIOD                      = 0x0A
        case SET_PIN_ANALOG_IN                   = 0x0B
        case UART_DATA                           = 0x0E
        case UART_CONFIG                         = 0x0D
        case SET_PIN_UART                        = 0x0F
        case SPI_MASTER_REQUEST                  = 0x11
        case SPI_CONFIGURE_MASTER                = 0x10
        case SET_PIN_SPI                         = 0x12
        case I2C_CONFIGURE_MASTER                = 0x13
        case I2C_WRITE_READ                      = 0x14
        case SET_ANALOG_IN_SAMPLING              = 0x0C
        case CHECK_INTERFACE                     = 0x02
        case ICSP_SIX                            = 0x16
        case ICSP_REGOUT                         = 0x17
        case ICSP_PROG_ENTER                     = 0x18
        case ICSP_PROG_EXIT                      = 0x19
        case ICSP_CONFIG                         = 0x1A
        case INCAP_CONFIG                        = 0x1B
        case SET_PIN_INCAP                       = 0x1C
        case SOFT_CLOSE                          = 0x1D
        case RGB_LED_MATRIX_ENABLE               = 0x1E
        case RGB_LED_MATRIX_FRAME                = 0x1F
        case RGB_LED_MATRIX_WRITE_FILE           = 0x20
            // API2
        case API2_IN_GET_DEVICE_INFO             = 0x21
        case API2_IN_SET_DEVICE_BRIGHTNESS       = 0x22
    }
  
    func make() -> NSData? {
        return nil
    }

    static func parseMessage(data: NSData) -> Command? {
        //IncomingMessageType(dataBytes[0])
        //let dataPtr = UnsafePointer<UInt8>(data.bytes)
        //var dataBytes = UnsafeBufferPointer<UInt8>(start: dataPtr, count: data.length)

        let stream = MessageStream(data: data)
        if let type = IncomingMessageType(rawValue: stream.readUInt8()) {
            switch type {
            //case ESTABLISH_CONNECTION:
            case IncomingMessageType.ESTABLISH_CONNECTION:
                return CommandEstablishConnection(stream: stream)
                       
            case IncomingMessageType.API2_OUT_GET_DEVICE_INFO:
                return CommandDeviceInfoResponse(stream: stream)
                       
            default:
                Log.info("Protocol makeCommand  data: \(data)")
                break
            }
        } else {
            Log.info("Protocol makeCommand  invalid type  data: \(data)")
        }

        return nil
    }

    func dump() {
    }

    func handle(api: Protocol) {
    }
}

class MessageStream {
    var data: NSData
    var pos = 0
    let ptr: UnsafePointer<UInt8>
    var bytes: UnsafeBufferPointer<UInt8>
    
    init(data: NSData) {
        self.data = data
        self.ptr = UnsafePointer<UInt8>(data.bytes)
        self.bytes = UnsafeBufferPointer<UInt8>(start: ptr, count: data.length)
    }

    func reset() {
        self.pos = 0
    }
    
    func hasData() -> Bool {
        return ( pos < data.length )
    }

    func skip(count: Int) -> Bool {
        if ( pos + count ) < data.length {
            pos += count
            return true
        } else {
            pos = data.length
            return false
        }
    }

    func readUInt8() -> UInt8 {
        let value = bytes[pos]
        pos++
        return value
    }

    func readUInt16() -> UInt16 {
        var value: UInt16
        value = ( ( UInt16(bytes[pos+1]) << 8) | UInt16(bytes[pos]) )
        pos += 2
        return value
    }

    func readUInt32() -> UInt32 {
        var value: UInt32
        value = ( ( UInt32(bytes[pos+3]) << 24) |
                  ( UInt32(bytes[pos+2]) << 16) |
                  ( UInt32(bytes[pos+1]) << 8) |
                  UInt32(bytes[pos]) )
        pos += 4
        return value
    }

    func readBytes(ptr: UnsafeMutablePointer<UInt8>, length: Int) {
        for index in 0 ..< length {
            ptr[index] = bytes[pos]
            pos++
        }
    }

    func readString(length: Int) -> String? {
        if let buffer = NSMutableData(length: length) {
            let ptr = UnsafeMutablePointer<UInt8>(buffer.mutableBytes)
            self.readBytes(ptr, length: length)
            if let str = NSString(bytes: ptr, length: length, encoding: NSASCIIStringEncoding) {
                return String(str)
            }
        }
        return nil
    }
}


//--------------------------------------------------------------------------------
//
// Incoming Commands
//

class CommandEstablishConnection : Command {
    static var sendCount = 0

    // 00 494f49 4f - 504958 4c303032 35 - 494f49 4f303430 31 - 504958 4c303031 30
    var magic: String?               // 4 bytes
    var hardwareVersion: String?     // 8 bytes
    var bootloaderVersion: String?   // 8 bytes
    var firmwareVersion: String?     // 8 bytes
    
    init(stream: MessageStream) {
        self.magic = stream.readString(4)
        self.hardwareVersion = stream.readString(8)
        self.bootloaderVersion = stream.readString(8)
        self.firmwareVersion = stream.readString(8)
    }
    
    override func dump() {
        Log.info("Establish Connecton  magic: \(self.magic)   hw: \(self.hardwareVersion)  boot: \(self.bootloaderVersion)  fw: \(self.firmwareVersion)")
    }

    override func handle(api: Protocol) {
//        api.sendAck()
//        CommandEstablishConnection.sendCount++
//        if CommandEstablishConnection.sendCount > 5 {
//            api.sendAck()
//            CommandEstablishConnection.sendCount = 0
//        }

        dispatch_async(dispatch_get_main_queue()) {
            // Send dev info request
            let app = App.sharedInstance
            app.getDeviceInfo(1) {
                () -> Void in
                Log.info("Get Device Info request sent")
                dispatch_async(dispatch_get_main_queue()) {                
                    app.setBrightnessLevel(7) {
                        () -> Void in
                        Log.info("Brightness set")
                    }
                }
            }
        }
    }
}    

class CommandDeviceInfoResponse : Command {
    var brightness_level: UInt16 = 0
    var battery_level: UInt16 = 0

    init(stream: MessageStream) {
        self.brightness_level = stream.readUInt16()
        self.battery_level = stream.readUInt16()
    }
    
    override func dump() {
        Log.info("Device Info Response  brightness: \(self.brightness_level)  battery: \(self.battery_level)")
    }
}


//--------------------------------------------------------------------------------
//
// Outgoing Commands
//

class CommandEnable : Command {
    var shifterLen32: UInt8 = 0
    var rows: UInt8 = 0
    
    init(shifterLen32: UInt8, rows: UInt8) {
        self.shifterLen32 = shifterLen32
        self.rows = rows
    }
    
    override func make() -> NSData? {
        let flags: UInt8 = ( (shifterLen32 & 0x0F) | ((rows == 8 ? 0 : 1) << 4) )
        let packet: [UInt8] = [0x1E, flags]
        
        // Log.info("matrixEnable \(shifterLen32)  \(rows)  \(flagsHex)  \(flagsHexH)  \(flagsHexL)")
        
        let data = NSData(bytes: packet, length: packet.count)
        return data
    }
}


class CommandFrame : Command {
    var frame: NSData

    init(frame: NSData) {
        self.frame = frame
    }

    override func make() -> NSData? {
        var bytes = [UInt8](count: 769, repeatedValue: 0)
        
        let framePtr = UnsafeMutablePointer<UInt8>(frame.bytes)
        let frameBytes = UnsafeMutableBufferPointer<UInt8>(start: framePtr, count: frame.length)

        bytes[0] = 0x1F
        for index in 0 ..< 768 {
            bytes[index + 1] = frameBytes[index]
        }

        let ptr = UnsafePointer<UInt8>(bytes)
        let data = NSData(bytes: ptr, length: 769)
        return data
    }
}


class CommandWriteFile : Command {
    var fps: Float = 0
    var shifterLen32: UInt8 = 0
    var rows: UInt8 = 0

    init(fps: Float, shifterLen32: UInt8, rows: UInt8) {
        self.fps = fps
        self.shifterLen32 = shifterLen32
        self.rows = rows
    }

    override func make() -> NSData? {
        let flags: UInt8 = ( (shifterLen32 & 0x0F) | ((rows == 8 ? 0 : 1) << 4) )

        let delay: UInt16 = UInt16( round(62500.0 / fps ) - 1 )
        let delay0: UInt8 = UInt8( delay & 0xff )
        let delay1: UInt8 = UInt8( ( delay >> 8 ) & 0xff )

        // Log.info("matrixWriteFile  \(fps)  \(shifterLen32)  \(rows)")

        let packet: [UInt8] = [0x20, delay0, delay1, flags]

        let data = NSData(bytes: packet, length: packet.count)
        return data
    }
}


class CommandGetDeviceInfo : Command {
    var version: UInt16 = 0

    init(version: UInt16) {
        self.version = version
    }
    
    override func make() -> NSData? {
        let versionL: UInt8 = UInt8( self.version & 0x00ff )
        let versionH: UInt8 = UInt8( (self.version & 0xff00) >> 8 )

        //let packet: [UInt8] = [OutgoingMessageType.API2_IN_GET_DEVICE_INFO, versionL, versionH]
        let packet: [UInt8] = [0x21, versionL, versionH]
        let data = NSData(bytes: packet, length: packet.count)
        return data
    }
}


class CommandSetBrightnessLevel : Command {
    var level: UInt8 = 0

    init(level: UInt8) {
        self.level = level
    }
    
    override func make() -> NSData? {
        //let packet: [UInt8] = [OutgoingMessageType.API2_IN_SET_DEVICE_BRIGHTNESS, self.level]
        let packet: [UInt8] = [0x22, self.level]
        let data = NSData(bytes: packet, length: packet.count)
        return data
    }
}

