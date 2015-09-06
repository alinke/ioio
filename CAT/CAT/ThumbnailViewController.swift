//
//  ThumbnailViewController.swift
//  LEDPurse
//
//  Created by Kevin Lovette on 7/18/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit
import CoreBluetooth


class ThumbnailViewController: UIViewController {

    @IBOutlet weak var leftBarButton: UIBarButtonItem?

    
    var thumbnailDataSource: ThumbnailDataSource?
    var thumbnailLayout: ThumbnailLayout?
    var thumbnailListView: CollectionView?


    func reconnectHandler(peripheral: CBPeripheral) {
        dispatch_async(dispatch_get_main_queue()) {
            self.leftBarButton!.title = "Connected"
        }
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view, typically from a nib.

        // change navbar background color
        if let navigationController = self.navigationController {
            //navigationController.navigationBar.tintColor = UIColor(hex: "a8aaac")
            navigationController.navigationBar.backgroundColor = UIColor(hex: "a8aaac")
        }

        // create the app to create the Protocol object to start bluetooth
        let app = App.sharedInstance
        // this should try to automatically connect to paired peripherals
        app.startProtocol(reconnectHandler: reconnectHandler)

        let frame = self.view.bounds

//        NSLog("frame \(frame)")

        loadThumbnails()
        //self.showThumbnail(0)

//        let frame = CGRect(x: 0, y: 60, width: 375.0, height: 250.0)
        self.thumbnailDataSource = ThumbnailDataSource(viewController: self, thumbnails: self.thumbnails)
        self.thumbnailLayout = ThumbnailLayout(dataSource: self.thumbnailDataSource!)
        self.thumbnailListView = CollectionView(frame: frame, dataSource: self.thumbnailDataSource!, layout: self.thumbnailLayout!)
        self.thumbnailListView!.registerClass(ThumbnailCellView.self, forCellWithReuseIdentifier:"ThumbnailCell")
        self.thumbnailListView!.backgroundColor = UIColor(hex: "a8aaac")
        self.view.addSubview(self.thumbnailListView!)
                                                                                               
        //self.api = Protocol()
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

        
    override func viewWillAppear(animated: Bool) {
        let app = App.sharedInstance
        if app.isConnected() {
            dispatch_async(dispatch_get_main_queue()) {
                self.leftBarButton!.title = "Connected"
            }
        }
    }


    

    
    var uploadAlert: UIAlertController?
    var uploadSpinner: UIActivityIndicatorView?

    func setupProgress() {
        if uploadAlert == nil {
            self.uploadAlert = UIAlertController(title: "Uploading", message: "", preferredStyle: UIAlertControllerStyle.Alert)
        }
    }
    func startProgress() {
        self.uploadAlert!.message = ( "" )
        self.presentViewController(self.uploadAlert!, animated: false, completion: nil)
    }
    func stopProgress() {
        dispatch_async(dispatch_get_main_queue()) {
            self.uploadAlert!.dismissViewControllerAnimated(false, completion: nil)
        }
    }
    func updateProgress(frame: Int, num: Int) {
        dispatch_async(dispatch_get_main_queue()) {
            self.uploadAlert!.message = ( "Frame \(frame) of \(num)" )
        }
    }
    
    let uploadDispatchQueue: dispatch_queue_t = dispatch_queue_create("com.ioio.uploadDispatchQueue", DISPATCH_QUEUE_SERIAL)

    func thumbnailSelected(thumbnail: Thumbnail) {
        NSLog("upload \(thumbnail.name)")
        self.setupProgress()

        dispatch_async(uploadDispatchQueue) {
            let app = App.sharedInstance
            app.upload(thumbnail, frameHandler: self.updateProgress) {
                () -> Void in
                NSLog("done")
                self.stopProgress()
            }
        }
        
        self.startProgress()
    }



    //
    // GIF Animations
    //
    var thumbnails = [Thumbnail]()
    
    func getAnimationFileNames() -> [String] {
        let docsPath = NSBundle.mainBundle().resourcePath!
        let fileManager = NSFileManager.defaultManager()
        var error: NSError?
        let docsArray = fileManager.contentsOfDirectoryAtPath(docsPath, error:&error)

        var res = [String]()

        if let paths = docsArray {
            for i in 0 ..< paths.count {
                var str = String(paths[i] as! NSString)
                if str.hasSuffix(".gif") {
                    let range = advance(str.endIndex, -4) ..< str.endIndex
                    str.removeRange(range)
                    res.append(str)
                }
            }
        }
        return res
    }

    func loadThumbnails() {
        let names = getAnimationFileNames()
        for name in names {
            self.loadThumbnail(name)
        }
    }

    func loadThumbnail(name: String) {
        if let path: String = NSBundle.mainBundle().pathForResource(name, ofType: "gif") {
            var loadError: NSError?
            if let data = NSData(contentsOfFile: path, options: NSDataReadingOptions.DataReadingMapped, error: &loadError) {
                var decoder = GIFDecoder(data: data)

                if let frame = decoder.getFrame(0) {
                    //NSLog("\(name)  frames: \(decoder.getFrameCount())   width: \(decoder.width)  height: \(decoder.height)")

                    let thumbnail = Thumbnail(name: name, frame: frame)
                    thumbnails.append(thumbnail)
                }
            }
        }
    }

}
