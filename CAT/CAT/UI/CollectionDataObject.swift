//
//  CollectionDataObject.swift
//  CAT
//
//  Created by Kevin Lovette on 8/31/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit


class CollectionDataObject {
    var reuseIdentifier: String
    var indexPath: NSIndexPath
    var layoutAttributes: UICollectionViewLayoutAttributes

    init(reuseIdentifier identifier: String, indexPath path: NSIndexPath) {
        self.reuseIdentifier = identifier
        self.indexPath = path
        self.layoutAttributes = UICollectionViewLayoutAttributes(forCellWithIndexPath: self.indexPath)
    }

    func size() -> CGSize {
        return CGSize(width: 167.0, height: 298.0)
    }
    
    func selectItem() {
        NSLog("didSelectItem: \(self.indexPath)")
    }
}
