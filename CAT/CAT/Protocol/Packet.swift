//
//  Packet.swift
//  CAT
//
//  Created by Kevin Lovette on 7/18/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit
import CoreBluetooth


///
/// Packet
///
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

