//
//  Protocol.swift
//  LEDPurse
//
//  Created by Kevin Lovette on 7/18/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit
import CoreBluetooth



class Protocol: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {

    var centralManager: CBCentralManager?
    var centralQueue: dispatch_queue_t = dispatch_queue_create("com.ioio.centralQueue", DISPATCH_QUEUE_SERIAL)

    var autoReconnect = true
    var reconnectHandler: ((CBPeripheral) -> Void)?
    
    init(reconnectHandler: ((CBPeripheral) -> Void)? = nil) {
        super.init()
        self.reconnectHandler = reconnectHandler
        self.centralManager = CBCentralManager(delegate: self, queue: self.centralQueue)

        //self.transport = Transport.sharedInstance
        //self.transport.connect(self.dongle, characteristic: self.dongleCharacteristic)
    }

    var dongle: CBPeripheral?
    var dongleService: CBService?
    var dongleCharacteristic: CBCharacteristic?

    var scanHandler: ((CBPeripheral, [NSObject : AnyObject]) -> Void)?
    var connectHandler: ((CBPeripheral) -> Void)?


    func isConnected() -> Bool {
        if ( (self.dongle != nil) && (self.dongleService != nil) && (self.dongleCharacteristic != nil) ) {
            return true
        }
        return false
    }
    
    func reconnect() {
        let defaults = NSUserDefaults.standardUserDefaults()

        // remove for testing
//        defaults.removeObjectForKey("pixelUUID")

        // get peripheral ID for reconnect        
        if let uuid = defaults.stringForKey("pixelUUID") {
            if let nsuuid = NSUUID(UUIDString: uuid) {
                let ids = [nsuuid] //NSUUID]()

                NSLog("TRY reconnect to \(uuid)")
                let peripherals = self.centralManager!.retrievePeripheralsWithIdentifiers(ids)

                for obj in peripherals {
                    if let peripheral = obj as? CBPeripheral {
                        NSLog("  \(peripheral)")
                        // connect
                        self.connect(peripheral, connectHandler: didReconnect)
                    }
                }
            }
        }
    }

    func didReconnect(peripheral: CBPeripheral) {
        NSLog("  CONNECTED TO \(peripheral)")
        if let handler = self.reconnectHandler {
            handler(peripheral)
        }
    }

    
    func startScan(scanHandler: ((CBPeripheral, [NSObject : AnyObject]) -> Void)? = nil) {
        if let manager = self.centralManager {
            self.scanHandler = scanHandler
            
            let serviceUUIDs = [CBUUID(string: "1130FBD0-6D61-422A-8939-042DD56B1EF5")]
//            let options = [CBCentralManagerScanOptionSolicitedServiceUUIDsKey: CBUUID(string: "1130FBD0-6D61-422A-8939-042DD56B1EF5")]

            self.centralManager!.scanForPeripheralsWithServices(serviceUUIDs, options: nil)
        }
    }
    func stopScan() {
        // stop scanning
        self.centralManager!.stopScan()
    }
    func connect(peripheral: CBPeripheral, connectHandler: ((CBPeripheral) -> Void)? = nil) {
        self.connectHandler = connectHandler

//        if let name = peripheral.name {
//            // Check the name 
//            if name == "PIXEL" {
        if dongle == nil {
            self.dongle = peripheral
            self.dongle!.delegate = self
            NSLog("didDiscoverPeripheral \(peripheral)")

            // save peripheral ID for reconnect
            let id = peripheral.identifier.UUIDString
            let defaults = NSUserDefaults.standardUserDefaults()
            defaults.setObject(id, forKey: "pixelUUID")
                    
            self.centralManager!.connectPeripheral(peripheral, options: nil)
        }
//              }
//          }
    }

    
    
    var transport: Transport = WriteWithResponseTransport.sharedInstance
    //var transport: Transport = WriteWithoutResponseTransport.sharedInstance
    
    func connectCompleteHandler(peripheral: CBPeripheral) {
        self.transport.connect(self.dongle, characteristic: self.dongleCharacteristic, completionHandler: self.connectHandler)
    }        


    func handleDidUpdateNotificationState(characteristic: CBCharacteristic, error: NSError?) {
        transport.handleUpdateNotificationState(characteristic, error: error)
    }
    
