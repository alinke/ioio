//
//  Transport.swift
//  LEDPurse
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
        var dataBytes = UnsafeBufferPointer<UInt8>(start: dataPtr, count: data.length)
        
        if let buffer = self.data {
            let ptr = UnsafeMutablePointer<UInt8>(buffer.mutableBytes)
            var bytes = UnsafeMutableBufferPointer<UInt8>(start: ptr, count: buffer.length)
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
            var bytes = UnsafeMutableBufferPointer<UInt8>(start: ptr, count: buffer.length)

            self.localSequenceNumber = UInt8(localSequenceNumber)
            self.remoteSequenceNumber = UInt8(remoteSequenceNumber)

            bytes[0] = self.localSequenceNumber
            bytes[1] = self.remoteSequenceNumber
        }
    }

    func dump() {
        if let buffer = self.data {
            let ptr = UnsafeMutablePointer<UInt8>(buffer.mutableBytes)
            var bytes = UnsafeMutableBufferPointer<UInt8>(start: ptr, count: buffer.length)

            NSLog("Packet  local: \(self.localSequenceNumber)  remote: \(self.remoteSequenceNumber)  size: \(size)")
            var str = ""
            for i in 0 ..< self.size {
                str += ( String(bytes[i], radix: 16) + " " )
            }
            NSLog("  \(str)")
        }
    }
}


//
// Transport base class
//
public class Transport {
    var peripheral: CBPeripheral?
    var characteristic: CBCharacteristic?

    //
    //
    //
    func connect(peripheral: CBPeripheral?, characteristic: CBCharacteristic?, completionHandler: ((CBPeripheral) -> Void)? = nil) {
        self.peripheral = peripheral
        self.characteristic = characteristic
    }

    func disconnect() {
    }

    
    //
    // API - Used by Protocol
    //

    func handleUpdateNotificationState(characteristic: CBCharacteristic, error: NSError?) {
    }

