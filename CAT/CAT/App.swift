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


    func dumpFrame(frame: NSData) {
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

}
