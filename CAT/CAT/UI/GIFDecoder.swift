//
//  GIFDecoder.swift
//  CAT
//
//  Created by Kevin Lovette on 7/18/15.
//  Copyright (c) 2015 Kevin Lovette. All rights reserved.
//

import UIKit


//
// Decoder
//
class GIFDecoder {
    var debug = false

    var stream: GIFStream

    init(data: NSData) {
        self.stream = GIFStream(data: data)
    }


    enum DecoderStatus {
        /// File read status: No errors.
        case Ok
        case FormatError
        case OpenError
    }

    
    var status = DecoderStatus.Ok

    /// Returns true if an error was encountered during reading/decoding
    func err() -> Bool {
        return status != DecoderStatus.Ok
    }


    /// Frame data
    var loopCount = 1            // iterations; 0 = repeat forever

    var frameCount = 0
    var frames = [Int: GIFFrame]()    // frames read from current file

    /// Logical Screen Descriptor
    var width = 0        // full image width
    var height = 0       // full image height
    var gctFlag = false  // global color table used
    var gctSize = 0      // size of global color table
    var bgIndex = 0      // background color index
    var pixelAspect = 0  // pixel aspect ratio

    /// Block buffer
    var blockSize = 0                        // block size
    var blockBuffer = NSMutableData(length: 256)   // current data block

    /// Color Table data
    var bgColor: GIFColor?       // background color
    var gct: [GIFColor]?         // global color table

    var lctFlag = false          // local color table flag
    var interlace = false        // interlace flag
    var lctSize = 0              // local color table size
    var lct: [GIFColor]?         // local color table

    var act: [GIFColor]?         // active color table



    /// Gets the number of frames read from file.
    /// @return frame count
    func getFrameCount() -> Int {
        return self.frameCount
    }

    /// Gets the "Netscape" iteration count, if any.
    /// A count of 0 means repeat indefinitiely.
    ///
    /// @return iteration count if one was specified, else 1.
    func getLoopCount() -> Int {
        return self.loopCount
    }

    /// Gets image size.
    ///
    /// @return GIF image dimensions
    func getFrameSize() -> GIFDimension {
        return GIFDimension(width: self.width, height: self.height)
    }


/*
    /// Gets display duration for specified frame.
    ///
    /// @param n int index of frame
    /// @return delay in milliseconds
    public int getDelay(int n) {
        //
        delay = -1
        if ((n >= 0) && (n < frameCount)) {
            delay = ((GifFrame) frames.get(n)).delay
        }
        return delay
    }

    /// Gets the first (or only) image read.
    ///
    /// @return BufferedImage containing first frame, or null if none.
    public BufferedImage getImage() {
        return getFrame(0)
    }
*/

    /// Gets the image contents of frame n.
    ///
    /// @return BufferedImage representation of frame, or null if n is invalid.
    func getFrame(n: Int) -> GIFFrame? {
        var frame: GIFFrame?
        if ( (n >= 0) && (n < frameCount) ) {
            frame = frames[n]
        }
        if frame == nil {
            if blockIndexes.count == 0 {
                self.scan()
            }
            frame = readFrame(n)
            frames[n] = frame    // add image to frame list
        }
        return frame
    }


    /// Initializes or re-initializes reader
    func reset() {
        self.stream.reset()
        
        self.status = DecoderStatus.Ok
        self.frameCount = 0
        self.frames = [Int: GIFFrame]()
        self.gct = nil
        self.lct = [GIFColor]()
    }


    /// Frame data
    var lastDispose = 0
    var rect = GIFRect()      // current image rectangle
    var lastRect = GIFRect()  // last image rect

    var image: GIFFrame?       // current frame
    var lastImage: GIFFrame?   // previous frame

    var lastBgColor: GIFColor? // previous bg color

    // last graphic control extension info
    var dispose = 0
    // 0=no action; 1=leave in place; 2=restore to bg; 3=restore to prev
    var transparency = false  // use transparent color
    var delay = 0             // delay in milliseconds