    func handleDidUpdateValue(characteristic: CBCharacteristic, error: NSError?) {
        let value = characteristic.value
        //NSLog("### handleNotification  data: \(data)")
        transport.handleNotification(value)
    }

    func handleDidWriteValue(characteristic: CBCharacteristic?, error: NSError?) {
        //NSLog("### handleNotification  data: \(data)")
        transport.handleWriteAck()
    }

    
    //--------------------------------------------------------------------------------
    //
    // MARK: - CBCentralManagerDelegate
    //
    
   func centralManagerDidUpdateState(central: CBCentralManager!) {

        switch ( central.state ) {
        case CBCentralManagerState.Unsupported:
            NSLog("centralManagerDidUpdateState  Unsupported    The platform/hardware doesn't support Bluetooth Low Energy.")
        case CBCentralManagerState.Unauthorized:
            NSLog("centralManagerDidUpdateState  Unauthorized   The app is not authorized to use Bluetooth Low Energy.")
        case CBCentralManagerState.PoweredOff:
            NSLog("centralManagerDidUpdateState  PoweredOff     Bluetooth is currently powered off.")
        case CBCentralManagerState.PoweredOn:
            NSLog("centralManagerDidUpdateState  PoweredOn      Powered On")
            if self.autoReconnect {
                self.reconnect()
            }
        case CBCentralManagerState.Unknown:
            NSLog("centralManagerDidUpdateState  Unknown        Unknown")
        default:
            break
        }
//        NSLog("centralManagerDidUpdateState  \(central)")
    }

    //
    // Handle discovery of a device found by the scan
    //
    func centralManager(central: CBCentralManager!,
                        didDiscoverPeripheral peripheral: CBPeripheral!,
                        advertisementData: [NSObject : AnyObject]!,
                        RSSI: NSNumber!) {
        let id = peripheral.identifier.UUIDString
        
        // look for PIXEL peripheral advertising service ID 0000FF10-0000-1000-8000-00805F9B34FB

        // notify UI
        if let handler = self.scanHandler {
            handler(peripheral, advertisementData)
        }
    }

    func centralManager(central: CBCentralManager!, didConnectPeripheral peripheral: CBPeripheral!) {
        NSLog("didConnectPeripheral \(peripheral)")

        self.dongle!.discoverServices(nil)
    }

    // MARK: - CBPeripheralDelegate
    
    func peripheral(peripheral: CBPeripheral!, didDiscoverServices error: NSError!) {
        NSLog("didDiscoverServices \(peripheral)")
        NSLog("    services = \(peripheral.services)")
        self.dongleService = peripheral.services[0] as? CBService

        self.dongle!.discoverCharacteristics(nil, forService: self.dongleService!)
    }

    func peripheral(peripheral: CBPeripheral!,
                    didDiscoverCharacteristicsForService service: CBService!,
                    error: NSError!) {
        NSLog("didDiscoverCharacteristics \(peripheral)")
        NSLog("    characteristics = \(self.dongleService!.characteristics)")

        self.dongleCharacteristic = service.characteristics[0] as? CBCharacteristic

        // stop scanning
        self.centralManager!.stopScan()
                    
        // put the state machine into a CONNECTED state
        self.connectCompleteHandler(peripheral)
    }

    func peripheral(peripheral: CBPeripheral!,
                    didUpdateNotificationStateForCharacteristic characteristic: CBCharacteristic!,
                    error: NSError!) {
        NSLog("didUpdateNotificationStateForCharacteristics \(characteristic)")
        self.handleDidUpdateNotificationState(characteristic, error: error)
    }

    func peripheral(peripheral: CBPeripheral!,
                    didUpdateValueForCharacteristic characteristic: CBCharacteristic!,
                    error: NSError!) {
        //NSLog("didUpdateValueForCharacteristics \(characteristic)")
        self.handleDidUpdateValue(characteristic, error: error)
    }

    func peripheral(peripheral: CBPeripheral!,
                    didWriteValueForCharacteristic characteristic: CBCharacteristic!,
                    error: NSError!) {
        //NSLog("didWriteValueForCharacteristics \(characteristic)  error: \(error)")
        self.handleDidWriteValue(characteristic, error: error)
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
        sendCommand(command, completionHandler: completionHandler)
    }


