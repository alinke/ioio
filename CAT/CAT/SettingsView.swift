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
    var selectHandler: ((SettingsDataObject) ->Void)?
    
    convenience init(reuseIdentifier identifier: String, indexPath path: NSIndexPath, name: String, selectHandler: ((SettingsDataObject) -> Void)?) {
        self.init(reuseIdentifier: identifier, indexPath: path)
        self.name = name
        self.selectHandler = selectHandler
    }

    override func size() -> CGSize {
        return CGSize(width: 375.0, height: 200.0)
    }

    func didSelect() {
        if let handler = self.selectHandler {
            handler(self)
        }
    }
}


class SettingsDataSource: CollectionDataSource {

    override func setupData() {
        var section: [SettingsDataObject] = [SettingsDataObject]()
        section.append(SettingsDataObject(reuseIdentifier: "SettingsCell", indexPath: NSIndexPath(forRow: 0, inSection: 0), name: "Scanning", selectHandler: scanSelected))
        sections.append(section)
    }

    func add(dataObject: SettingsDataObject) {
        var section = self.sections[0]
        dataObject.indexPath = NSIndexPath(forRow: count(section), inSection: 0)
        section.append(dataObject)
    }
    
    func scanSelected(dataObject: SettingsDataObject) {
        NSLog("scanSelected \(dataObject)");
    }
}


class SettingsLayout: CollectionViewLayout {

    override init(dataSource: CollectionDataSource) {
        super.init(dataSource: dataSource)

        self.itemInsets = UIEdgeInsets(top: 8.0, left: 8.0, bottom: 8.0, right: 8.0)
        self.itemSize = CGSize(width: 359.0, height: 126.0)
        self.interItemSpacingX = 8.0
        self.numberOfRows = 4
    }
    
    required init(coder decoder: NSCoder) {
        super.init(coder: decoder)
    }

    override func setupLayout(object: CollectionDataObject) {
        var x: CGFloat = ( self.itemInsets.left )
        var y: CGFloat = ( self.itemInsets.top + ( CGFloat(object.indexPath.row) * ( self.itemSize.height + self.interItemSpacingX ) ) )
        let frame = CGRect(x: x, y: y, width: self.itemSize.width, height: self.itemSize.height)
//        NSLog("setupLayout \(object.indexPath.row)  frame: \(frame)")
        object.layoutAttributes.frame = frame
    }

    override func collectionViewContentSize() -> CGSize {
        // return the size of the cells + padding
        var numRows: Int = self.collectionDataSource!.numberOfRows(inSection: 0)
        
        var width: CGFloat = ( self.itemInsets.left + self.itemInsets.right + self.itemSize.width )
        var height: CGFloat = ( self.itemInsets.top + self.itemInsets.bottom + ( CGFloat(numRows) * ( self.itemSize.height + self.interItemSpacingX ) ) )

//        NSLog("collectionViewContentSize width: \(width)  height: \(height)")
        return CGSize(width: width, height: height)
    }

}


class SettingsCellView: CollectionDataObjectCellView {

    var separatorView: UIView?

//    var iconView: UIImageView?
    var titleView: UILabel?
    
    override init(frame aRect: CGRect) {
        super.init(frame: aRect)

//        NSLog("SettingsCellView frame: \(frame)")

        // top separator line
        var separatorFrame = CGRect(x:20.0, y:0.0, width:335.0, height:1.0)
        self.separatorView = UIView(frame: separatorFrame)
        // from #111e35  to #233d53
        self.separatorView!.backgroundColor = UIColor(hex: "ffffff", alpha: 0.3)
        self.addSubview(self.separatorView!)
        

//        var iconFrame = CGRect(x:20.0, y:32.0, width:60.0, height:60.0)
//        self.iconView = UIImageView(frame: iconFrame)
//        self.addSubview(self.iconView!)
        
//        var titleFrame = CGRect(x:108, y:44.0, width:250.0, height:36.0)
        var titleFrame = CGRect(x:20, y:44.0, width:300.0, height:36.0)
        self.titleView = UILabel(frame: titleFrame)
        self.titleView!.font = UIFont.systemFontOfSize(24.0)
        self.titleView!.textColor = UIColor.whiteColor()
        self.addSubview(self.titleView!)
    }

    required init(coder decoder: NSCoder) {
        super.init(coder: decoder)
    }


    override func prepareForReuse() {
//        NSLog("prepareForReuse")
    }

    override func setupCell(object: CollectionDataObject) {
        if let obj = object as? SettingsDataObject {
            self.titleView!.text = obj.name
        }
    }
}



class SettingsView: UIView {

    var gradientLayer: CAGradientLayer = CAGradientLayer()
    
    var settingsDataSource: SettingsDataSource?
    var settingsLayout: SettingsLayout?
    var settingsView: CollectionView?

    var titleView: UILabel?


    required init(coder decoder: NSCoder) {
        super.init(coder: decoder)
    }

    override init(frame: CGRect) {
        super.init(frame: frame)

        // background gradient
        self.gradientLayer.frame = self.bounds
        self.backgroundColor = UIColor(hex: "ffffff", alpha: 0.0)
        let color1 = UIColor(hex: "10182c").CGColor as CGColorRef
        let color2 = UIColor(hex: "26363c").CGColor as CGColorRef
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

