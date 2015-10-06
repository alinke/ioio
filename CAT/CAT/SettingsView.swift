//
//  SettingsView.swift
//  CAT
//
//  Created by Kevin Lovette on 8/31/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit


class SettingsDataObject: CollectionDataObject {

    var name: String?
    var device: Device?
    var selectHandler: ((SettingsDataObject) ->Void)?
    
    convenience init(reuseIdentifier identifier: String, indexPath path: NSIndexPath, device: Device? = nil, name: String, selectHandler: ((SettingsDataObject) -> Void)?) {
        self.init(reuseIdentifier: identifier, indexPath: path)
        self.name = name
        self.device = device
        self.selectHandler = selectHandler
    }

    override func size() -> CGSize {
        return CGSize(width: 375.0, height: 200.0)
    }

    override func selectItem() {
        if let handler = self.selectHandler {
            handler(self)
        }
    }
}

extension SettingsDataObject: CustomStringConvertible {
    var description: String {
        return ("\(self.name)  \(device)")
    }
}


//
// DataSource
//
class SettingsDataSource: CollectionDataSource {

    override func setupData() {
        let section: [SettingsDataObject] = [SettingsDataObject]()
        //section.append(SettingsDataObject(reuseIdentifier: "SettingsCell", indexPath: NSIndexPath(forRow: 0, inSection: 0), name: "Scanning", selectHandler: scanSelected))
        self.sections.append(section)
    }

    func add(dataObject: SettingsDataObject) {
        let section = self.sections[0]
        dataObject.indexPath = NSIndexPath(forRow: section.count, inSection: 0)
        dataObject.layoutAttributes = UICollectionViewLayoutAttributes(forCellWithIndexPath: dataObject.indexPath)
        
        Log.info("--")
        Log.info("SettingsDataSource ADD")
        Log.info("      row: \(dataObject.indexPath.row)")
        Log.info("  section: \(dataObject.indexPath.section)")
        Log.info("   layout: \(dataObject.layoutAttributes)")

        self.sections[0].append(dataObject)

        Log.info("--")
        for (index, row) in self.sections[0].enumerate() {
            Log.info("section[0][\(index)] = \(row)")
        }
        Log.info("--")
    }
    
    func scanSelected(dataObject: SettingsDataObject) {
        Log.info("scanSelected \(dataObject)");
    }

    func deviceExists(device: Device) -> SettingsDataObject? {
        let section = self.sections[0]
        for row in section {
            if let obj = row as? SettingsDataObject {
                if let rowDevice = obj.device {
                    if device == rowDevice {
                        return obj
                    }
                }
            }
        }
        return nil
    }
}


class SettingsLayout: CollectionViewLayout {

    override init(dataSource: CollectionDataSource) {
        super.init(dataSource: dataSource)

        self.itemInsets = UIEdgeInsets(top: 16.0, left: 8.0, bottom: 8.0, right: 8.0)
        self.itemSize = CGSize(width: 359.0, height: 100.0)
        self.interItemSpacingX = 8.0
//        self.numberOfRows = 4
    }
    
    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
    }

    override func setupLayout(object: CollectionDataObject) {
        let x: CGFloat = ( self.itemInsets.left )
        let y: CGFloat = ( self.itemInsets.top + ( CGFloat(object.indexPath.row) * ( self.itemSize.height + self.interItemSpacingX ) ) )
        let frame = CGRect(x: x, y: y, width: self.itemSize.width, height: self.itemSize.height)

        if let obj = object as? SettingsDataObject {
            Log.info("SettingsLayout row: \(object.indexPath.row)  \(obj.name)  frame: \(frame)")
        } else {
            Log.info("SettingsLayout row: \(object.indexPath.row)  frame: \(frame)")
        }
        object.layoutAttributes.frame = frame
    }

    override func collectionViewContentSize() -> CGSize {
        // return the size of the cells + padding
//        let numRows: Int = self.collectionDataSource!.numberOfRows(inSection: 0)
        let numRows = self.LnumberOfRows(0)
        
        let width: CGFloat = ( self.itemInsets.left + self.itemInsets.right + self.itemSize.width )
        let height: CGFloat = ( self.itemInsets.top + self.itemInsets.bottom + ( CGFloat(numRows) * ( self.itemSize.height + self.interItemSpacingX ) ) )

        Log.info("SettingsLayout contentSize  width: \(width)  height: \(height)")
        return CGSize(width: width, height: height)
    }

}


