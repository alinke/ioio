//
//  Transport.swift
//  CAT
//
//  Created by Kevin Lovette on 7/18/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit
import CoreBluetooth


class Packet {
    static let headerSize = 2
    static let mtu = 146
 
    var localSequenceNumber: UInt8 = 0
    var remoteSequenceNumber: UInt8 = 0
    
    var size = 0
    var data: NSMutableData?

    var sentTime: NSTimeInterval = 0
    var ackTime: NSTimeInterval = 0


    init() {
        self.data = NSMutableData(length: (Packet.headerSize + Packet.mtu))
    }

    convenience init(data: NSData, start: Int, count: Int) {
        self.init()
        self.copyFrom(data, start: start, count: count)
    }

    
    func copyFrom(data: NSData, start: Int, count: Int) {
        let dataPtr = UnsafePointer<UInt8>(data.bytes)
        let dataBytes = UnsafeBufferPointer<UInt8>(start: dataPtr, count: data.length)
        
        if let buffer = self.data {
            let ptr = UnsafeMutablePointer<UInt8>(buffer.mutableBytes)
            let bytes = UnsafeMutableBufferPointer<UInt8>(start: ptr, count: buffer.length)
            bytes[0] = 0
            bytes[1] = 0
            for index in 0 ..< count {
                bytes[Packet.headerSize + index] = dataBytes[index + start]
            }
            buffer.length = (Packet.headerSize + count)
            self.size = (Packet.headerSize + count)
        }
    }
    
    func setHeader(localSequenceNumber: Int, remoteSequenceNumber: Int) {
        if let buffer = self.data {
            let ptr = UnsafeMutablePointer<UInt8>(buffer.mutableBytes)
            let bytes = UnsafeMutableBufferPointer<UInt8>(start: ptr, count: buffer.length)

            self.localSequenceNumber = UInt8(localSequenceNumber)
            self.remoteSequenceNumber = UInt8(remoteSequenceNumber)

            bytes[0] = self.localSequenceNumber
            bytes[1] = self.remoteSequenceNumber
        }
    }

    func dump() {
        if let buffer = self.data {
            let ptr = UnsafeMutablePointer<UInt8>(buffer.mutableBytes)
            let bytes = UnsafeMutableBufferPointer<UInt8>(start: ptr, count: buffer.length)

            Log.info("Packet  local: \(self.localSequenceNumber)  remote: \(self.remoteSequenceNumber)  size: \(size)")
            var str = ""
            for i in 0 ..< self.size {
                str += ( String(bytes[i], radix: 16) + " " )
            }
            Log.info("  \(str)")
        }
    }
}


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
