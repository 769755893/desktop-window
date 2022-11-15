//
//  OriginalPluginRegister.swift
//  desktop_multi_window
//
//  Created by WCJ on 2022/11/15.
//

import Foundation
import FlutterMacOS
import Cocoa

extension NSScreen {
  var displayID: CGDirectDisplayID {
    return deviceDescription[NSDeviceDescriptionKey(rawValue: "NSScreenNumber")] as? CGDirectDisplayID ?? 0
  }
}

public class ScreenRetrieverPluginOriginal: NSObject, FlutterPlugin {
    var registrar: FlutterPluginRegistrar!;
    var channel: FlutterMethodChannel!
    
    public static func register(with registrar: FlutterPluginRegistrar) {
        let channel = FlutterMethodChannel(name: "screen_retriever", binaryMessenger: registrar.messenger)
        let instance = ScreenRetrieverPluginOriginal()
        instance.registrar = registrar
        instance.channel = channel
        registrar.addMethodCallDelegate(instance, channel: channel)
    }
    
    public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
        switch (call.method) {
        case "getCursorScreenPoint":
            getCursorScreenPoint(call, result: result)
            break
        case "getPrimaryDisplay":
            getPrimaryDisplay(call, result: result)
            break
        case "getAllDisplays":
            getAllDisplays(call, result: result)
            break
        default:
            result(FlutterMethodNotImplemented)
        }
    }
    
    public func _screenToDict(_ screen: NSScreen) -> NSDictionary {
        var name: String = "";
        if #available(macOS 10.15, *) {
            name = screen.localizedName
        }
        let size: NSDictionary = [
            "width": screen.frame.width,
            "height": screen.frame.height,
        ]
        let visiblePosition: NSDictionary = [
            "x": screen.visibleFrame.topLeft.x,
            "y": screen.visibleFrame.topLeft.y,
        ]
        let visibleSize: NSDictionary = [
            "width": screen.visibleFrame.width,
            "height": screen.visibleFrame.height,
        ]
        let dict: NSDictionary = [
            "id": screen.displayID,
            "name": name,
            "size": size,
            "visiblePosition": visiblePosition,
            "visibleSize": visibleSize,
        ]
        return dict;
    }
    
    public func getCursorScreenPoint(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
        let screenFrame = NSScreen.main!.frame
        let mouseLocation: NSPoint = NSEvent.mouseLocation;
        
        let resultData: NSDictionary = [
            "x": mouseLocation.x,
            "y": screenFrame.size.height - mouseLocation.y,
        ]
        result(resultData)
    }
    
    public func getPrimaryDisplay(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
        let resultData: NSDictionary = _screenToDict(NSScreen.main!)
        result(resultData)
    }
    
    public func getAllDisplays(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
        let resultData: NSDictionary = [
            "displays": NSScreen.screens.map({ screen in
                return _screenToDict(screen)
            }),
        ]
        result(resultData)
    }
    
    public func _emitEvent(_ eventName: String) {
        let args: NSDictionary = [
            "eventName": eventName,
        ]
        channel.invokeMethod("onEvent", arguments: args, result: nil)
    }
}


public class SharedPreferencesPluginOriginal: NSObject, FlutterPlugin {
  public static func register(with registrar: FlutterPluginRegistrar) {
    let channel = FlutterMethodChannel(
      name: "plugins.flutter.io/shared_preferences_macos",
      binaryMessenger: registrar.messenger)
    let instance = SharedPreferencesPluginOriginal()
    registrar.addMethodCallDelegate(instance, channel: channel)
  }

  public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
    switch call.method {
    case "getAll":
      result(getAllPrefs())
    case "setBool",
         "setInt",
         "setDouble",
         "setString",
         "setStringList":
      let arguments = call.arguments as! [String: Any]
      let key = arguments["key"] as! String
      UserDefaults.standard.set(arguments["value"], forKey: key)
      result(true)
    case "commit":
      // UserDefaults does not need to be synchronized.
      result(true)
    case "remove":
      let arguments = call.arguments as! [String: Any]
      let key = arguments["key"] as! String
      UserDefaults.standard.removeObject(forKey: key)
      result(true)
    case "clear":
      let defaults = UserDefaults.standard
      for (key, _) in getAllPrefs() {
        defaults.removeObject(forKey: key)
      }
      result(true)
    default:
      result(FlutterMethodNotImplemented)
    }
  }
}