class SettingsCellView: CollectionDataObjectCellView {

    var separatorView: UIView?

//    var iconView: UIImageView?
    var connectView: UILabel?
    var titleView: UILabel?
    
    override init(frame aRect: CGRect) {
        super.init(frame: aRect)

        Log.info("SettingsCellView frame: \(frame)")

        let connectFrame = CGRect(x:20.0, y:24.0, width:60.0, height:36.0)
        self.connectView = UILabel(frame: connectFrame)
        self.connectView!.font = UIFont.systemFontOfSize(24.0)
//        self.connectView!.textColor = UIColor.whiteColor()
        self.connectView!.textColor = UIColor.blackColor()
        self.addSubview(self.connectView!)

        //var titleFrame = CGRect(x:20, y:44.0, width:300.0, height:36.0)
        let titleFrame = CGRect(x:88, y:24.0, width:240.0, height:36.0)
        self.titleView = UILabel(frame: titleFrame)
        self.titleView!.font = UIFont.systemFontOfSize(24.0)
//        self.titleView!.textColor = UIColor.whiteColor()
        self.titleView!.textColor = UIColor.blackColor()
        self.addSubview(self.titleView!)
        
        // top separator line
        let separatorFrame = CGRect(x:20.0, y:84.0, width: (frame.width - 40.0), height:1.0)
        self.separatorView = UIView(frame: separatorFrame)
        // from #111e35  to #233d53
        self.separatorView!.backgroundColor = UIColor(hex: "000000", alpha: 0.3)
        self.addSubview(self.separatorView!)

    }

    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
    }


    override func prepareForReuse() {
//        Log.info("prepareForReuse")
    }

    override func setupCell(object: CollectionDataObject) {
        if let obj = object as? SettingsDataObject {
            Log.info("setupCell \(obj.name)  frame: \(self.frame)  layout: \(object.layoutAttributes.frame)")

            self.titleView!.text = obj.name

            if let device = obj.device {
                if device.isConnected() {
                    self.connectView!.text = "C"
                } else {
                    self.connectView!.text = ""
                }
            }
        }
    }
}



class SettingsView: UIView {

    var gradientLayer: CAGradientLayer = CAGradientLayer()
    
    var settingsDataSource: SettingsDataSource?
    var settingsLayout: SettingsLayout?
    var settingsView: CollectionView?

    var titleView: UILabel?


    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
    }

    override init(frame: CGRect) {
        super.init(frame: frame)

        // background gradient
        self.gradientLayer.frame = self.bounds
        self.backgroundColor = UIColor(hex: "ffffff", alpha: 0.0)
//        let color1 = UIColor(hex: "10182c").CGColor as CGColorRef
//        let color2 = UIColor(hex: "26363c").CGColor as CGColorRef
        let color1 = UIColor(hex: "a8aaac").CGColor as CGColorRef
        let color2 = UIColor(hex: "a8aaac").CGColor as CGColorRef
        self.gradientLayer.colors = [color1, color2]
        self.gradientLayer.locations = [0.0, 1.0]
        self.layer.addSublayer(self.gradientLayer)
/*
        var titleFrame = CGRect(x:20, y:76.0, width:300.0, height:36.0)
        self.titleView = UILabel(frame: titleFrame)
        self.titleView!.font = UIFont.systemFontOfSize(24.0)
        self.titleView!.textColor = UIColor.whiteColor()
        self.addSubview(self.titleView!)
        self.titleView!.text = "Scanning"
*/        
        // vertical collection devices
//        let menuFrame = CGRect(x:0, y:120.0, width:375.0, height:547.0)

        let menuFrame = CGRect(x:0, y:0.0, width:375.0, height:667.0)
        self.settingsDataSource = SettingsDataSource()
        self.settingsLayout = SettingsLayout(dataSource: self.settingsDataSource!)
        self.settingsView = CollectionView(frame: menuFrame, dataSource: self.settingsDataSource!, layout: self.settingsLayout!)
        self.settingsView!.registerClass(SettingsCellView.self, forCellWithReuseIdentifier:"SettingsCell")
        self.settingsView!.backgroundColor = UIColor(hex: "ffffff", alpha: 0.0)
        self.settingsView!.opaque = false
        self.addSubview(self.settingsView!)
    }

    func reloadData() {
        dispatch_async(dispatch_get_main_queue()) {
            self.settingsView!.reloadData()
        }
    }
}

