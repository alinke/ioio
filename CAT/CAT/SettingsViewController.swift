//
//  SettingsViewController.swift
//  CAT
//
//  Created by Kevin Lovette on 8/31/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit
import CoreBluetooth


class SettingsViewController: UIViewController {

    var autoConnect = true

    var settingsView: SettingsView?


    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view, typically from a nib.

        let frame = self.view.bounds
//        NSLog("frame \(frame)")
        self.settingsView = SettingsView(frame: frame)
        self.view.addSubview(self.settingsView!)
    }

    override func viewWillAppear(animated: Bool) {
//        NSLog("viewWillAppear")

        let app = App.sharedInstance
        app.startScan(scanHandler: didDiscoverPeripheral)
    }

    func didDiscoverPeripheral(peripheral: CBPeripheral, advertisementData: [NSObject : AnyObject]) {
        NSLog("didDiscoverPeripheral \(peripheral)")

/*
        // add item to SettingsDataSource
        if let settingsView = self.settingsView {
            if let dataSource = settingsView.settingsDataSource {
                let dataObject = SettingsDataObject(reuseIdentifier: "SettingsCell", indexPath: NSIndexPath(forRow: 0, inSection: 0), name: peripheral.identifier.UUIDString) {
                    (dataObject: SettingsDataObject) -> Void in
                    NSLog("pressed \(dataObject)")
                }
                dataSource.add(dataObject)
                settingsView.reloadData()
            }
        }        
*/
        if self.autoConnect {
            NSLog("  autoConnect")
            let app = App.sharedInstance
            app.connect(peripheral, connectHandler: didConnect)
        }
    }

    func didConnect(peripheral: CBPeripheral) {
        NSLog("didConnect")
    }
    
    override func viewWillDisappear(animated: Bool) {
//        NSLog("viewWillDisappear")
        let app = App.sharedInstance
        app.stopScan()
    }

    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }


}