/// Returns all preferences stored by this plugin.
private func getAllPrefs() -> [String: Any] {
  var filteredPrefs: [String: Any] = [:]
  if let appDomain = Bundle.main.bundleIdentifier,
    let prefs = UserDefaults.standard.persistentDomain(forName: appDomain)
  {
    for (key, value) in prefs where key.hasPrefix("flutter.") {
      filteredPrefs[key] = value
    }
  }
  return filteredPrefs
}


public class WindowManagerPluginOriginal: NSObject, FlutterPlugin {
    public static var RegisterGeneratedPlugins:((FlutterPluginRegistry) -> Void)?

    public static func register(with registrar: FlutterPluginRegistrar) {
        let channel = FlutterMethodChannel(name: "window_manager", binaryMessenger: registrar.messenger)
        let instance = WindowManagerPluginOriginal(registrar, channel)
        registrar.addMethodCallDelegate(instance, channel: channel)
    }

    private var registrar: FlutterPluginRegistrar!;
    private var channel: FlutterMethodChannel!

    private var mainWindow: NSWindow {
        get {
            return (self.registrar.view?.window)!;
        }
    }

    private var _inited: Bool = false
    private var windowManager: WindowManager = WindowManager()

    public init(_ registrar: FlutterPluginRegistrar, _ channel: FlutterMethodChannel) {
        super.init()
        self.registrar = registrar
        self.channel = channel
    }

    private func ensureInitialized() {
        if (!_inited) {
            windowManager.mainWindow = mainWindow
            windowManager.onEvent = {
                (eventName: String) in
                self._emitEvent(eventName)
            }
            _inited = true
        }
    }

    public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
        let methodName: String = call.method
        let args: [String: Any] = call.arguments as? [String: Any] ?? [:]

