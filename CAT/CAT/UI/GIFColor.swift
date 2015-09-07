//
//  GIFColor.swift
//  CAT
//
//  Created by Kevin Lovette on 7/18/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit
import CoreBluetooth


class GIFColor {
    var red: UInt8 = 0
    var green: UInt8 = 0
    var blue: UInt8 = 0
    var alpha: UInt8 = 255

    init(red: UInt8, green: UInt8, blue: UInt8, alpha: UInt8 = 255) {
        self.red = red
        self.green = green
        self.blue = blue
        self.alpha = alpha
    }

    class func transparentColor() -> GIFColor {
        return GIFColor(red: 0, green: 0, blue: 0, alpha: 0)
    }


    func aRGB() -> UInt32 {
        var pixel: UInt32 = 0
        pixel |= ( UInt32( self.alpha & 0xff ) << 24 )
        pixel |= ( UInt32( self.red & 0xff ) << 16 )
        pixel |= ( UInt32( self.green & 0xff ) << 8 )
        pixel |= UInt32( self.blue & 0xff )
        return pixel
    }

    func rgb565() -> UInt16 {
        var pixel: UInt16 = 0
        var r = ( ( self.red >> 3 ) & 0x1f )
        var g = ( ( self.green >> 2 ) & 0x3f )
        var b = ( ( self.blue >> 3 ) & 0x1f )
        pixel |= ( ( UInt16( self.red & 0xf8 ) >> 3 ) << 11 )
        pixel |= ( ( UInt16( self.green & 0xf8 ) >> 2 ) << 5 )
        pixel |= ( UInt16( self.blue & 0xf8 ) >> 3 )
        return pixel
    }
}



extension GIFColor: Printable {
    var description: String {
        var rhex = String(red, radix: 16)
        if count(rhex) == 1 {
            rhex = ( "0" + rhex )
        }
        var ghex = String(green, radix: 16)
        if count(ghex) == 1 {
            ghex = ( "0" + ghex )
        }
        var bhex = String(blue, radix: 16)
        if count(bhex) == 1 {
            bhex = ( "0" + bhex )
        }
        var ahex = String(alpha, radix: 16)
        if count(ahex) == 1 {
            ahex = ( "0" + ahex )
        }

        return ( rhex + ghex + bhex + ahex )
    }
}
