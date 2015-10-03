//
//  CollectionViewLayout.swift
//  CAT
//
//  Created by Kevin Lovette on 8/31/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit


class CollectionViewLayout: UICollectionViewLayout {

    var itemInsets: UIEdgeInsets = UIEdgeInsets(top: 2.0, left: 2.0, bottom: 2.0, right: 2.0)
    var itemSize: CGSize = CGSize(width: 167.0, height: 298.0)
    var interItemSpacingX: CGFloat = 2.0
    var numberOfRows: Int = 1

    var collectionDataSource: CollectionDataSource?


    init(dataSource: CollectionDataSource) {
        super.init()
        self.collectionDataSource = dataSource
    }
    
    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
    }
    
    func setupLayout(object: CollectionDataObject) {
        let x: CGFloat = ( self.itemInsets.left + ( CGFloat(object.indexPath.row) * ( self.itemSize.width + self.interItemSpacingX ) ) )
        let y: CGFloat = ( self.itemInsets.top )
        let frame = CGRect(x: x, y: y, width: self.itemSize.width, height: self.itemSize.height)
        object.layoutAttributes.frame = frame
    }


    //--------------------------------------------------------------------------------
    //
    // UICollectionViewLayout sub-classed methods
    //

    override func prepareLayout() {
        // setup the frame for each cell in the collectionDataSource
        if let source = self.collectionDataSource {
            for section in 0 ... ( source.numberOfSections() - 1 ) {
                for row in 0 ... ( source.numberOfRows(inSection: section) - 1 ) {
                    let indexPath = NSIndexPath(forRow: row, inSection: section)
                    if let obj = source.object(forIndexPath: indexPath) {
                        setupLayout(obj)
                    }
                }
            }
        }
    }

    override func collectionViewContentSize() -> CGSize {
        // return the size of the cells + padding
        let numRows: Int = self.collectionDataSource!.numberOfRows(inSection: 0)
        
        let width: CGFloat = ( self.itemInsets.left + ( CGFloat(numRows) * ( self.itemSize.width + self.interItemSpacingX ) ) + self.itemInsets.right )
        let height: CGFloat = ( self.itemInsets.top + self.itemSize.height )
        return CGSize(width: width, height: height)
    }
    
//    override func layoutAttributesForElementsInRect(rect: CGRect) -> [UICollectionViewLayoutAttributes]? {
    override func layoutAttributesForElementsInRect(rect: CGRect) -> [UICollectionViewLayoutAttributes]? {
        if let objects = self.collectionDataSource?.objects(inRect: rect) {
            var attrs = [UICollectionViewLayoutAttributes]()
            for object in objects {
                attrs.append(object.layoutAttributes)
            }
            return attrs
        }
        return nil
    }

    override func layoutAttributesForItemAtIndexPath(indexPath: NSIndexPath) -> UICollectionViewLayoutAttributes? {
        if let obj = self.collectionDataSource?.object(forIndexPath: indexPath) {
            return obj.layoutAttributes
        }
        return nil
    }

//    func shouldInvalidateLayoutForBoundsChange(_ newBounds: CGRect) -> Bool {
//    }
    
}