    var transIndex = 0        // transparent color index



    
    /// Resets frame state for reading next image.
    func resetFrame() {
        self.lastDispose = dispose
        self.lastRect = GIFRect(rect: rect)
        self.lastImage = self.image
        self.lastBgColor = bgColor
        //int dispose = 0
        //boolean transparency = false
        //int delay = 0
        self.dispose = 0
        self.transparency = false
        self.delay = 0
        self.lct = [GIFColor]()
    }

   
    struct GIFBlockIndex {
        var pos = 0
        var code: UInt8 = 0
        var subcode: UInt8 = 0

        init(pos: Int) {
            self.pos = pos
        }
    }
    
    var blockIndexes = [GIFBlockIndex]()

    func frameIndex(n: Int) -> GIFBlockIndex? {
        var count = 0
        for blockIndex in blockIndexes {
            if ( (blockIndex.code == 0x21) && (blockIndex.subcode == 0xf9) ) {
                // graphics control extension
                if count == n {
                    return blockIndex
                }
                count++
            }
        }
        return nil
    }

    func readFrame(n: Int) -> GIFFrame? {
        if let blockIndex = frameIndex(n) {
            // move the start of the graphics control extension
            stream.skip(blockIndex.pos)

            //NSLog("frame: \(n)  pos: \(blockIndex.pos)   stream: \(stream.pos)")

            // read graphics control extension
            stream.readUInt8()
            stream.readUInt8()
            readGraphicControlExt()
            // read frame
            stream.readUInt8()
            return readImage()
        }
        return nil
    }

    
    func scan() {
        reset()
        readHeader()

        //NSLog("width: \(self.width)  height: \(self.height)  frames: \(self.frameCount)")

        if !err() {
            scanContents()
            if frameCount < 0 {
                self.status = DecoderStatus.FormatError
            }
        } else {
                self.status = DecoderStatus.OpenError
        }

        stream.reset()

        self.lastDispose = 0
        self.lastRect = GIFRect()
        self.lastImage = nil
        self.lastBgColor = nil
        //int dispose = 0
        //boolean transparency = false
        //int delay = 0
        self.dispose = 0
        self.transparency = false
        self.delay = 0
        self.lct = [GIFColor]()
        self.status = DecoderStatus.Ok
    }

    /// Main file parser.  Indexes GIF content blocks.
    func scanContents() {
        // read GIF file content blocks
        var done = false

        while !( done || err() ) {
            var blockIndex = GIFBlockIndex(pos: stream.pos)
            
            var code = stream.readUInt8()
            blockIndex.code = code

            switch code {
            case 0x2C:
                // image separator
                scanImage()

            case 0x21: 
                // extension
                code = stream.readUInt8()
                blockIndex.subcode = code
                
                scanBlocks()
                
            case 0x3b:
                // terminator
                done = true
                
            case 0x00:
                // bad byte, but keep going and see what happens
                break

            default:
                self.status = DecoderStatus.FormatError
            }

            //NSLog("frame: \(self.frameCount)  pos: \(blockIndex.pos)  code: \(String(blockIndex.code, radix: 16))  subcode: \(String(blockIndex.subcode, radix: 16))")
            blockIndexes.append(blockIndex)
        }
    }
    
    /// Skips variable length blocks up to and including
    /// next zero length block.
    func scanBlock() -> Int {
        let size = Int(stream.readUInt8())
        stream.skip(size)
        return size
    }
    
    /// Skips variable length blocks up to and including
    /// next zero length block.
    func scanBlocks() {
        var size = scanBlock()
        while ( (size > 0) && !err() ) {
            size = scanBlock()
        }
    }
    
