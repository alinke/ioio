//
//  App.swift
//  CAT
//
//  Created by Kevin Lovette on 8/31/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit
import CoreBluetooth


class App {
    
    static let sharedInstance = App()

    init() {
    }
    
    var api: Protocol?

    func startProtocol(reconnectHandler: ((CBPeripheral) -> Void)? = nil) {
        self.api = Protocol(reconnectHandler: reconnectHandler)
    }

    func isConnected() -> Bool {
        if let api = self.api {
            return api.isConnected()
        }
        return false
    }
    
    func startScan(scanHandler: ((CBPeripheral, [NSObject : AnyObject]) -> Void)? = nil) {
        self.api!.startScan(scanHandler: scanHandler)
    }
    func stopScan() {
        self.api!.stopScan()
    }
    func connect(peripheral: CBPeripheral, connectHandler: ((CBPeripheral) -> Void)? = nil) {
        self.api!.connect(peripheral, connectHandler: connectHandler)
    }
        
    
    func upload(thumbnail: Thumbnail, frameHandler: ((Int, Int) -> Void)? = nil, completionHandler: (() -> Void)? = nil) {
        if let path: String = NSBundle.mainBundle().pathForResource(thumbnail.name, ofType: "gif") {
            var loadError: NSError?
            if let data = NSData(contentsOfFile: path, options: NSDataReadingOptions.DataReadingMapped, error: &loadError) {
                var decoder = GIFDecoder(data: data)
                //decoder.debug = true
                decoder.read()

                var numFrames = decoder.getFrameCount()
                var images = [NSData]()
                for i in 0 ..< numFrames {
                    if let frame = decoder.getFrame(i) {
                        if let image = frame.pixelFrame() {
                            images.append(image)
                        }
                    }
                }
                
                self.api!.matrixWriteFile(10.0, shifterLen32: 0x01, rows: 8) {
                    for i in 0 ..< numFrames {
                        let frame = images[i]
                        self.api!.matrixFrame(frame) {
                            () -> Void in
                            if let handler = frameHandler {
                                handler(i, numFrames)
                            }
                        }
                    }

                    self.api!.matrixEnable(0x00, rows: 8) {
                        () -> Void in
                        if let handler = completionHandler {
                            handler()
                        }
                    }
                }
            }
        }
    }

    func sendWriteFile() {
        self.api!.matrixWriteFile(10.0, shifterLen32: 0x01, rows: 8) {
            () -> Void in
        }
    }

    func sendPlay() {
        self.api!.matrixEnable(0x00, rows: 8) {
            () -> Void in
        }
    }

    func sendInteractive() {
        self.api!.matrixEnable(0x01, rows: 8) {
            () -> Void in
        }
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
