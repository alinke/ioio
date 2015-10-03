//
//  GIFFrame.swift
//  CAT
//
//  Created by Kevin Lovette on 7/18/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit


class GIFFrame {
    var width = 0
    var height = 0
    var pixelData: NSData?
    var frameData: NSData?
    var delay = 0
    
    init(pixels: NSData?, frame: NSData, width: Int, height: Int, delay: Int) {
        self.pixelData = pixels
        self.frameData = frame
        self.width = width
        self.height = height
        self.delay = delay
    }

    func image() -> UIImage? {
        if let frameBuffer = self.frameData {
            let framePtr = UnsafeMutablePointer<UInt8>(frameBuffer.bytes)
//            var frameBytes = UnsafeMutableBufferPointer<UInt8>(start: framePtr, count: frameBuffer.length)
                
            let provider = CGDataProviderCreateWithData(nil, framePtr, (self.width * self.height * 4), nil)

            let bitsPerComponent = 8
            let bitsPerPixel = 32
            let bytesPerRow = (self.width * 4)
            let colorSpace = CGColorSpaceCreateDeviceRGB()
            let bitmapInfo = CGBitmapInfo.ByteOrderDefault
            let renderingIntent: CGColorRenderingIntent = CGColorRenderingIntent.RenderingIntentDefault
                
            let cgImage = CGImageCreate(self.width, self.height, bitsPerComponent, bitsPerPixel, bytesPerRow, colorSpace, bitmapInfo, provider, nil, false, renderingIntent)
 
            let newImage = UIImage(CGImage: cgImage!)
            return newImage
        }
        return nil
    }

    // Convert to 16 bit rgb565 color using the frameData
    func frame565() -> NSData? {
        if let frameBuffer = NSMutableData(length: (self.width * self.height * 2)) {
            let framePtr = UnsafeMutablePointer<UInt8>(frameBuffer.mutableBytes)
            let frameBytes = UnsafeMutableBufferPointer<UInt8>(start: framePtr, count: frameBuffer.length)
                
            if let frame = self.frameData {
                let ptr = UnsafeMutablePointer<UInt8>(frame.bytes)
                let bytes = UnsafeMutableBufferPointer<UInt8>(start: ptr, count: frame.length)
                
                for row in 0 ..< self.height {                    
                    for col in 0 ..< self.width {
                        let offset = ( ( (row * self.width) + col ) * 4 )
                        let color = GIFColor(red: bytes[offset], green: bytes[offset+1], blue: bytes[offset+2], alpha: bytes[offset+3])
                        let pixel = color.rgb565()

                        let offset565 = ( ( (row * self.width) + col ) * 2 )
                        frameBytes[offset565] = UInt8( pixel & 0x00ff )
                        frameBytes[offset565 + 1] = UInt8( ( pixel & 0xff00 ) >> 8 )
                    }
                }
            }                
            return frameBuffer
        }
        return nil
    }

