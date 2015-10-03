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


    func reconnectHandler(device: Device) {
        dispatch_async(dispatch_get_main_queue()) {
            self.leftBarButton!.title = "Connected"
        }
    }

    func disconnectHandler(device: Device) {
        dispatch_async(dispatch_get_main_queue()) {
            self.leftBarButton!.title = "Disconnected"
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
        app.startProtocol(reconnectHandler, disconnectHandler: disconnectHandler)

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
        } else {
            dispatch_async(dispatch_get_main_queue()) {
                self.leftBarButton!.title = "Disconnected"
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
        let docsArray: [AnyObject]?
        do {
            docsArray = try fileManager.contentsOfDirectoryAtPath(docsPath)
        } catch let error1 as NSError {
            error = error1
            docsArray = nil
        }

        var res = [String]()

        if let paths = docsArray {
            for i in 0 ..< paths.count {
                var str = String(paths[i] as! NSString)
                if str.hasSuffix(".gif") {
                    let range = str.endIndex.advancedBy(-4) ..< str.endIndex
                    str.removeRange(range)
                    res.append(str)
                }
            }
        }
        return res
    }

    func loadThumbnails() {
        let names = getAnimationFileNames()
        var thumbs = [Thumbnail]()        
        for name in names {
            if let thumbnail = self.loadThumbnail(name) {
                thumbs.append(thumbnail)
            }
        }

        for name in thumbOrder {
            for thumb in thumbs {
                if thumb.name == name {
                    thumbnails.append(thumb)
                }
            }
        }
    }

    func loadThumbnail(name: String) -> Thumbnail? {
        if let path: String = NSBundle.mainBundle().pathForResource(name, ofType: "gif") {
            var loadError: NSError?
            do {
                let data = try NSData(contentsOfFile: path, options: NSDataReadingOptions.DataReadingMapped)
                let decoder = GIFDecoder(data: data)

                let thumbNum = Thumbnail.thumbnailFrame(name)
                if let frame = decoder.getFrame(thumbNum) {
                    //NSLog("width: \(decoder.width)  height: \(decoder.height)  frames: \(decoder.getFrameCount())  thumb: \(thumbNum)   \(name)")

                    let thumbnail = Thumbnail(name: name, frame: frame)
                    return thumbnail
                }
            } catch let error as NSError {
                loadError = error
            }
        }
        return nil
    }

    let thumbOrder = [
        "0catandfish16",
        "0line-game",
        "barber_shop",
        "deuces",
        "diagonals",
        "equalizer1",
        "equalizer2",
        "merging-arrows16",
        "octopus16",
        "0apaulrgreenmonster16",
        "0apaulrkanji16",
        "0apaulrkanji216",
        "0fire16",
        "0glitter16",
        "0hermippe16_bw",
        "0matrix16",
        "0pacghosts16",
        "0petit_kitty16",
        "0rain16",
        "1adata16",
        "Angled_100_Perc",
        "Angled_Color-Change_100_Perc_2",
        "Circles_100_Perc_2",
        "Dueces_Darker_100_Perc",
        "Fleek_100_Perc_2",
        "Lattice_Color_Change_100_Perc",
        "Trill_Animation_100_perc",
        "arrowfinal16",
        "bike16",
        "boat16",
        "bubbles16",
        "colortiles16",
        "firehalf16",
        "firewhole16",
        "fliptile16",
        "float16",
        "flow16",
        "fountain16",
        "gousa",
        "lines16",
        "mbutterflies_black16",
        "pauldots16",
        "paulglasses16",
        "paulrainbow16",
        "paulrcircles16",
        "paulrfallingdrops16",
        "paulrgreenmonser16",
        "paulrnintendo16",
        "paulrpackages_left16",
        "paulrpackages_right16",
        "paulrpurpleguy",
        "paulrteeth16",
        "paulryellowcat16",
        "paulryellowguy",
        "paultriangle16",
        "postman_dog16",
        "quest16",
        "rshifter16",
        "rspray16",
        "rstarburst16",
        "sboxerpink16",
        "shark16",
        "sjumppink16",
        "squid16",
        "triopy16",
        "usnowy16",
        "waterflow16",
        "worm16",
    ]
}
