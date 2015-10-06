//
//  Logging.swift
//  CAT
//
//  Created by Kevin Lovette on 10/3/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit


///
/// Logging support
///
class Log {
    /// Enable or disable all logging
    static var loggingEnabled = false

    /// Log if the class is not found
    static var defaultEnabled = true

    /// Log for each class
    static var classEnabled = [
               "App": true,
               "AppDelegate": true,

               "SettingsView": true,
               "SettingsViewController": true,
               "ThumbnailView": true,
               "ThumbnailViewController": true,

               "Device": true,               
               "DeviceManager": true,
               "Protocol": true,
               "Transport": true,
               
               "CollectionDataObject": true,
               "CollectionDataObjectCellView": true,
               "CollectionDataSource": true,
               "CollectionView": true,
               "CollectionViewLayout": true,
               "CollectionViewObjectCellView": true,
               "GIFColor": true,
               "GIFDecoder": true,
               "GIFFrame": true,
               "GIFRect": true,
               "GIFStream": true,
               "Graphics": true,
           ]

    ///
    /// Should a line be logged.
    ///
    /// - Parameters:
    ///     - className: The class name of the object that called the log statement
    ///     - function: The function name that called the log statement
    ///     - line: The line number that called the log statement
    ///
    /// - returns: true if the line should be logged.
    ///
    static func shouldLog(className: String, function: String, line: Int) -> Bool {
        if let enabled = self.classEnabled[className] {
            return enabled
        } else {
            return self.defaultEnabled
        }
    }
    

    ///
    /// Log info
    ///
    /// - Parameters:
    ///     - message The logging message
    ///     - file: The filename that called the log statement
    ///     - function: The function name that called the log statement
    ///     - line: The line number that called the log statement
    ///
    static func info(@autoclosure message: () -> String, file: String = __FILE__, function: String = __FUNCTION__, line: Int = __LINE__) {
        if self.loggingEnabled {
            let className = NSString(string: NSString(string: file).stringByDeletingPathExtension).lastPathComponent
            if self.shouldLog(className, function: function, line: line) {
                let msg = message()
                NSLog("\(className) \(function) \(line)  \(msg)")
            }
        }
    }

}