    /// Scan next frame image
    func scanImage() {
        stream.skip(8)   // (sub)image position & size

        let packed = stream.readUInt8()
        self.lctFlag = ( (packed & 0x80) != 0 )     // 1 - local color table flag
        self.interlace = ( (packed & 0x40) != 0 )   // 2 - interlace flag
        // 3 - sort flag
        // 4-5 - reserved
        self.lctSize = ( 1 << Int( (packed & 0x07) + 1 ) )  // 6-8 - local color table size
        
        if self.lctFlag {
            let nbytes = ( 3 * self.lctSize )
            stream.skip(nbytes)
        }

        stream.skip(1)     // LZW code size
        self.scanBlocks()  // skip pixel data

        self.frameCount++

        self.resetFrame()
    }


    
    //
    // Read the GIF file and create all the frames
    //
    func read() {
        reset()
        readHeader()
        if !err() {
            readContents()
            if frameCount < 0 {
                self.status = DecoderStatus.FormatError
            }
        } else {
                self.status = DecoderStatus.OpenError
        }
        //return status
    }

    /// Main file parser.  Reads GIF content blocks.
    func readContents() {
        // read GIF file content blocks
        var done = false

        while !( done || err() ) {
//            let pos = stream.pos

            var code = stream.readUInt8()
            
            switch code {
            case 0x2C:
                // image separator
                let frame = readImage()
                //NSLog("add frame  \(self.frameCount-1)")
                frames[self.frameCount - 1] = frame // add image to frame list

            case 0x21: 
                // extension
                code = stream.readUInt8()
                switch code {
                case 0xf9:
                    // graphics control extension
                    readGraphicControlExt()

                case 0xff:
                    // application extension
                    readBlock()
                    let ptr = UnsafePointer<UInt8>(self.blockBuffer!.mutableBytes)
                    if let str = NSString(bytes: ptr, length: self.blockSize, encoding: NSASCIIStringEncoding) {
                        // check for "NETSCAPE2.0"
                        if str == "NETSCAPE2.0" {
                            readNetscapeExt()
                        } else {
                            skipBlocks()
                        }
                    }

                default:
                    // uninteresting extension
                    skipBlocks()
                }
                
            case 0x3b:
                // terminator
                done = true
                
            case 0x00:
                // bad byte, but keep going and see what happens
                break
                
            default:
                self.status = DecoderStatus.FormatError
            }
        }
    }

    
    /// Reads next variable length block from input.
    ///
    /// @return number of bytes stored in "buffer"
    func readBlock() -> Int {
        self.blockSize = Int(stream.readUInt8())
//        var n = 0
        if self.blockSize > 0 {
            let ptr = UnsafeMutablePointer<UInt8>(self.blockBuffer!.mutableBytes)

            // check if the stram has enough bytes left to read the block  if not set status = DecoderStatus.FormatError
            stream.readBytes(ptr, length: self.blockSize)
        }
        return self.blockSize
    }

    /// Skips variable length blocks up to and including
    /// next zero length block.
    func skipBlocks() {
        readBlock()
        while ( (blockSize > 0) && !err() ) {
              readBlock()
        }
    }
    
    
    /// Reads GIF file header information.
    func readHeader() {
        if let buffer = NSMutableData(length: 6) {
            let ptr = UnsafeMutablePointer<UInt8>(buffer.mutableBytes)
            stream.readBytes(ptr, length: 6)
            if let str = NSString(bytes: ptr, length: 6, encoding: NSASCIIStringEncoding) {
                // first three bytes must be "GIF"
                if !str.hasPrefix("GIF") {
                    status = DecoderStatus.FormatError
                    return
                }
            } else {
                status = DecoderStatus.FormatError
                return
            }

            readLSD()
            if self.gctFlag && !err() {
                self.gct = self.readColorTable(self.gctSize)
                if let colorTable = self.gct {
                    self.bgColor = colorTable[self.bgIndex]
                }
            }
        }
    }

    /// Reads Logical Screen Descriptor
    func readLSD() {
        // logical screen size
        self.width = Int(stream.readUInt16())
        self.height = Int(stream.readUInt16())

        // packed fields
        let packed = stream.readUInt8()
        self.gctFlag = (packed & 0x80) != 0         // 1   : global color table flag
        // 2-4 : color resolution
        // 5   : gct sort flag
        self.gctSize = ( 1 << Int( (packed & 0x07) + 1 ) )  // 6-8 : gct size

        self.bgIndex = Int(stream.readUInt8())      // background color index
        self.pixelAspect = Int(stream.readUInt8())  // pixel aspect ratio
    }

