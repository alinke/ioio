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

    var name: String {
        get {
            return self.peripheral!.name
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
        NSLog("Device \(self.name)   advData \(advertisementData)")
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
    func didFailToConnect(error: NSError) {
        NSLog("didFailToConnect \(self.name)  error: \(error)")
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
        NSLog("didDisconnect \(self.name)  error: \(error)")
        self.stopTransport()
        self.connected = false
        
        if error != nil {
            NSLog("  error: \(error)")
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
        // NSLog("startDeviceSetup")
        self.peripheral!.discoverServices(nil)
    }

    func didSetupDevice(error: NSError?) {
        NSLog("Device - didSetupDevice \(self.name)  error: \(error)")

        self.connected = true
        self.startTransport()

        // call the connection complete handler
        if let handler = self.connectHandler {
            handler(device: self)
        }
    }

    // MARK: - CBPeripheralDelegate
    func peripheral(peripheral: CBPeripheral!, didDiscoverServices error: NSError!) {
        // NSLog("didDiscoverServices \(peripheral.name)  error: \(error)")
        // NSLog("    services = \(peripheral.services)")

        self.service = peripheral.services[0] as? CBService
        self.peripheral!.discoverCharacteristics(nil, forService: self.service!)
    }

    func peripheral(peripheral: CBPeripheral!,
                    didDiscoverCharacteristicsForService service: CBService!,
                    error: NSError!) {
        // NSLog("didDiscoverCharacteristicsForService \(peripheral.name)  \(service)  error: \(error)")
        // NSLog("    characteristics = \(self.service!.characteristics)")

        self.characteristic = service.characteristics[0] as? CBCharacteristic

        // enable notifications
        self.peripheral!.setNotifyValue(true, forCharacteristic: self.characteristic!)
    }

    func peripheral(peripheral: CBPeripheral!,
                    didUpdateNotificationStateForCharacteristic characteristic: CBCharacteristic!,
                    error: NSError!) {
        // NSLog("didUpdateNotificationStateForCharacteristics \(peripheral.name)  \(characteristic)  error: \(error)")

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
                // NSLog("  writePacket: \(packet.data)")
                peripheral.writeValue(packet.data, forCharacteristic: characteristic, type: CBCharacteristicWriteType.WithResponse)
            }
        }
    }

    // MARK: - CBPeripheralDelegate
    func peripheral(peripheral: CBPeripheral!,
                    didUpdateValueForCharacteristic characteristic: CBCharacteristic!,
                    error: NSError!) {
        if error != nil {
            NSLog("handleDidUpdateValue error: \(error)")
        }
        let value = characteristic.value
        transport.handleNotification(value)
    }

    func peripheral(peripheral: CBPeripheral!,
                    didWriteValueForCharacteristic characteristic: CBCharacteristic!,
                    error: NSError!) {
        if error != nil {
            NSLog("handleDidWriteValue error: \(error)")
        }
        transport.handleWriteAck()
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
            // NSLog("SendCommand call completionHandler")
            handler()
        }
    }

    // function to handle an incoming command message
    func handleCommandMessage(data: NSData) {
        // construct a command
        if let api = self.api {
            api.handleCommandMessage(data)
        }
    }

}


