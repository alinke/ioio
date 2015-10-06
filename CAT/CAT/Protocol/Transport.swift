//
//  Transport.swift
//  CAT
//
//  Created by Kevin Lovette on 7/18/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit
import CoreBluetooth


//--------------------------------------------------------------------------------
//
// Transport base class
//
public class Transport {
    var device: Device?

    //
    func connect(device: Device) {
        self.device = device
    }

    func disconnect() {
        self.device = nil
    }

    
    //
    // API - Used by Protocol
    //

    func handleUpdateNotificationState(characteristic: CBCharacteristic, error: NSError?) {
    }

    func handleNotification(data: NSData) {
        //Log.info("+++ Transport :: handleNotification  data: \(data)")
        receivePacket(data)
    }

    func handleWriteAck() {
    }

    func sendCommand(command: NSData) {
        if command.length < Packet.mtu {
            self.sendPacket(command, start: 0, count: command.length)
        } else {
            var start = 0
            while start < command.length {
                let count = ( ( ( command.length - start ) < Packet.mtu ) ? ( command.length - start ) : Packet.mtu )
                self.sendPacket(command, start: start, count: count)
                start += count
            }
        }
    }

    func sendPacket(data: NSData, start: Int, count: Int) {
    }
    func receivePacket(data: NSData) {
    }
}


//--------------------------------------------------------------------------------
//
// Write With Response transport
//
public class WriteWithResponseTransport: Transport {

    // Singleton
    public static let sharedInstance = WriteWithResponseTransport()


    override func connect(device: Device) {
        super.connect(device)
    }

    override func disconnect() {
        super.disconnect()
    }

    let packetQueueSemaphore: dispatch_semaphore_t = dispatch_semaphore_create(0)

    override func handleWriteAck() {
        // send next packet
        dispatch_semaphore_signal(self.packetQueueSemaphore)
    }
    
    let maxSequenceNumber = 255

    /// sequence number for the next packet
    var nextLocalSequenceNumber = 0
    var nextRemoteSequenceNumber = 0

    override func sendPacket(data: NSData, start: Int, count: Int) {

        let packet = Packet()
        packet.copyFrom(data, start: start, count: count)
        packet.setHeader(self.nextLocalSequenceNumber, remoteSequenceNumber: self.nextRemoteSequenceNumber)

        // update nextLocalSequenceNumber
        if self.nextLocalSequenceNumber == self.maxSequenceNumber {
            self.nextLocalSequenceNumber = 0
        } else {
            self.nextLocalSequenceNumber++
        }

        // TX packet
        if let device = self.device {
            device.writePacket(packet)
        }

        dispatch_semaphore_wait(self.packetQueueSemaphore, DISPATCH_TIME_FOREVER)
    }

    override func receivePacket(data: NSData) {
        if let device = self.device {
            device.handleCommandMessage(data)
        }
    }
    
}
