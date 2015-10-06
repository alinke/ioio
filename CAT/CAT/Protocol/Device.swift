//
//  Device.swift
//  CAT
//
//  Created by Kevin Lovette on 7/18/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit
import CoreBluetooth


class Device: NSObject, CBPeripheralDelegate {

    var peripheral: CBPeripheral?
    var service: CBService?
    var characteristic: CBCharacteristic?
    var controlCharacteristic: CBCharacteristic?

    var name: String {
        get {
            return self.peripheral!.name!
        }
    }
    var identifier: String {
        get {
            return self.peripheral!.identifier.UUIDString
        }
    }


    // MARK: - Init

    init(peripheral: CBPeripheral) {
        super.init()
        self.peripheral = peripheral
        self.peripheral!.delegate = self
    }
    
    // MARK: - CBPeripheralDelegate
    func peripheralDidUpdateName(peripheral: CBPeripheral) {
        // Update UI
    }


    //--------------------------------------------------------------------------------
    //
    // Scan
    //

    // MARK: - Scan
    var advertisementData: [NSObject: AnyObject]?

    // Called when the central is scanning and discovers a device.
    func didDiscover(advertisementData: [NSObject: AnyObject]) {
        self.advertisementData = advertisementData
        Log.info("Device \(self.name)   advData \(advertisementData)")
    }


    //--------------------------------------------------------------------------------
    //
    // Connect
    //

    // MARK: - Connect

    var connected = false;
    typealias ConnectHandler = (device: Device) -> Void
    typealias DisconnectHandler = (device: Device) -> Void
    
    func isConnected() -> Bool {
        return self.connected
    }

    var connectHandler: ConnectHandler?
    
    func connect(connectHandler: ConnectHandler? = nil) {
        self.connectHandler = connectHandler
    }

    // Called when the device connects
    func didConnect() {
        // Start device setup. Discover services and characteristics, enable notifications.
        self.startDeviceSetup()
    }

    // Called when the device fails to connect
    func didFailToConnect(error: NSError?) {
        Log.info("didFailToConnect \(self.name)  error: \(error)")
    }

    
    //--------------------------------------------------------------------------------
    //
    // Disconnect
    //

    // MARK: - Disconnect
    
    var disconnectHandler: DisconnectHandler?
    
    func disconnect(disconnectHandler: DisconnectHandler? = nil) {
        self.disconnectHandler = disconnectHandler
    }
    
    // Called when the device disconnects
    func didDisconnect(error: NSError?) {
        Log.info("didDisconnect \(self.name)  error: \(error)")
        self.stopTransport()
        self.connected = false
        
        if error != nil {
            Log.info("  error: \(error)")
            if let handler = self.disconnectHandler {
                handler(device: self)
            }
        } else {
            // disconnected in response to cancelPeripheralConnection
            if let handler = self.disconnectHandler {
                handler(device: self)
            }
        }
    }


    //--------------------------------------------------------------------------------
    //
    // Setup
    //

    // MARK: - Setup

    func startDeviceSetup() {
        // Log.info("startDeviceSetup")
        self.peripheral!.discoverServices(nil)
    }

    func didSetupDevice(error: NSError?) {
        Log.info("Device - didSetupDevice \(self.name)  error: \(error)")

        self.connected = true
        self.startTransport()

        // call the connection complete handler
        if let handler = self.connectHandler {
            handler(device: self)
        }
    }

    // MARK: - CBPeripheralDelegate
    func peripheral(peripheral: CBPeripheral, didDiscoverServices error: NSError?) {
        // Log.info("didDiscoverServices \(peripheral.name)  error: \(error)")
        // Log.info("    services = \(peripheral.services)")

        if let services = peripheral.services {
            self.service = services[0]
            self.peripheral!.discoverCharacteristics(nil, forService: self.service!)
        } else {
            Log.info("nil services")
        }
    }

    func peripheral(peripheral: CBPeripheral,
                    didDiscoverCharacteristicsForService service: CBService,
                    error: NSError?) {
        // Log.info("didDiscoverCharacteristicsForService \(peripheral.name)  \(service)  error: \(error)")
        // Log.info("    characteristics = \(self.service!.characteristics)")

        if let characteristics = service.characteristics {
            for characteristic in characteristics {
                let uuid = characteristic.UUID.UUIDString
                Log.info("    characteristic: \(uuid)")

                if uuid == "1130FBD1-6D61-422A-8939-042DD56B1EF5" {
                    self.characteristic = characteristic
                }
                if uuid == "1130FBD2-6D61-422A-8939-042DD56B1EF5" {
                    self.controlCharacteristic = characteristic
                }
            }
        }

        // enable notifications
        self.peripheral!.setNotifyValue(true, forCharacteristic: self.characteristic!)
    }

