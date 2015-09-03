//
//  CollectionDataSource.swift
//  CAT
//
//  Created by Kevin Lovette on 8/31/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit


class CollectionDataSource {

    // Override this method to setup the section data
    func setupData() {
    }


    var sections: [[CollectionDataObject]] = [[CollectionDataObject]]()

    init() {
        self.setupData()
    }

    /// Asks the data source for the number of sections in the data source.
    /// - Returns: The number of sections in the data source.
    func numberOfSections() -> Int {
        return self.sections.count
    }
    
    /// Asks the data source for the number of rows in a section
    /// - Parameter section: The section to return the number of rows
    /// - Returns: The number of rows in the section
    func numberOfRows(inSection section: Int) -> Int {
        let rows = self.sections[section]
        return rows.count
    }

    func object(forIndexPath indexPath: NSIndexPath) -> CollectionDataObject? {
        if let rows = self.sections[indexPath.section] as [CollectionDataObject]? {
            return rows[indexPath.row]
        }
        return nil
    }

    func objects(inRect rect: CGRect) -> [CollectionDataObject]? {
        var objects: [CollectionDataObject] = [CollectionDataObject]()
        for section in self.sections {
            for row in section {
                if CGRectIntersectsRect(rect, row.layoutAttributes.frame) {
                    objects.append(row)
                }
            }
        }
        return objects
    }
}