    /// Reads color table as 256 RGB integer values
    ///
    /// @param ncolors int number of colors to read
    /// @return int array containing 256 colors (packed ARGB with full alpha)
    func readColorTable(ncolors: Int) -> [GIFColor]? {
        let nbytes = ( 3 * ncolors )

        if let buffer = NSMutableData(length: nbytes) {
            let ptr = UnsafeMutablePointer<UInt8>(buffer.mutableBytes)
            // check if the stram has enough bytes left to read the block  if not set status = DecoderStatus.FormatError
            stream.readBytes(ptr, length: buffer.length)

            // create colormap
            var colorMap = [GIFColor]()

            let bytes = UnsafeBufferPointer<UInt8>(start: ptr, count: buffer.length)
            for index in 0 ..< ncolors {
                let color = GIFColor(red: bytes[(index*3)], green: bytes[(index*3)+1], blue: bytes[(index*3)+2])
                colorMap.append(color)
            }
            return colorMap
        }
        return nil
    }

    /// Reads Graphics Control Extension values
    func readGraphicControlExt() {
        stream.readUInt8()          // block size
        let packed = stream.readUInt8()              // packed fields
        self.dispose = Int( (packed & 0x1c) >> 2 )   // disposal method
        if  self.dispose == 0 {
            self.dispose = 1                         // elect to keep old image if discretionary
        }
        self.transparency = ( (packed & 1) != 0 )
        self.delay = Int(stream.readUInt16() * 10)   // delay in milliseconds
        self.transIndex = Int(stream.readUInt8())    // transparent color index
        stream.readUInt8()          // block terminator
    }

    /// Reads next frame image
    func readImage() -> GIFFrame? {
        rect.x = Int(stream.readUInt16())  // (sub)image position & size
        rect.y = Int(stream.readUInt16())
        rect.width = Int(stream.readUInt16())
        rect.height = Int(stream.readUInt16())

        let packed = stream.readUInt8()
        self.lctFlag = ( (packed & 0x80) != 0 )     // 1 - local color table flag
        self.interlace = ( (packed & 0x40) != 0 )   // 2 - interlace flag
        // 3 - sort flag
        // 4-5 - reserved
        self.lctSize = ( 1 << Int( (packed & 0x07) + 1 ) )  // 6-8 - local color table size
        
        if self.lctFlag {
            self.lct = readColorTable(self.lctSize)      // read table
            self.act = self.lct                     // make local table active
        } else {
            self.act = self.gct                     // make global table active
            if self.bgIndex == self.transIndex {
                bgColor = GIFColor.transparentColor()
            }
        }
        var save: GIFColor?
        if self.transparency {
            save = self.act![self.transIndex]
            self.act![self.transIndex] = GIFColor.transparentColor()     // set transparent color if specified
        }

        if self.act == nil {
            self.status = DecoderStatus.FormatError // no color table defined
        }

        if self.err() {
            return nil
        }

        decodeImageData() // decode pixel data
        self.skipBlocks()

        if self.err() {
            return nil
        }

        if self.debug {
            NSLog("frame \(frameCount)  transparency: \(self.transparency)  transIndex: \(self.transIndex)  save: \(save)  lastBgColor: \(self.lastBgColor)")
        }
        
        self.frameCount++

        // create new image to receive frame data
//        image = new BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB_PRE)

        var frame: GIFFrame?

        let frameBuffer = setPixels() // transfer pixel data to image

        if let buffer = frameBuffer {
            frame = GIFFrame(pixels: self.pixelData(), frame: buffer, width: self.width, height: self.height, delay: delay)
        }

        if self.transparency {
            self.act![self.transIndex] = save!
        }

        self.resetFrame()

        return frame
    }