    func peripheral(peripheral: CBPeripheral,
                    didUpdateNotificationStateForCharacteristic characteristic: CBCharacteristic,
                    error: NSError?) {
        // Log.info("didUpdateNotificationStateForCharacteristics \(peripheral.name)  \(characteristic)  error: \(error)")

        // put the state machine into a CONNECTED state
        self.didSetupDevice(error)
    }


    //--------------------------------------------------------------------------------
    //
    // Transport
    //

    // MARK: - Transport

    var transport: Transport = WriteWithResponseTransport.sharedInstance

    func startTransport() {
        transport.connect(self)
    }
    
    func stopTransport() {
        transport.disconnect()
    }

    func writePacket(packet: Packet) {
        if let peripheral = self.peripheral {
            if let characteristic = self.characteristic {
                // Log.info("  writePacket: \(packet.data)")
                if let data = packet.data {
                    peripheral.writeValue(data, forCharacteristic: characteristic, type: CBCharacteristicWriteType.WithResponse)
                }
            }
        }
    }

    var writeControlCompletionHandler: (() -> Void)?

    func writeControlPacket(data: NSData, completionHandler: (() -> Void)? = nil) {
        if let peripheral = self.peripheral {
            if let characteristic = self.controlCharacteristic {
                Log.info("  writeControlPacket: \(data)")
                self.writeControlCompletionHandler = completionHandler
                peripheral.writeValue(data, forCharacteristic: characteristic, type: CBCharacteristicWriteType.WithResponse)
            }
        }
    }

    // MARK: - CBPeripheralDelegate
    func peripheral(peripheral: CBPeripheral,
                    didUpdateValueForCharacteristic characteristic: CBCharacteristic,
                    error: NSError?) {
//        let uuid = characteristic.UUID.UUIDString
//        Log.info("didUpdateValueForCharacteristic: \(uuid)  error: \(error)")

        if error != nil {
            Log.info("handleDidUpdateValue error: \(error)")
        }

        if let value = characteristic.value {
            transport.handleNotification(value)
        }
    }

    func peripheral(peripheral: CBPeripheral,
                    didWriteValueForCharacteristic characteristic: CBCharacteristic,
                    error: NSError?) {
        let uuid = characteristic.UUID.UUIDString
//        Log.info("didWriteValueForCharacteristic: \(uuid)  error: \(error)")

        if error != nil {
            Log.info("handleDidWriteValue error: \(error)")
        }

        if uuid == "1130FBD1-6D61-422A-8939-042DD56B1EF5" {
            transport.handleWriteAck()
        }
        if uuid == "1130FBD2-6D61-422A-8939-042DD56B1EF5" {
            Log.info("Control characteristic did write")
            if let handler = self.writeControlCompletionHandler {
                handler();
                self.writeControlCompletionHandler = nil
            }
        }
    }


    //--------------------------------------------------------------------------------
    //
    // Protocol interface
    //

    var api: Protocol?

    func registerProtocol(api: Protocol) {
        self.api = api
    }
    
    // Send Command
    func sendCommand(command: Command, completionHandler: (() -> Void)? = nil) {
        if let data = command.make() {
            transport.sendCommand(data)
        }

        if let handler = completionHandler {
            // Log.info("SendCommand call completionHandler")
            handler()
        }
    }

    func sendAck(completionHandler: (() -> Void)? = nil) {
        // send ACK
        let ack: [UInt8] = [0x01]
        let packet = NSData(bytes: ack, length: ack.count)
        self.writeControlPacket(packet)

        Log.info("DONE writing control ACK")

        if let handler = completionHandler {
            Log.info("sendAck call completionHandler")
            handler()
        }
/*
        self.writeControlPacket(packet) {
            () -> Void in
            Log.info("DONE writing control ACK")

            if let handler = completionHandler {
                Log.info("sendAck call completionHandler")
                handler()
            }
        }
*/
    }

    // function to handle an incoming command message
    func handleCommandMessage(data: NSData) {
        // construct a command
        if let api = self.api {
            api.handleCommandMessage(data)
        }
    }

}


