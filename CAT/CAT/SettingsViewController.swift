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
        app.startScan(didDiscoverDevice)
    }

    func didSelectDevice(device: Device) {
        NSLog("didSelectDevice: \(device.name)")

        let app = App.sharedInstance
        // disconnect from any connected devices
        if app.isConnected() {
            app.disconnect(app.currentDevice!) {
                (disconnectedDevice: Device) -> Void in
                // connect to the selected device.
                app.connect(device, connectHandler: self.didConnectDevice)
            }
        } else {
            // connect to the selected device.
            app.connect(device, connectHandler: self.didConnectDevice)
        }
    }

    func didConnectDevice(device: Device) {
        NSLog("didConnectDevice \(device)")
        let app = App.sharedInstance
        app.didConnect(device)
        
        if let settingsView = self.settingsView {
            settingsView.reloadData()
        }
    }    
    
    func didDiscoverDevice(device: Device) {
        NSLog("didDiscoverDevice \(device)")

        // Update the UI - add device to the SettingsDataSource
        if let settingsView = self.settingsView {
            if let dataSource = settingsView.settingsDataSource {
                if dataSource.deviceExists(device) == nil {
//                if let obj = dataSource.deviceExists(device) {
//                } else {
                    // add device
                    let dataObject = SettingsDataObject(reuseIdentifier: "SettingsCell", indexPath: NSIndexPath(forRow: 0, inSection: 0), device: device, name: device.name) {
                        (dataObject: SettingsDataObject) -> Void in
                        if let device = dataObject.device {
                            self.didSelectDevice(device)
                        }
                    }
                    dataSource.add(dataObject)
                    settingsView.reloadData()
                }
            }
        }        

/*
        if self.autoConnect {
            NSLog("  autoConnect")
            let app = App.sharedInstance
            app.connect(peripheral, connectHandler: didConnect)
        }
*/
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