    func matrixWriteFile(fps: Float, shifterLen32: UInt8, rows: UInt8, completionHandler: (() -> Void)? = nil) {
        let command = CommandWriteFile(fps: fps, shifterLen32: shifterLen32, rows: rows)
        // send command
        sendCommand(command, completionHandler: completionHandler)
    }

    func matrixFrame(frame: NSData, completionHandler: (() -> Void)? = nil) {
        let command = CommandFrame(frame: frame)
        // send command
        sendCommand(command, completionHandler: completionHandler)
    }


    let commandQueue: dispatch_queue_t = dispatch_queue_create("com.ioio.commandQueue", DISPATCH_QUEUE_SERIAL)

/*
    func matrixUpload(data: NSData, completionHandler: (() -> Void)? = nil) {
        let numFrames = ( data.length / 768 )
            
        let thread = NSThread.currentThread()
        NSLog("matrixUpload - Thread: \(thread.description)  isMain: \(NSThread.isMainThread())  main: \(thread.isMainThread)")

        dispatch_async(self.commandQueue) {
            let thread = NSThread.currentThread()
            NSLog("Thread: \(thread.description)  isMain: \(NSThread.isMainThread())  main: \(thread.isMainThread)")

            self.matrixWriteFile(10.0, shifterLen32: 0x01, rows: 8)
            for index in 0 ..< numFrames {
                let data = data.subdataWithRange(NSMakeRange( (index * 768), 768 ))            
                NSLog("write frame \(index)")
                self.matrixFrame(data)
            }
            self.matrixPlay(0)

            if let handler = self.connectHandler {
                handler()
            }
        }

        let thread2 = NSThread.currentThread()
        NSLog("DONE - matrixUpload - Thread: \(thread2.description)  isMain: \(NSThread.isMainThread())  main: \(thread2.isMainThread)")
    }
*/


    //--------------------------------------------------------------------------------
    //
    // Command Queue
    //


    // release queue when you are done with all the work
    //dispatch_release(backgroundQueue)
    

    func sendCommand(command: Command, completionHandler: (() -> Void)? = nil) {
        if let data = command.make() {
            transport.sendCommand(data)
        }

        if let handler = completionHandler {
            handler()
        }
    }

}


//
// Command protocol
//
protocol Command {
    func make() -> NSData?
}


class CommandEnable : Command {
    var shifterLen32: UInt8 = 0
    var rows: UInt8 = 0
    
    init(shifterLen32: UInt8, rows: UInt8) {
        self.shifterLen32 = shifterLen32
        self.rows = rows
    }
    
    func make() -> NSData? {
        var flagsL: UInt8 = ( (shifterLen32 & 0x0F) )
        var flagsH: UInt8 = ( (rows == 8 ? 0 : 1) << 4)
        
        var flags: UInt8 = ( (shifterLen32 & 0x0F) | ((rows == 8 ? 0 : 1) << 4) )
        let packet: [UInt8] = [0x1E, flags]
        
        let flagsHex = String(flags, radix: 16, uppercase: false)
        let flagsHexL = String(flags, radix: 16, uppercase: false)
        let flagsHexH = String(flags, radix: 16, uppercase: false)
        NSLog("matrixEnable \(shifterLen32)  \(rows)  \(flagsHex)  \(flagsHexH)  \(flagsHexL)")
        
        let data = NSData(bytes: packet, length: packet.count)
        return data
    }
}


class CommandFrame : Command {
    var frame: NSData

    init(frame: NSData) {
        self.frame = frame
    }

    func make() -> NSData? {
        var bytes = [UInt8](count: 769, repeatedValue: 0)
        
        let framePtr = UnsafeMutablePointer<UInt8>(frame.bytes)
        var frameBytes = UnsafeMutableBufferPointer<UInt8>(start: framePtr, count: frame.length)

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

    func make() -> NSData? {
        var flags: UInt8 = ( (shifterLen32 & 0x0F) | ((rows == 8 ? 0 : 1) << 4) )

        var delay: UInt16 = UInt16( round(62500.0 / fps ) - 1 )
        var delay0: UInt8 = UInt8( delay & 0xff )
        var delay1: UInt8 = UInt8( ( delay >> 8 ) & 0xff )

        NSLog("matrixWriteFile  \(fps)  \(shifterLen32)  \(rows)")

        let packet: [UInt8] = [0x20, delay0, delay1, flags]

        let data = NSData(bytes: packet, length: packet.count)
        return data
    }
}