    /// Reads Netscape extenstion to obtain iteration count
    func readNetscapeExt() {
        readBlock()
        while ( (blockSize > 0) && !err() ) {
            let ptr = UnsafeMutablePointer<UInt8>(self.blockBuffer!.mutableBytes)            
            let bytes = UnsafeBufferPointer<UInt8>(start: ptr, count: self.blockSize)
            
            if bytes[0] == UInt8(1) {
                // loop count sub-block
                self.loopCount = Int( ( ( UInt16(bytes[2]) & 0xff ) << 8 ) | ( UInt16(bytes[1]) & 0xff ) )
            }

            readBlock()
        } 
    }



    //--------------------------------------------------------------------------------
    //
    // Decoder state
    //


    /// Creates new frame image from current data (and previous
    /// frames as specified by their disposition codes).
    func setPixels() -> NSData? {
        if let frameBuffer = NSMutableData(length: ((self.width * self.height) * 4)) {
            let ptr = UnsafeMutablePointer<UInt8>(frameBuffer.mutableBytes)

            // expose destination image's pixels as int array
            let dest = UnsafeMutableBufferPointer<UInt8>(start: ptr, count: frameBuffer.length)

            //int[] dest = ((DataBufferInt) image.getRaster().getDataBuffer()).getData()

            if self.debug {
                NSLog("  setPixels \(self.frameCount)  lastDispose: \(self.lastDispose)")
            }
            
            // fill in starting image contents based on last image's dispose code
            if self.lastDispose > 0 {
                if self.lastDispose == 3 {
                    // use image before last
                    let n = frameCount - 2
                    if n > 0 {
                        lastImage = self.getFrame(n - 1)
                    } else {
                        lastImage = nil
                    }
                }
                
                if let lastImage = self.lastImage {
                    // copy last frame pixel data
                    if let lastFrameBuffer = lastImage.frameData {
                        let lastPtr = UnsafePointer<UInt8>(lastFrameBuffer.bytes)
                        let prev = UnsafeBufferPointer<UInt8>(start: lastPtr, count: lastFrameBuffer.length)
                        // int[] prev = ((DataBufferInt) lastImage.getRaster().getDataBuffer()).getData()

                        // copy pixels
                        for i in 0 ..< ( (self.width * self.height) * 4) {
                            dest[i] = prev[i]
                        }
                    }

                    if self.lastDispose == 2 {
                        // fill last image rect area with background color
                        var color: GIFColor? = lastBgColor         // use given background color
                        if self.transparency {
                            color = GIFColor.transparentColor()   // assume background is transparent
                        }

                        if let color = color {
                            // fill dest with color
                            var pos = 0
//                            for i in 0 ..< (self.width * self.height) {
                            for _ in 0 ..< (self.width * self.height) {
                                dest[pos+0] = color.red
                                dest[pos+1] = color.green
                                dest[pos+2] = color.blue
                                dest[pos+3] = color.alpha
                                pos += 4
                            }
                        }
                    }
                }
            }


            // copy each source line to the appropriate place in the destination
            var pass = 1
            var inc = 8
            var iline = 0

            for i in 0 ..< self.rect.height {
                var line = i
                if self.interlace {
                    if iline >= self.rect.height {
                        pass++
                        switch pass {
                        case 2:
                            iline = 4
                        case 3:
                            iline = 2
                            inc = 4
                        case 4:
                            iline = 1
                            inc = 2
                        default:
                            break
                        }
                    }
                    line = iline
                    iline += inc
                }

                line += self.rect.y

/*
                if self.debug {
                    NSLog("  line: \(line)  pass: \(pass)  inc: \(inc)  iline: \(iline)  iy: \(self.rect.y)     rect: \(self.rect)")

                    var str = ""
                    for col in 0 ..< self.width {
                        let offset = ( ( (line * self.width) + col ) * 4 )
                        let r = dest[offset]
                        let g = dest[offset+1]
                        let b = dest[offset+2]
                        let a = dest[offset+3]
                        var rhex = String(r, radix: 16)
                        if count(rhex) == 1 {
                            rhex = ( "0" + rhex )
                        }
                        var ghex = String(g, radix: 16)
                        if count(ghex) == 1 {
                            ghex = ( "0" + ghex )
                        }
                        var bhex = String(b, radix: 16)
                        if count(bhex) == 1 {
                            bhex = ( "0" + bhex )
                        }
                        var ahex = String(a, radix: 16)
                        if count(ahex) == 1 {
                            ahex = ( "0" + ahex )
                        }
                        str += (" " + rhex + ghex + bhex + ahex )
                    }
                    var rs = ( "\(line)" )
                    if count(rs) == 1 {
                        rs = ( "0" + rs )
                    }
                    NSLog("[\(rs)]  \(str)")
                }
*/
                
                if line < self.height {
                    let k = line * self.width
                    var dx = k + self.rect.x            // start of line in dest
                    var dlim = dx + self.rect.width     // end of dest line
                    if (k + self.width) < dlim {
                        dlim = k + self.width           // past dest edge
                    }
                    var sx = i * self.rect.width        // start of line in source
                    while dx < dlim {
                        // map color and insert in destination
                        let index = Int( self.pixels[sx++] & 0xff)
                        let color = self.act![index]

//                        let destColor = GIFColor(red: dest[(dx * 4) + 0], green: dest[(dx * 4) + 1], blue: dest[(dx * 4) + 2], alpha: dest[(dx * 4) + 3])

                        // check color != 0
                        if color.aRGB() != 0 {
                            dest[(dx * 4) + 0] = color.red
                            dest[(dx * 4) + 1] = color.green
                            dest[(dx * 4) + 2] = color.blue
                            dest[(dx * 4) + 3] = color.alpha
                        }

/*
                        if self.debug {
                            var hex = String(index, radix: 16)
                            if count(hex) == 1 {
                                hex = ( "0" + hex )
                            }
                            let dc = GIFColor(red: dest[(dx * 4) + 0], green: dest[(dx * 4) + 1], blue: dest[(dx * 4) + 2], alpha: dest[(dx * 4) + 3])
                            NSLog("    dx: \(dx)  dlim: \(dlim)  line: \(line)  sx: \(sx)  index: \(hex)  color: \(color)  dest: \(dc)  destColor: \(destColor)")
                        }
*/
                        dx++
                    }
                }
            }

            self.image = GIFFrame(pixels: self.pixelData(), frame: frameBuffer, width: self.width, height: self.height, delay: delay)

            return frameBuffer
        }
        return nil
    }


