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



    static let thumbIndex = [
               // "barber_shop":                       6,
               // "diagonals":                         0,
               // "equalizer1":                        0,
               // "equalizer2":                       23,

               "Balloons_Straight":                 0,
               "Jack-O-Lantern":                    0,
               "Kaleidoscope":                      0,
               "RADIAL-COLOR":                      0,
               "Shifting-Lines":                    0,
               "Spiral":                            0,
               "Spiral_colors":                     0,

               "0catandfish16":                    18,
               "0line-game":                       14,
               "Barber_Shop_Pole":                  6,
               "deuces":                           27,
               "Color-Changing-Chevron":            0,
               "EQ_Visualizer_Multi_bar":           0,
               "EQ_Visualizer_Single_Bar":         23,
               "merging-arrows16":                 10,
               "octopus16":                         0,
               "0apaulrgreenmonster16":             0,
               "0apaulrkanji16":                    0,
               "0apaulrkanji216":                   0,
               "0fire16":                           0,
               "0glitter16":                        0,
               "0hermippe16_bw":                    0,
               "0matrix16":                         0,
               "0pacghosts16":                     12,
               "0petit_kitty16":                    0,
               "0rain16":                           0,
               "1adata16":                          0,
               "Angled_100_Perc":                  10,
               "Angled_Color-Change_100_Perc_2":    0,
               "Circles_100_Perc_2":                0,
               "Dueces_Darker_100_Perc":           10,
               "Fleek_100_Perc_2":                 10,
               "Lattice_Color_Change_100_Perc":     0,
               "Trill_Animation_100_perc":          0,
               "arrowfinal16":                      0,
               "bike16":                            0,
               "boat16":                            0,
               "bubbles16":                         0,
               "colortiles16":                      0,
               "firehalf16":                        0,
               "firewhole16":                       0,
               "fliptile16":                        0,
               "float16":                           0,
               "flow16":                            0,
               "fountain16":                        0,
               "gousa":                             0,
               "lines16":                           0,
               "mbutterflies_black16":              0,
               "pauldots16":                        0,
               "paulglasses16":                     0,
               "paulrainbow16":                     0,
               "paulrcircles16":                    0,
               "paulrfallingdrops16":               0,
               "paulrgreenmonser16":                0,
               "paulrnintendo16":                   0,
               "paulrpackages_left16":              0,
               "paulrpackages_right16":             0,
               "paulrpurpleguy":                    0,
               "paulrteeth16":                      0,
               "paulryellowcat16":                  0,
               "paulryellowguy":                    0,
               "paultriangle16":                    0,
               "postman_dog16":                     0,
               "quest16":                           0,
               "rshifter16":                        0,
               "rspray16":                          0,
               "rstarburst16":                      0,
               "sboxerpink16":                      0,
               "shark16":                          10,
               "sjumppink16":                       0,
               "squid16":                           0,
               "triopy16":                          0,
               "usnowy16":                          0,
               "waterflow16":                       0,
               "worm16":                            0,
           ]

    static func thumbnailFrame(name: String) -> Int {
        if let index = thumbIndex[name] {
            return index
        }
        return 0
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
            for (index, thumbnail) in thumbnails.enumerate() {
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
    
    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
    }

    override func setupLayout(object: CollectionDataObject) {
        let row = ( object.indexPath.row / self.numberOfCols )
        let col = ( object.indexPath.row % self.numberOfCols )
        
        let x: CGFloat = ( self.itemInsets.left + ( CGFloat(col) * ( self.itemSize.width + self.interItemSpacingX ) ) )
        let y: CGFloat = ( self.itemInsets.top + ( CGFloat(row) * ( self.itemSize.height + self.interItemSpacingY ) ) )
        let frame = CGRect(x: x, y: y, width: self.itemSize.width, height: self.itemSize.height)
        object.layoutAttributes.frame = frame
    }

    override func collectionViewContentSize() -> CGSize {
        // return the size of the cells + padding
//        var numRows: Int = self.collectionDataSource!.numberOfRows(inSection: 0)

        let width: CGFloat = ( self.itemInsets.left + ( ( CGFloat(self.numberOfCols) * ( self.itemSize.width + self.interItemSpacingX ) ) - self.interItemSpacingX ) + self.itemInsets.right )
        let height: CGFloat = ( self.itemInsets.top + ( CGFloat(self.numberOfRows) * ( self.itemSize.height + self.interItemSpacingY ) ) + self.itemInsets.bottom )

//        Log.info("ThumbnailLayout contentSize  width: \(width)  height: \(height)")
        return CGSize(width: width, height: height)
    }

}


class ThumbnailCellView: CollectionDataObjectCellView {
    var thumbnailView: UIImageView?

    override init(frame aRect: CGRect) {
        super.init(frame: aRect)

//        Log.info("ThumbnailCellView frame: \(frame)")

//        self.backgroundColor = UIColor(hex: "ecf0f1")
        self.backgroundColor = UIColor(hex: "ffffff")

        let thumbnailFrame = CGRect(x:0, y:0.0, width: frame.size.width, height: frame.size.height)
        let imageFrame = CGRectInset(thumbnailFrame, 3, 3)

//        self.thumbnailView = UIImageView(frame: thumbnailFrame)
        self.thumbnailView = UIImageView(frame: imageFrame)
        self.addSubview(self.thumbnailView!)
    }

    required init?(coder decoder: NSCoder) {
        super.init(coder: decoder)
    }


    override func prepareForReuse() {
//        Log.info("ThumbnailCellView: prepareForReuse")
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