    func handleNotification(data: NSData) {
        //NSLog("+++ Transport :: handleNotification  data: \(data)")
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


//
// Write With Response transport
//
public class WriteWithResponseTransport: Transport {

    // Singleton
    public static let sharedInstance = WriteWithResponseTransport()

    var connectCompletionHandler: ((CBPeripheral) -> Void)?

    override func connect(peripheral: CBPeripheral?, characteristic: CBCharacteristic?, completionHandler: ((CBPeripheral) -> Void)? = nil) {
        super.connect(peripheral, characteristic: characteristic)

        // enable notifications
        self.peripheral!.setNotifyValue(true, forCharacteristic: self.characteristic!)
        self.connectCompletionHandler = completionHandler
    }

    override func disconnect() {
        super.disconnect()
    }

    override func handleUpdateNotificationState(characteristic: CBCharacteristic, error: NSError?) {
        NSLog("Transport CONNECTED  peripheral: \(self.peripheral)  characteristic: \(self.characteristic)")

        // call handler
        if let handler = self.connectCompletionHandler {
            handler(self.peripheral!)
        }
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
        if let peripheral = self.peripheral {
            if let characteristic = self.characteristic {
                peripheral.writeValue(packet.data, forCharacteristic: characteristic, type: CBCharacteristicWriteType.WithResponse)
            }
        }

        dispatch_semaphore_wait(self.packetQueueSemaphore, DISPATCH_TIME_FOREVER)
    }

    override func receivePacket(data: NSData) {
        NSLog("--> receivePacket len: \(data.length)  data: \(data)")
    }
    
}


//
// Write Without Response transport
//
public class WriteWithoutResponseTransport: Transport {

    // Singleton
    public static let sharedInstance = WriteWithoutResponseTransport()

    //packetQueue = PacketQueue(peripheral: self.dongle!, characteristic: self.dongleCharacteristic!, size: 64)

    override init() {
        super.init()
        self.initTimer()
    }

    var connectCompletionHandler: ((CBPeripheral) -> Void)?

    override func connect(peripheral: CBPeripheral?, characteristic: CBCharacteristic?, completionHandler: ((CBPeripheral) -> Void)? = nil) {
        super.connect(peripheral, characteristic: characteristic)

        // enable notifications
        self.peripheral!.setNotifyValue(true, forCharacteristic: self.characteristic!)
        
        self.connectCompletionHandler = completionHandler
    }

    override func disconnect() {
        super.disconnect()
        self.stopSendTimer()
    }

    override func handleUpdateNotificationState(characteristic: CBCharacteristic, error: NSError?) {
        // start timer
        self.startSendTimer()
        NSLog("Transport CONNECTED  peripheral: \(self.peripheral)  characteristic: \(self.characteristic)")

        // call handler
        if let handler = self.connectCompletionHandler {
            handler(self.peripheral!)
        }
    }

        
    //
    // Packets
    //
    override func sendPacket(data: NSData, start: Int, count: Int) {
        // get a free packet
        let packet = Packet()
        packet.copyFrom(data, start: start, count: count)
        self.queuePacket(packet)
    }

    // keep track of time since the last received packet to check of the connection has timed out
    
    override func receivePacket(data: NSData) {
        NSLog("--> receivePacket len: \(data.length)  data: \(data)")
    }


    //
    // Internal code
    //
    
    let packetQueueMax = 16
    let packetQueueMiddle = 8
    
    var packetQueue = [Packet]()
    let packetQueueDispatchQueue: dispatch_queue_t = dispatch_queue_create("com.ioio.packetQueue", DISPATCH_QUEUE_SERIAL)
    let packetQueueSemaphore: dispatch_semaphore_t = dispatch_semaphore_create(0)
    
    func queuePacket(packet: Packet) {
//        NSLog("queuePacket \(packet.sequenceNumber)   size: \(packetQueue.count)")
//        let thread = NSThread.currentThread()
//        NSLog("  Thread: \(thread.description)  isMain: \(thread.isMainThread)")
        
        dispatch_sync(self.packetQueueDispatchQueue) {
//            let thread = NSThread.currentThread()
//            NSLog("    sync  Thread: \(thread.description)  isMain: \(thread.isMainThread)")
            self.packetQueue.append(packet)
        }
        if self.packetQueue.count == self.packetQueueMax {
//            let thread = NSThread.currentThread()
//            NSLog("  WAIT  count: \(self.packetQueue.count)   Thread: \(thread.description)  isMain: \(thread.isMainThread)")
            dispatch_semaphore_wait(self.packetQueueSemaphore, DISPATCH_TIME_FOREVER)
        }
    }

    func dequeuePacket() -> Packet? {
        var packet: Packet?
        dispatch_sync(self.packetQueueDispatchQueue) {
//            let thread = NSThread.currentThread()
//            NSLog("dequeuePacket  count: \(self.packetQueue.count)   Thread: \(thread.description)  isMain: \(thread.isMainThread)")
            if self.packetQueue.count > 0 {
                packet = self.packetQueue[0]
                self.packetQueue.removeAtIndex(0)
//                NSLog("  new  count: \(self.packetQueue.count)")
            }
        }
        if self.packetQueue.count == self.packetQueueMiddle {
//            let thread = NSThread.currentThread()
//            NSLog("  SIGNAL  count: \(self.packetQueue.count)   Thread: \(thread.description)  isMain: \(thread.isMainThread)")
            dispatch_semaphore_signal(self.packetQueueSemaphore)
        }
        return packet
    }


    let maxSequenceNumber = 255

    /// sequence number for the next packet
    var nextLocalSequenceNumber = 0
    var nextRemoteSequenceNumber = 0

    var sentPackets = [Packet]()


    func sendNextPacket(delta: UInt64) {
//        let thread = NSThread.currentThread()
//        NSLog("tick - delta: \(delta)   Thread: \(thread.description)  isMain: \(thread.isMainThread)")

        if let packet = self.dequeuePacket() {
            // assign the packet the next sequence number
            packet.setHeader(self.nextLocalSequenceNumber, remoteSequenceNumber: self.nextRemoteSequenceNumber)

            // update nextLocalSequenceNumber
            if self.nextLocalSequenceNumber == self.maxSequenceNumber {
                self.nextLocalSequenceNumber = 0
            } else {
                self.nextLocalSequenceNumber++
            }

            // check for existing packet with same sequence id
            sentPackets.append(packet)

            // send packet
            NSLog("delta:  \(delta)   pending: \(self.packetQueue.count)   sent: \(self.sentPackets.count)  local: \(self.nextLocalSequenceNumber)  remote: \(self.nextRemoteSequenceNumber)")
            //packet.dump()

            // TX packet
            if let peripheral = self.peripheral {
                if let characteristic = self.characteristic {
                    peripheral.writeValue(packet.data, forCharacteristic: characteristic, type: CBCharacteristicWriteType.WithoutResponse)
                }
            }
        }
    }


    
    //
    // Timer
    //

    //var sendRate = Double( 1.0 / 10.0 )
    var sendRate = Double( 1.0 / 5.0 )
    var info : mach_timebase_info = mach_timebase_info(numer: 0, denom: 0)
    
    func initTimer() {
        mach_timebase_info(&info)

        self.lastTick = mach_absolute_time()
        //createSendTimer(self.interval, leeway: self.leeway) {
        let interval = UInt64(self.sendRate * Double(NSEC_PER_SEC))

        createSendTimer(interval, leeway: 0, tickHandler: sendTickHandler)
    }


    var lastTick: UInt64 = 0
    var tickTime: UInt64 = 0
//    var interval: UInt64 = 0
//    var leeway: UInt64 = 0

    func sendTickHandler() {
        var now: UInt64 = mach_absolute_time()

        // send a packet
        let dt = ((now - self.lastTick) * UInt64(self.info.numer) / UInt64(self.info.denom))
        sendNextPacket(dt)

        // last tick time
        self.lastTick = now

        // re-sechedule timer
        // let lastTickTimeNanoseconds = (self.tickTime * UInt64(self.info.numer) / UInt64(self.info.denom))
        // let nextInterval = UInt64(self.sendRate * Double(NSEC_PER_SEC))
        // dispatch_source_set_timer(self.timer, dispatch_time(DISPATCH_TIME_NOW, Int64(nextInterval - lastTickTimeNanoseconds)), 0, 0)

        // re-sechedule timer
/*
        let tickTimeNanoseconds = (self.tickTime * UInt64(self.info.numer) / UInt64(self.info.denom))
        let nextInterval = UInt64(self.sendRate * Double(NSEC_PER_SEC))
        let delay = Int64(nextInterval - tickTimeNanoseconds)
        dispatch_source_set_timer(self.timer, dispatch_time(DISPATCH_TIME_NOW, delay), 0, 0)
*/
        
        // record handler execution time
        self.tickTime = ( mach_absolute_time() - now )
    }

    func sendTickHandlerPost() {
        var now: UInt64 = mach_absolute_time()

        // send a packet
        let dt = ((now - self.lastTick) * UInt64(self.info.numer) / UInt64(self.info.denom))
        sendNextPacket(dt)

        // last tick time
        self.lastTick = now

        // re-sechedule timer
        // let lastTickTimeNanoseconds = (self.tickTime * UInt64(self.info.numer) / UInt64(self.info.denom))
        // let nextInterval = UInt64(self.sendRate * Double(NSEC_PER_SEC))
        // dispatch_source_set_timer(self.timer, dispatch_time(DISPATCH_TIME_NOW, Int64(nextInterval - lastTickTimeNanoseconds)), 0, 0)

        // record handler execution time
        let tickTime = ( mach_absolute_time() - now )

        // re-sechedule timer

        let tickTimeNanoseconds = (tickTime * UInt64(self.info.numer) / UInt64(self.info.denom))
        let nextInterval = UInt64(self.sendRate * Double(NSEC_PER_SEC))
        let delay = Int64(nextInterval - tickTimeNanoseconds)
        dispatch_source_set_timer(self.timer, dispatch_time(DISPATCH_TIME_NOW, delay), 0, 0)
    }

    
    let sendDispatchQueue: dispatch_queue_t = dispatch_queue_create("com.ioio.sendDispatchQueue", DISPATCH_QUEUE_SERIAL)    

    //var timer: dispatch_source_t! = nil
    var timer: dispatch_source_t!

    ///
    /// Create the send timer.
    ///
    /// - Parameter interval: The nanosecond interval for the timer
    /// - Parameter leeway: The amount of time, in nanoseconds, that the system can defer the timer
    ///
    func createSendTimer(interval: UInt64, leeway: UInt64, tickHandler: () -> Void) {
        self.timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, DISPATCH_TIMER_STRICT, self.sendDispatchQueue)
        if timer != nil {
            var start: dispatch_time_t = dispatch_time(DISPATCH_TIME_NOW, 0)
            dispatch_source_set_timer(timer, start, interval, leeway)
            dispatch_source_set_event_handler(timer, tickHandler)
        }
    }
    
    func startSendTimer() {
        if timer != nil {
            // start the timer
            dispatch_resume(timer)
        }
    }

    func stopSendTimer() {
        if timer != nil {
            dispatch_suspend(timer)
        }
    }

    func cancelSendTimer() {
        if timer != nil {
            dispatch_source_cancel(timer)
        }
    }

}
