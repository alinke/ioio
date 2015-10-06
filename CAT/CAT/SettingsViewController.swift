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
        Log.info("SettingsView frame \(frame)")
        
        self.settingsView = SettingsView(frame: frame)
        self.view.addSubview(self.settingsView!)
    }

    override func viewWillAppear(animated: Bool) {
//        Log.info("viewWillAppear")

        let app = App.sharedInstance

        // add the currently connected device to the list of devices
        if let currentDevice = app.currentDevice {
            self.didDiscoverDevice(currentDevice)
        }        
        
        app.startScan(didDiscoverDevice)
    }

    override func viewDidAppear(animated: Bool) {
/*
        //        Log.info("viewDidAppear")

        let app = App.sharedInstance

        // add the currently connected device to the list of devices
        if let currentDevice = app.currentDevice {
            self.didDiscoverDevice(currentDevice)
        }        
        
        app.startScan(didDiscoverDevice)
*/
    }

    func didSelectDevice(device: Device) {
        Log.info("didSelectDevice: \(device.name)")

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
        Log.info("didConnectDevice \(device)")
        let app = App.sharedInstance
        app.didConnect(device)
        
        if let settingsView = self.settingsView {
            settingsView.reloadData()
        }
    }    
    
    func didDiscoverDevice(device: Device) {
        Log.info("didDiscoverDevice \(device)")

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
            Log.info("  autoConnect")
            let app = App.sharedInstance
            app.connect(peripheral, connectHandler: didConnect)
        }
*/
    }


    func didConnect(peripheral: CBPeripheral) {
        Log.info("didConnect")
    }
    
    override func viewWillDisappear(animated: Bool) {
//        Log.info("viewWillDisappear")
        let app = App.sharedInstance
        app.stopScan()
    }

    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }


}