        switch (methodName) {
        case "ensureInitialized":
            ensureInitialized()
            result(true)
            break
        case "waitUntilReadyToShow":
            windowManager.waitUntilReadyToShow()
            result(true)
            break
        case "setAsFrameless":
            windowManager.setAsFrameless()
            result(true)
            break
        case "destroy":
            windowManager.destroy()
            result(true)
            break
        case "close":
            windowManager.close()
            result(true)
            break
        case "isPreventClose":
            result(windowManager.isPreventClose())
            break
        case "setPreventClose":
            windowManager.setPreventClose(args)
            result(true)
            break
        case "focus":
            windowManager.focus()
            result(true)
            break
        case "blur":
            windowManager.blur()
            result(true)
            break
        case "isFocused":
            result(windowManager.isFocused())
            break
        case "show":
            windowManager.show()
            result(true)
            break
        case "hide":
            windowManager.hide()
            result(true)
            break
        case "isVisible":
            result(windowManager.isVisible())
            break
        case "isMaximized":
            result(windowManager.isMaximized())
            break
        case "maximize":
            windowManager.maximize()
            result(true)
            break
        case "unmaximize":
            windowManager.unmaximize()
            result(true)
            break
        case "isMinimized":
            result(windowManager.isMinimized())
            break
        case "minimize":
            windowManager.minimize()
            result(true)
            break
        case "restore":
            windowManager.restore()
            result(true)
            break
        case "isFullScreen":
            result(windowManager.isFullScreen())
            break
        case "setFullScreen":
            windowManager.setFullScreen(args)
            result(true)
            break
        case "setAspectRatio":
            windowManager.setAspectRatio(args)
            result(true)
            break
        case "setBackgroundColor":
            windowManager.setBackgroundColor(args)
            result(true)
            break
        case "getBounds":
            result(windowManager.getBounds())
            break
        case "setBounds":
            windowManager.setBounds(args)
            result(true)
            break
        case "setMinimumSize":
            windowManager.setMinimumSize(args)
            result(true)
            break
        case "setMaximumSize":
            windowManager.setMaximumSize(args)
            result(true)
            break
        case "isResizable":
            result(windowManager.isResizable())
            break
        case "setResizable":
            windowManager.setResizable(args)
            result(true)
            break
        case "isMovable":
            result(windowManager.isMovable())
            break
        case "setMovable":
            windowManager.setMovable(args)
            result(true)
            break
        case "isMinimizable":
            result(windowManager.isMinimizable())
            break
        case "setMinimizable":
            windowManager.setMinimizable(args)
            result(true)
            break
        case "isClosable":
            result(windowManager.isClosable())
            break
        case "setClosable":
            windowManager.setClosable(args)
            result(true)
            break
        case "isAlwaysOnTop":
            result(windowManager.isAlwaysOnTop())
            break
        case "setAlwaysOnTop":
            windowManager.setAlwaysOnTop(args)
            result(true)
            break
        case "getTitle":
            result(windowManager.getTitle())
            break
        case "setTitle":
            windowManager.setTitle(args)
            result(true)
            break
        case "setTitleBarStyle":
            windowManager.setTitleBarStyle(args)
            result(true)
            break
        case "getTitleBarHeight":
            result(windowManager.getTitleBarHeight())
            break
        case "isSkipTaskbar":
            result(windowManager.isSkipTaskbar())
            break
        case "setSkipTaskbar":
            windowManager.setSkipTaskbar(args)
            result(true)
            break
        case "setProgressBar":
            windowManager.setProgressBar(args)
            result(true)
            break
        case "hasShadow":
            result(windowManager.hasShadow())
            break
        case "setHasShadow":
            windowManager.setHasShadow(args)
            result(true)
            break
        case "getOpacity":
            result(windowManager.getOpacity())
            break
        case "setOpacity":
            windowManager.setOpacity(args)
            result(true)
            break
        case "setBrightness":
            windowManager.setBrightness(args)
            result(true)
            break
        case "setIgnoreMouseEvents":
            windowManager.setIgnoreMouseEvents(args)
            result(true)
            break
        case "startDragging":
            windowManager.startDragging()
            result(true)
            break
        case "isSubWindow":
            result(windowManager.isSubWindow())
            break
        case "createSubWindow":
            windowManager.createSubWindow(args);
            result(true)
        default:
            result(FlutterMethodNotImplemented)
        }
    }

    public func _emitEvent(_ eventName: String) {
        let args: NSDictionary = [
            "eventName": eventName,
        ]
        channel.invokeMethod("onEvent", arguments: args, result: nil)
    }
}



extension NSWindow {
    private struct AssociatedKeys {
        static var configured: Bool = false
    }
    var configured: Bool {
        get {
            return objc_getAssociatedObject(self, &AssociatedKeys.configured) as? Bool ?? false
        }
        set(value) {
            objc_setAssociatedObject(self, &AssociatedKeys.configured, value, objc_AssociationPolicy.OBJC_ASSOCIATION_RETAIN_NONATOMIC)
        }
    }
    public func hiddenWindowAtLaunch() {
        if (!configured) {
            setIsVisible(false)
            configured = true
        }
    }
}

extension NSRect {
    var topLeft: CGPoint {
        set {
            let screenFrameRect = NSScreen.main!.frame
            origin.x = newValue.x
            origin.y = screenFrameRect.height - newValue.y - size.height
        }
        get {
            let screenFrameRect = NSScreen.main!.frame
            return CGPoint(x: origin.x, y: screenFrameRect.height - origin.y - size.height)
        }
    }
}

public class WindowManager: NSObject, NSWindowDelegate {
    public var onEvent:((String) -> Void)?
    
    private var _mainWindow: NSWindow?
    public var mainWindow: NSWindow {
        get {
            return _mainWindow!
        }
        set {
            _mainWindow = newValue
            _mainWindow?.delegate = self
        }
    }
    
    private var _isPreventClose: Bool = false
    private var _isMaximized: Bool = false
    
    override public init() {
        super.init()
    }
    
    public func waitUntilReadyToShow() {
        // nothing
    }
    