    func pixelData() -> NSData? {
        // copy pixel data
        if self.debug {
            NSLog("pixelData  frame: \(self.frameCount)  width: \(self.width)  height: \(self.height)  pixels size: \(self.pixels.count)   rect: \(self.rect)  last: \(self.lastRect)")
        }

        if let indexBuffer = NSMutableData(length: (self.width * self.height)) {
            let indexPtr = UnsafeMutablePointer<UInt8>(indexBuffer.mutableBytes)
            let indexBytes = UnsafeMutableBufferPointer<UInt8>(start: indexPtr, count: indexBuffer.length)
            // copy pixel index data
            for row in 0 ..< self.height {
                for col in 0 ..< self.width {
                    let offset = ( (row * self.height) + col )
                    // is the row col inside 
                    if self.rect.pointInside(col, y: row) {
                        let i = ( ((row - self.rect.y) * self.rect.width) + (col - self.rect.x) )
                        indexBytes[offset] = self.pixels[i]
                    } else {
                        indexBytes[offset] = 0x00
                    }
                }
            }
            return indexBuffer
        }
        return nil
    }
                   

    //--------------------------------------------------------------------------------
    //
    // LWZ decoder
    //

    // max decoder pixel stack size
    static let MaxStackSize = 4096

    // LZW decoder working arrays
    var prefix = [UInt16](count: GIFDecoder.MaxStackSize, repeatedValue: UInt16(0))
    var suffix = [UInt8](count: GIFDecoder.MaxStackSize, repeatedValue: UInt8(0))
    var pixelStack = [UInt8](count: (GIFDecoder.MaxStackSize + 1), repeatedValue: UInt8(0))
    var pixels = [UInt8]()

