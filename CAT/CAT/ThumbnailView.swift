//
//  ThumbnailView.swift
//  CAT
//
//  Created by Kevin Lovette on 8/31/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit


class Thumbnail {
    var name: String
    var frame: GIFFrame
    
    init(name: String, frame: GIFFrame) {
        self.name = name
        self.frame = frame
    }
    
    func image() -> UIImage? {
        return frame.image()
    }
}


class ThumbnailDataObject: CollectionDataObject {
    var viewController: ThumbnailViewController?
    var thumbnail: Thumbnail?
    
    convenience init(reuseIdentifier identifier: String, indexPath path: NSIndexPath, thumbnail: Thumbnail, viewController: ThumbnailViewController) {
        self.init(reuseIdentifier: identifier, indexPath: path)
        self.thumbnail = thumbnail
        self.viewController = viewController
    }

    override func size() -> CGSize {
        return CGSize(width: 128.0, height: 64.0)
    }

    override func selectItem() {
        if let thumbnail = self.thumbnail {
            if let viewController = self.viewController {
                viewController.thumbnailSelected(thumbnail)
            }
        }
    }
}

class ThumbnailDataSource: CollectionDataSource {
    var viewController: ThumbnailViewController?
    var thumbnails: [Thumbnail]?
    
    init(viewController: ThumbnailViewController, thumbnails: [Thumbnail]) {
        self.thumbnails = thumbnails
        self.viewController = viewController
    }
    
    override func setupData() {
        var section: [ThumbnailDataObject] = [ThumbnailDataObject]()

        if let thumbnails = self.thumbnails {
            for (index, thumbnail) in enumerate(thumbnails) {
                let dataObject = ThumbnailDataObject(reuseIdentifier: "ThumbnailCell", indexPath: NSIndexPath(forRow: index, inSection: 0), thumbnail: thumbnail, viewController: viewController!)
                section.append(dataObject)
            }
        }
        sections.append(section)
    }
}

    
class ThumbnailLayout: CollectionViewLayout {

    var interItemSpacingY: CGFloat = 0
    var numberOfCols = 0
    
    override init(dataSource: CollectionDataSource) {
        super.init(dataSource: dataSource)

        self.numberOfCols = 3
        self.numberOfRows = ( dataSource.numberOfRows(inSection: 0) / self.numberOfCols )

        self.itemInsets = UIEdgeInsets(top: 6.0, left: 6.0, bottom: 6.0, right: 6.0)
        self.interItemSpacingX = 6.0
        self.interItemSpacingY = 6.0
        
        let spaceWidth = ( self.itemInsets.left + ( CGFloat( self.numberOfCols - 1 ) * self.interItemSpacingX ) + self.itemInsets.right )
        let width = ( ( 375.0 - spaceWidth ) / CGFloat(self.numberOfCols) )
        let height = ( width * 0.5 )
        
        self.itemSize = CGSize(width: width, height: height)

        if ( ( dataSource.numberOfRows(inSection: 0) % self.numberOfCols ) != 0 ) {
            self.numberOfRows++
        }
    }
    
    required init(coder decoder: NSCoder) {
        super.init(coder: decoder)
    }

    override func setupLayout(object: CollectionDataObject) {
        let row = ( object.indexPath.row / self.numberOfCols )
        let col = ( object.indexPath.row % self.numberOfCols )
        
        var x: CGFloat = ( self.itemInsets.left + ( CGFloat(col) * ( self.itemSize.width + self.interItemSpacingX ) ) )
        var y: CGFloat = ( self.itemInsets.top + ( CGFloat(row) * ( self.itemSize.height + self.interItemSpacingY ) ) )
        let frame = CGRect(x: x, y: y, width: self.itemSize.width, height: self.itemSize.height)
        object.layoutAttributes.frame = frame
    }

    override func collectionViewContentSize() -> CGSize {
        // return the size of the cells + padding
        var numRows: Int = self.collectionDataSource!.numberOfRows(inSection: 0)

        var width: CGFloat = ( self.itemInsets.left + ( ( CGFloat(self.numberOfCols) * ( self.itemSize.width + self.interItemSpacingX ) ) - self.interItemSpacingX ) + self.itemInsets.right )
        var height: CGFloat = ( self.itemInsets.top + ( CGFloat(self.numberOfRows) * self.itemSize.height ) )

//        NSLog("ThumbnailLayout contentSize  width: \(width)  height: \(height)")
        return CGSize(width: width, height: height)
    }

}


class ThumbnailCellView: CollectionDataObjectCellView {
    var thumbnailView: UIImageView?

    override init(frame aRect: CGRect) {
        super.init(frame: aRect)

//        NSLog("ThumbnailCellView frame: \(frame)")

//        self.backgroundColor = UIColor(hex: "ecf0f1")
        self.backgroundColor = UIColor(hex: "ffffff")

        var thumbnailFrame = CGRect(x:0, y:0.0, width: frame.size.width, height: frame.size.height)
        var imageFrame = CGRectInset(thumbnailFrame, 3, 3)

//        self.thumbnailView = UIImageView(frame: thumbnailFrame)
        self.thumbnailView = UIImageView(frame: imageFrame)
        self.addSubview(self.thumbnailView!)
    }

    required init(coder decoder: NSCoder) {
        super.init(coder: decoder)
    }


    override func prepareForReuse() {
//        NSLog("ThumbnailCellView: prepareForReuse")
    }

    override func setupCell(object: CollectionDataObject) {
        if let obj = object as? ThumbnailDataObject {
            if let thumbnail = obj.thumbnail {
                if let image = thumbnail.image() {
                    self.thumbnailView!.image = image
                }
            }
        }
    }
}