    public func setAsFrameless() {
        mainWindow.styleMask.insert(.fullSizeContentView)
        mainWindow.titleVisibility = .hidden
        mainWindow.isOpaque = true
        mainWindow.hasShadow = false
        mainWindow.backgroundColor = NSColor.clear
        
        if (mainWindow.styleMask.contains(.titled)) {
            let titleBarView: NSView = (mainWindow.standardWindowButton(.closeButton)?.superview)!.superview!
            titleBarView.isHidden = true
        }
    }
    
    public func destroy() {
        NSApp.terminate(nil)
    }
    
    public func close() {
        mainWindow.performClose(nil)
    }
    
    public func isPreventClose() -> Bool {
        return _isPreventClose;
    }
    
    public func setPreventClose(_ args: [String: Any]) {
        _isPreventClose = args["isPreventClose"] as! Bool
    }
    
    public func focus() {
        NSApp.activate(ignoringOtherApps: false)
        mainWindow.makeKeyAndOrderFront(nil)
    }
    
    public func blur() {
        mainWindow.orderBack(nil)
    }
    
    public func isFocused() -> Bool {
        return mainWindow.isKeyWindow
    }
    
    public func show() {
        mainWindow.setIsVisible(true)
        DispatchQueue.main.async {
            self.mainWindow.makeKeyAndOrderFront(nil)
            NSApp.activate(ignoringOtherApps: true)
        }
    }
    
    public func hide() {
        DispatchQueue.main.async {
            self.mainWindow.orderOut(nil)
        }
    }
    
    public func isVisible() -> Bool {
        return mainWindow.isVisible
    }
    
    public func isMaximized() -> Bool {
        return mainWindow.isZoomed
    }
    
    public func maximize() {
        if (!isMaximized()) {
            mainWindow.zoom(nil);
        }
    }
    
    public func unmaximize() {
        if (isMaximized()) {
            mainWindow.zoom(nil);
        }
    }
    
    public func isMinimized() -> Bool {
        return mainWindow.isMiniaturized
    }
    
    public func minimize() {
        mainWindow.miniaturize(nil)
    }
    
    public func restore() {
        mainWindow.deminiaturize(nil)
    }
    
    public func isFullScreen() -> Bool {
        return mainWindow.styleMask.contains(.fullScreen)
    }
    
    public func setFullScreen(_ args: [String: Any]) {
        let isFullScreen: Bool = args["isFullScreen"] as! Bool
        
        if (isFullScreen) {
            if (!mainWindow.styleMask.contains(.fullScreen)) {
                mainWindow.toggleFullScreen(nil)
            }
        } else {
            if (mainWindow.styleMask.contains(.fullScreen)) {
                mainWindow.toggleFullScreen(nil)
            }
        }
    }
    
    public func setAspectRatio(_ args: [String: Any]) {
        let hasFrame = !mainWindow.styleMask.contains(.fullSizeContentView);
        let aspectRatio = (args["aspectRatio"] as! NSNumber).doubleValue
        
        // Reset the behaviour to default if aspect_ratio is set to 0 or less.
        if (aspectRatio > 0.0) {
            let aspectRatioSize: NSSize = NSMakeSize(CGFloat(aspectRatio), 1.0)
            if (hasFrame) {
                mainWindow.contentAspectRatio = aspectRatioSize
            } else {
                mainWindow.aspectRatio = aspectRatioSize;
            }
        } else {
            mainWindow.resizeIncrements = NSMakeSize(1.0, 1.0)
        }
    }
    
    public func setBackgroundColor(_ args: [String: Any]) {
        let backgroundColorA = args["backgroundColorA"] as! Int
        let backgroundColorR = args["backgroundColorR"] as! Int
        let backgroundColorG = args["backgroundColorG"] as! Int
        let backgroundColorB = args["backgroundColorB"] as! Int
        
        let isTransparent: Bool = backgroundColorA == 0
        && backgroundColorR == 0
        && backgroundColorG == 0
        && backgroundColorB == 0;
        
        if (isTransparent) {
            mainWindow.backgroundColor = NSColor.clear
        } else {
            let rgbR = CGFloat(backgroundColorR) / 255
            let rgbG = CGFloat(backgroundColorG) / 255
            let rgbB = CGFloat(backgroundColorB) / 255
            let rgbA = CGFloat(backgroundColorA) / 255
            
            mainWindow.backgroundColor = NSColor(red: rgbR,
                                                 green: rgbG,
                                                 blue: rgbB,
                                                 alpha: rgbA)
        }
    }
    
