//
//  CollectionView.swift
//  CAT
//
//  Created by Kevin Lovette on 8/31/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit


class CollectionView: UICollectionView, UICollectionViewDataSource, UICollectionViewDelegate {

    var collectionDataSource: CollectionDataSource
    var collectionLayout: CollectionViewLayout


    convenience init(frame: CGRect, dataSource: CollectionDataSource) {
        let layout = CollectionViewLayout(dataSource: dataSource)
        self.init(frame: frame, dataSource: dataSource, layout: layout)
    }
    
    init(frame: CGRect, dataSource: CollectionDataSource, layout: CollectionViewLayout) {
        self.collectionDataSource = dataSource
        self.collectionLayout = layout

        super.init(frame: frame, collectionViewLayout: self.collectionLayout)

        registerCells()

        self.dataSource = self
        self.delegate = self
    }
    
    required init(coder decoder: NSCoder) {
        self.collectionDataSource = CollectionDataSource()
        self.collectionLayout = CollectionViewLayout(dataSource: self.collectionDataSource)
        super.init(coder: decoder)
    }


    
    //--------------------------------------------------------------------------------
    //
    // Overrides
    //
    func registerCells() {
    }
    

    //--------------------------------------------------------------------------------
    //
    // UICollectionViewDataSource methods
    //
    func collectionView(collectionView: UICollectionView,
                        numberOfItemsInSection section: Int) -> Int {
        return self.collectionDataSource.numberOfRows(inSection: section)
    }

    func numberOfSectionsInCollectionView(collectionView: UICollectionView) -> Int {
        return self.collectionDataSource.numberOfSections()
    }

    
    func collectionView(collectionView: UICollectionView,
                        cellForItemAtIndexPath indexPath: NSIndexPath) -> UICollectionViewCell {
        let obj = self.collectionDataSource.object(forIndexPath: indexPath)
        // what happens if obj is nil ?
        let cellIdentifier = obj!.reuseIdentifier
        var cell = self.dequeueReusableCellWithReuseIdentifier(cellIdentifier, forIndexPath:indexPath) as! UICollectionViewCell

        if let cell = cell as? CollectionDataObjectCellView {
            // setup cell contents
            cell.setupCell(obj!)
        }
        return cell
    }

//    optional func collectionView(_ collectionView: UICollectionView,
//                                 viewForSupplementaryElementOfKind kind: String,
//                                 atIndexPath indexPath: NSIndexPath) -> UICollectionReusableView {
//        return nil
//    }


    //--------------------------------------------------------------------------------
    //
    // UICollectionViewDataSource methods
    //

    func collectionView(collectionView: UICollectionView, didSelectItemAtIndexPath indexPath: NSIndexPath) {
        if let obj = self.collectionDataSource.object(forIndexPath: indexPath) {
            obj.selectItem()
        }
    }
    
}