    func pixelFrame() -> NSData? {
        if let frame = self.frame565() {
            let ptr = UnsafeMutablePointer<UInt16>(frame.bytes)
            let bytes = UnsafeMutableBufferPointer<UInt16>(start: ptr, count: (frame.length / 2))

            if let frameBuffer = NSMutableData(length: 768) {
                let framePtr = UnsafeMutablePointer<UInt8>(frameBuffer.mutableBytes)
                let frameBytes = UnsafeMutableBufferPointer<UInt8>(start: framePtr, count: frameBuffer.length)
                
                for row in 0 ..< 8 {
                    for col in 0 ..< 32 {
                        var pixel1 = UInt16(0)
                        var pixel2 = UInt16(0)

                        if ( (row < self.height) && (col < self.width) ) {
                            pixel1 = bytes[(row * self.width) + col]
                        }
                        if ( ((row + 8 ) < self.height) && (col < self.width) ) {
                            pixel2 = bytes[((row + 8 ) * self.width) + col]
                        }
                        
                        let offset = ( (row * self.width) + col )

//                        NSLog("\(row)  \(col)  \(offset)   pixel1: \(String(pixel1, radix: 16))  \(String(pixel1, radix: 2))")
                        
                        let r1 = ( ( pixel1 >> 11 ) & 0x1f )
                        let g1 = ( ( pixel1 >> 5 ) & 0x3f )
                        let b1 = ( pixel1 & 0x1f )
                        let r2 = ( ( pixel2 >> 11 ) & 0x1f )
                        let g2 = ( ( pixel2 >> 5 ) & 0x3f )
                        let b2 = ( pixel2 & 0x1f )
                        
                        let p01 = UInt8( ( ( ( ( r1 >> 2 ) & 0x1 ) << 2 ) | ( ( ( g1 >> 3 ) & 0x1 ) << 1 ) | ( ( b1 >> 2 ) & 0x1 ) ) & 0x07 )
                        let p02 = UInt8( ( ( ( ( r2 >> 2 ) & 0x1 ) << 2 ) | ( ( ( g2 >> 3 ) & 0x1 ) << 1 ) | ( ( b2 >> 2 ) & 0x1 ) ) & 0x07 )
                        let p11 = UInt8( ( ( ( ( r1 >> 3 ) & 0x1 ) << 2 ) | ( ( ( g1 >> 4 ) & 0x1 ) << 1 ) | ( ( b1 >> 3 ) & 0x1 ) ) & 0x07 )
                        let p12 = UInt8( ( ( ( ( r2 >> 3 ) & 0x1 ) << 2 ) | ( ( ( g2 >> 4 ) & 0x1 ) << 1 ) | ( ( b2 >> 3 ) & 0x1 ) ) & 0x07 )
                        let p21 = UInt8( ( ( ( ( r1 >> 4 ) & 0x1 ) << 2 ) | ( ( ( g1 >> 5 ) & 0x1 ) << 1 ) | ( ( b1 >> 4 ) & 0x1 ) ) & 0x07 )
                        let p22 = UInt8( ( ( ( ( r2 >> 4 ) & 0x1 ) << 2 ) | ( ( ( g2 >> 5 ) & 0x1 ) << 1 ) | ( ( b2 >> 4 ) & 0x1 ) ) & 0x07 )
                        
                        let f0 = ( ( p01 << 3 ) & 0x38 ) | ( p02 & 0x07 )
                        let f1 = ( ( p11 << 3 ) & 0x38 ) | ( p12 & 0x07 )
                        let f2 = ( ( p21 << 3 ) & 0x38 ) | ( p22 & 0x07 )

//                        let f0 = UInt8(0x21)
//                        let f1 = UInt8(0x21)
//                        let f2 = UInt8(0x21)
                        
                        frameBytes[offset] = f0
                        frameBytes[offset+256] = f1
                        frameBytes[offset+512] = f2
                        
                        //NSLog("\(row)  \(col)  \(offset)   pixel1: \(String(pixel1, radix: 16))  \(String(pixel1, radix: 2))   pixel2: \(String(pixel2, radix: 16))  \(String(pixel2, radix: 2))  f0: \(String(f0, radix: 16)) f1: \(String(f1, radix: 16)) f2: \(String(f2, radix: 16))")
                    }
                }

                return frameBuffer
            }
        }
        return nil
    }


/*
    func dumpFrame(frame: GIFFrame, number: Int) {
        if let data = frame.pixelData {
            let ptr = UnsafePointer<UInt8>(data.bytes)
            var bytes = UnsafeBufferPointer<UInt8>(start: ptr, count: data.length)

            NSLog("frame \(number)  width: \(frame.width)  height: \(frame.height)   size: \(data.length)")
            for row in 0 ..< frame.height {
                var str = ""
                for col in 0 ..< frame.width {
                    let offset = ( (row * frame.width) + col )
                    let b = bytes[offset]
                    var hex = String(b, radix: 16)
                    if count(hex) == 1 {
                        hex = ( "0" + hex )
                    }
                    str += (" " + hex)
                }
                var rs = ( "\(row)" )
                if count(rs) == 1 {
                    rs = ( "0" + rs )
                }
                NSLog("[\(rs)]  \(str)")
            }
            NSLog("--")
        }

        // frameData is RGBa pixels
        if let data = frame.frameData {
            let ptr = UnsafePointer<UInt8>(data.bytes)
            var bytes = UnsafeBufferPointer<UInt8>(start: ptr, count: data.length)

            NSLog("frame \(number)  width: \(frame.width)  height: \(frame.height)   size: \(data.length)")
            for row in 0 ..< frame.height {
                var str = ""
                for col in 0 ..< frame.width {
                    let offset = ( ( (row * frame.width) + col ) * 4 )
                    let r = bytes[offset]
                    let g = bytes[offset+1]
                    let b = bytes[offset+2]
                    let a = bytes[offset+3]
                    var rhex = String(r, radix: 16)
                    if count(rhex) == 1 {
                        rhex = ( "0" + rhex )
                    }
                    var ghex = String(g, radix: 16)
                    if count(ghex) == 1 {
                        ghex = ( "0" + ghex )
                    }
                    var bhex = String(b, radix: 16)
                    if count(bhex) == 1 {
                        bhex = ( "0" + bhex )
                    }
                    var ahex = String(a, radix: 16)
                    if count(ahex) == 1 {
                        ahex = ( "0" + ahex )
                    }
                    str += (" " + rhex + ghex + bhex + ahex )
                }
                var rs = ( "\(row)" )
                if count(rs) == 1 {
                    rs = ( "0" + rs )
                }
                NSLog("[\(rs)]  \(str)")
            }
            NSLog("--")
        }
    }

    func dumpImage(frame: NSData) {
        let ptr = UnsafePointer<UInt8>(frame.bytes)
        var bytes = UnsafeBufferPointer<UInt8>(start: ptr, count: frame.length)
        for subframe in 0 ..< 3 {
            for row in 0 ..< 8 {
                var str = ""
                for col in 0 ..< 32 {
                    let offset = ( (subframe * 256) + (row * 32) + col )
                    let b = bytes[offset]
                    str += (" " + String(b, radix: 16))
                }
                NSLog("[\(subframe) : \(row)]  \(str)")
            }
            NSLog("--")
        }
    }
*/

}