    public func getBounds() -> NSDictionary {
        let frameRect: NSRect = mainWindow.frame;
        
        let data: NSDictionary = [
            "x": frameRect.topLeft.x,
            "y": frameRect.topLeft.y,
            "width": frameRect.size.width,
            "height": frameRect.size.height,
        ]
        return data;
    }
    
    public func setBounds(_ args: [String: Any]) {
        let animate = args["animate"] as? Bool ?? false
        
        var frameRect = mainWindow.frame
        if (args["width"] != nil && args["height"] != nil) {
            let width: CGFloat = CGFloat(truncating: args["width"] as! NSNumber)
            let height: CGFloat = CGFloat(truncating: args["height"] as! NSNumber)
            
            frameRect.origin.y += (frameRect.size.height - height)
            frameRect.size.width = width
            frameRect.size.height = height
        }
        
        if (args["x"] != nil && args["y"] != nil) {
            frameRect.topLeft.x = CGFloat(args["x"] as! Float)
            frameRect.topLeft.y = CGFloat(args["y"] as! Float)
        }
        
        if (animate) {
            mainWindow.animator().setFrame(frameRect, display: true, animate: true)
        } else {
            mainWindow.setFrame(frameRect, display: true)
        }
    }
    
    public func setMinimumSize(_ args: [String: Any]) {
        let minSize: NSSize = NSSize(
            width: CGFloat(args["width"] as! Float),
            height: CGFloat(args["height"] as! Float)
        )
        mainWindow.minSize = minSize
    }
    
    public func setMaximumSize(_ args: [String: Any]) {
        let maxSize: NSSize = NSSize(
            width: CGFloat(args["width"] as! Float),
            height: CGFloat(args["height"] as! Float)
        )
        mainWindow.maxSize = maxSize
    }
    
    public func isResizable() -> Bool {
        return mainWindow.styleMask.contains(.resizable)
    }
    
    public func setResizable(_ args: [String: Any]) {
        let isResizable: Bool = args["isResizable"] as! Bool
        if (isResizable) {
            mainWindow.styleMask.insert(.resizable)
        } else {
            mainWindow.styleMask.remove(.resizable)
        }
    }
    
    public func isMovable() -> Bool {
        return mainWindow.isMovable
    }
    
    public func setMovable(_ args: [String: Any]) {
        let isMovable: Bool = args["isMovable"] as! Bool
        mainWindow.isMovable = isMovable
    }
    
    public func isMinimizable() -> Bool {
        return mainWindow.styleMask.contains(.miniaturizable)
    }
    
    public func setMinimizable(_ args: [String: Any]) {
        let isMinimizable: Bool = args["isMinimizable"] as! Bool
        if (isMinimizable) {
            mainWindow.styleMask.insert(.miniaturizable)
        } else {
            mainWindow.styleMask.remove(.miniaturizable)
        }
    }
    
    public func isClosable() -> Bool {
        return mainWindow.styleMask.contains(.closable)
    }
    
    public func setClosable(_ args: [String: Any]) {
        let isClosable: Bool = args["isClosable"] as! Bool
        if (isClosable) {
            mainWindow.styleMask.insert(.closable)
        } else {
            mainWindow.styleMask.remove(.closable)
        }
    }
    
    public func isAlwaysOnTop() -> Bool {
        return mainWindow.level == .floating
    }
    
    public func setAlwaysOnTop(_ args: [String: Any]) {
        let isAlwaysOnTop: Bool = args["isAlwaysOnTop"] as! Bool
        mainWindow.level = isAlwaysOnTop ? .floating : .normal
    }
    
    public func getTitle() -> String {
        return mainWindow.title
    }
    
    public func setTitle(_ args: [String: Any]) {
        let title: String = args["title"] as! String
        mainWindow.title = title;
    }
    
