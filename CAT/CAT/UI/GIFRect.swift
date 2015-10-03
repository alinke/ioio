//
//  GIFRect.swift
//  CAT
//
//  Created by Kevin Lovette on 7/18/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit


class GIFRect {
    var x = 0
    var y = 0
    var width = 0
    var height = 0

    convenience init() {
        self.init(x: 0, y: 0, width: 0, height: 0)
    }

    convenience init(rect: GIFRect) {
        self.init(x: rect.x, y: rect.y, width: rect.width, height: rect.height)
    }

    init(x: Int, y: Int, width: Int, height: Int) {
        self.x = x
        self.y = y
        self.width = width
        self.height = height
    }

    func pointInside(x: Int, y: Int) -> Bool {
        return ( ( (y >= self.y) && (y < (self.y + self.height) ) ) &&
                 ( (x >= self.x) && (x < (self.x + self.width) ) ) )
    }
}


class GIFDimension {
    var width = 0
    var height = 0

    init(width: Int, height: Int) {
        self.width = width
        self.height = height
    }
}



extension GIFRect: CustomStringConvertible {
    var description: String {
        return ( "\(self.x),\(self.y) \(self.width)x\(self.height)" )
    }
}
