//
//  GIFStream.swift
//  CAT
//
//  Created by Kevin Lovette on 7/18/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit


class GIFStream {
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

    func readBytes(ptr: UnsafeMutablePointer<UInt8>, length: Int) {
        for index in 0 ..< length {
            ptr[index] = bytes[pos]
            pos++
        }
    }
}