    public func setTitleBarStyle(_ args: [String: Any]) {
        let titleBarStyle: String = args["titleBarStyle"] as! String
        let windowButtonVisibility: Bool = args["windowButtonVisibility"] as! Bool
        
        if (titleBarStyle == "hidden") {
            mainWindow.titleVisibility = .hidden
            mainWindow.titlebarAppearsTransparent = true
            mainWindow.styleMask.insert(.fullSizeContentView)
        } else {
            mainWindow.titleVisibility = .visible
            mainWindow.titlebarAppearsTransparent = false
            mainWindow.styleMask.remove(.fullSizeContentView)
        }
        
        mainWindow.standardWindowButton(.closeButton)?.isHidden = !windowButtonVisibility
        mainWindow.standardWindowButton(.miniaturizeButton)?.isHidden = !windowButtonVisibility
        mainWindow.standardWindowButton(.zoomButton)?.isHidden = !windowButtonVisibility
    }
    
    public func getTitleBarHeight() -> Int {
        let frame = mainWindow.frame;
        let windowHeight: CGFloat = mainWindow.frame.height
        return Int(windowHeight - mainWindow.contentRect(forFrameRect: frame).height)
    }
    
    public func isSkipTaskbar() -> Bool {
        return NSApplication.shared.activationPolicy() == .accessory
    }
    
    public func setSkipTaskbar(_ args: [String: Any]) {
        let isSkipTaskbar: Bool = args["isSkipTaskbar"] as! Bool
        NSApplication.shared.setActivationPolicy(isSkipTaskbar ? .accessory : .regular)
    }
    
    public func setProgressBar(_ args: [String: Any]) {
        let progress: CGFloat = CGFloat(truncating: args["progress"] as! NSNumber)
        
        let dockTile: NSDockTile = NSApp.dockTile;
        
        let firstTime = dockTile.contentView == nil || dockTile.contentView?.subviews.count == 0
        
        if (firstTime) {
            let imageView: NSImageView = NSImageView.init()
            imageView.image = NSApp.applicationIconImage
            dockTile.contentView = imageView
            
            let frame: NSRect = NSMakeRect(0.0, 0.0, dockTile.size.width, 15.0)
            let progressIndicator: NSProgressIndicator = NSProgressIndicator.init(frame: frame)
            progressIndicator.style = .bar
            progressIndicator.isIndeterminate = false
            progressIndicator.isBezeled = true
            progressIndicator.minValue = 0
            progressIndicator.maxValue = 1
            progressIndicator.isHidden = false
            dockTile.contentView?.addSubview(progressIndicator)
        }
        
        let progressIndicator: NSProgressIndicator = dockTile.contentView!.subviews.last as! NSProgressIndicator
        if (progress < 0) {
            progressIndicator.isHidden = true
        } else if (progress > 1) {
            progressIndicator.isHidden = false
            progressIndicator.isIndeterminate = true
            progressIndicator.doubleValue = 1
        } else {
            progressIndicator.isHidden = false
            progressIndicator.doubleValue = Double(progress)
        }
        dockTile.display()
    }
    
    public func hasShadow() -> Bool {
        return mainWindow.hasShadow
    }
    
    public func setHasShadow(_ args: [String: Any]) {
        let hasShadow: Bool = args["hasShadow"] as! Bool
        mainWindow.hasShadow = hasShadow;
        mainWindow.invalidateShadow();
    }
    
    public func getOpacity() -> CGFloat {
        return mainWindow.alphaValue
    }
    
    public func setOpacity(_ args: [String: Any]) {
        let opacity: CGFloat = CGFloat(truncating: args["opacity"] as! NSNumber)
        mainWindow.alphaValue = opacity
    }
    
    public func setBrightness(_ args: [String: Any]) {
        let brightness: String = args["brightness"] as! String
        if (brightness == "dark") {
            mainWindow.appearance = NSAppearance(named: NSAppearance.Name.vibrantDark)
        } else {
            mainWindow.appearance = NSAppearance(named: NSAppearance.Name.vibrantLight)
        }
        mainWindow.invalidateShadow()
    }
    