    /// Decodes LZW image data into pixel array.
    /// Adapted from John Cristy's ImageMagick.
    func decodeImageData() {
        let NullCode = -1
        let npix = (self.rect.width * self.rect.height)

        pixels = [UInt8](count: npix, repeatedValue: UInt8(0))
//        if ((pixels == null) || (pixels.length < npix)) {
//            pixels = new byte[npix]; // allocate new pixel array
//        }
//        if (prefix == null) prefix = new short[MaxStackSize];
//        if (suffix == null) suffix = new byte[MaxStackSize];
//        if (pixelStack == null) pixelStack = new byte[MaxStackSize + 1];

        //  Initialize GIF data stream decoder.

        let data_size = Int(stream.readUInt8())
        let clear = ( 1 << data_size )
        let end_of_information = ( clear + 1 )
        var available = ( clear + 2 )
        var old_code = NullCode
        var code_size = ( data_size + 1 )
        var code_mask = ( (1 << code_size) - 1 )
        
        for codeIndex in 0 ..< clear {
            prefix[codeIndex] = UInt16(0)
            suffix[codeIndex] = UInt8(codeIndex)
        }

        //  Decode GIF pixel stream.

        var datum = 0
        var bits = 0
        var count = 0
        var first = 0
        var top = 0
        var pi = 0
        var bi = 0

        var code = 0
        var in_code = 0
        var i = 0

        // block data
        var ptr = UnsafeMutablePointer<UInt8>(self.blockBuffer!.mutableBytes)
        var bytes = UnsafeBufferPointer<UInt8>(start: ptr, count: self.blockSize)

        while ( i < npix ) {
            if top == 0 {
                if bits < code_size {
                    //  Load bytes until there are enough bits for a code.
                    if count == 0 {
                        // Read a new data block.
                        count = self.readBlock()
                        ptr = UnsafeMutablePointer<UInt8>(self.blockBuffer!.mutableBytes)
                        bytes = UnsafeBufferPointer<UInt8>(start: ptr, count: self.blockSize)
                        if count <= 0 {
                            break
                        }
                        bi = 0
                    }
                    datum += Int( (UInt16(bytes[bi]) & UInt16(0xff)) << UInt16(bits) ) 
                    bits += 8
                    bi++
                    count--
                    continue
                }
                
                //  Get the next code.

                code = ( datum & code_mask )
                datum >>= code_size
                bits -= code_size

                //  Interpret the code

                if ((code > available) || (code == end_of_information)) {
                    break
                }
                
                if (code == clear) {
                    //  Reset decoder.
                    code_size = data_size + 1
                    code_mask = (1 << code_size) - 1
                    available = clear + 2
                    old_code = NullCode
                    continue
                }
                
                if (old_code == NullCode) {
                    pixelStack[top++] = suffix[code]
                    old_code = code
                    first = code
                    continue
                }
                
                in_code = code
                if (code == available) {
                    pixelStack[top++] = UInt8(first)
                    code = old_code
                }
                
                while (code > clear) {
                    pixelStack[top++] = suffix[code]
                    code = Int(prefix[code])
                }
                
                first = Int( suffix[code] & 0xff )

                //  Add a new string to the string table,

                if (available >= GIFDecoder.MaxStackSize) {
                    pixelStack[top++] = UInt8(first)
                    continue
                }
                pixelStack[top++] = UInt8(first)
                prefix[available] = UInt16(old_code)
                suffix[available] = UInt8(first)
                available++

                if ( (available & code_mask) == 0 ) && (available < GIFDecoder.MaxStackSize) {
                    code_size++
                    code_mask += available
                }
                old_code = in_code
            }

            //  Pop a pixel off the pixel stack.

            top--
            pixels[pi++] = pixelStack[top]
            i++
        }

        for i in pi ..< npix {
            pixels[i] = UInt8(0) // clear missing pixels
        }

    }

}
