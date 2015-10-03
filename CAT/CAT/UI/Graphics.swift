//
//  Graphics.swift
//  CAT
//
//  Created by Kevin Lovette on 7/18/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit


// UIColor
extension UIColor {
    convenience init(hex: String, alpha: CGFloat = 1.0) {
        let scanner = NSScanner(string: hex)
        var color: UInt32 = 0
        scanner.scanHexInt(&color)

        let mask = 0x000000FF
        let r = CGFloat(Float(Int(color >> 16) & mask)/255.0)
        let g = CGFloat(Float(Int(color >> 8) & mask)/255.0)
        let b = CGFloat(Float(Int(color) & mask)/255.0)

        self.init(red: r, green: g, blue: b, alpha: alpha)
    }
}


extension UIGestureRecognizerState {
    public var description: String {
        switch self {
        case UIGestureRecognizerState.Possible:
            return "Possible"
        case UIGestureRecognizerState.Cancelled:
            return "Cancelled"
        case UIGestureRecognizerState.Failed:
            return "Failed"
        case UIGestureRecognizerState.Began:
            return "Began"
        case UIGestureRecognizerState.Changed:
            return "Changed"
        case UIGestureRecognizerState.Ended:
            return "Ended"
        default:
            return "unknown"
        }
    }
}


extension UIViewAutoresizing: CustomStringConvertible {
    public var description: String {
        switch self {
        case UIViewAutoresizing.None:
            return "None"
        case UIViewAutoresizing.FlexibleLeftMargin:
            return "FlexibleLeftMargin"
        case UIViewAutoresizing.FlexibleWidth:
            return "FlexibleWidth"
        case UIViewAutoresizing.FlexibleRightMargin:
            return "FlexibleRightMargin"
        case UIViewAutoresizing.FlexibleTopMargin:
            return "FlexibleTopMargin"
        case UIViewAutoresizing.FlexibleHeight:
            return "FlexibleHeight"
        case UIViewAutoresizing.FlexibleBottomMargin:
            return "FlexibleBottomMargin"
        default:
            return "unknown"
        }
    }
}