    public func setIgnoreMouseEvents(_ args: [String: Any]) {
        let ignore: Bool = args["ignore"] as! Bool
        let forward: Bool = args["forward"] as! Bool
        mainWindow.ignoresMouseEvents = ignore
        
        if (!ignore) {
            mainWindow.acceptsMouseMovedEvents = false
        } else {
            mainWindow.acceptsMouseMovedEvents = forward
        }
    }
    
    public func startDragging() {
        DispatchQueue.main.async {
            let window: NSWindow  = self.mainWindow
            if(window.currentEvent != nil) {
                window.performDrag(with: window.currentEvent!)
            }
        }
    }
    
    public func isSubWindow() -> Bool {
        let identifier: String = mainWindow.identifier?.rawValue ?? "";
        return identifier == "subwindow"
    }
    
    public func createSubWindow(_ args: [String: Any]) {
        let visibleFrame = NSScreen.main!.visibleFrame
        
        var frameRect: NSRect = NSRect.zero
        if (args["width"] != nil && args["width"] != nil) {
            frameRect.size.width = CGFloat(args["width"] as! Float)
            frameRect.size.height = CGFloat(args["height"] as! Float)
        }
        if (args["x"] != nil && args["y"] != nil) {
            frameRect.topLeft.x = CGFloat(args["x"] as! Float)
            frameRect.topLeft.y = CGFloat(args["y"] as! Float)
        }
        
        let center: Bool = args["center"] as! Bool
        let title: String = args["title"] as! String
        
        if (center) {
            frameRect.origin.x = (visibleFrame.width / 2) - (frameRect.size.width / 2)
            frameRect.origin.y = (visibleFrame.height / 2) + (frameRect.size.height / 2)
        }
        
        let flutterViewController = FlutterViewController.init()
        WindowManagerPluginOriginal.RegisterGeneratedPlugins!(flutterViewController)
        
        let window = SubWindow()
        window.identifier = NSUserInterfaceItemIdentifier("subwindow")
        window.styleMask = NSWindow.StyleMask(rawValue: 0xf)
        window.backingType = .buffered
        
        window.title = title
        window.setFrameOrigin(frameRect.origin)
        window.setContentSize(frameRect.size)
        
        let windowController = NSWindowController()
        windowController.contentViewController = flutterViewController
        windowController.shouldCascadeWindows = true
        windowController.window = window
        windowController.showWindow(self)
    }
    
    // NSWindowDelegate
    
    public func windowShouldClose(_ sender: NSWindow) -> Bool {
        _emitEvent("close")
        if (isPreventClose()) {
            return false
        }
        return true;
    }
    
    public func windowDidResize(_ notification: Notification) {
        _emitEvent("resize")
        if (!_isMaximized && mainWindow.isZoomed) {
            _isMaximized = true
            _emitEvent("maximize")
        }
        if (_isMaximized && !mainWindow.isZoomed) {
            _isMaximized = false
            _emitEvent("unmaximize")
        }
    }
    
    public func windowDidEndLiveResize(_ notification: Notification) {
        _emitEvent("resized")
    }
    
    public func windowWillMove(_ notification: Notification) {
        _emitEvent("move")
    }
    
    public func windowDidMove(_ notification: Notification) {
        _emitEvent("moved")
    }
    
    public func windowDidBecomeMain(_ notification: Notification) {
        _emitEvent("focus");
    }
    
    public func windowDidResignMain(_ notification: Notification){
        _emitEvent("blur");
    }
    
    public func windowDidMiniaturize(_ notification: Notification) {
        _emitEvent("minimize");
    }
    
    public func windowDidDeminiaturize(_ notification: Notification) {
        _emitEvent("restore");
    }
    
    public func windowDidEnterFullScreen(_ notification: Notification){
        _emitEvent("enter-full-screen");
    }
    
    public func windowDidExitFullScreen(_ notification: Notification){
        _emitEvent("leave-full-screen");
    }
    
    public func _emitEvent(_ eventName: String) {
        if (onEvent != nil) {
            onEvent!(eventName)
        }
    }
}


public class SubWindow: NSWindow {
    public override func awakeFromNib() {
        let windowFrame = self.frame
        self.setFrame(windowFrame, display: true)

        super.awakeFromNib()
    }
}
