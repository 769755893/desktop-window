//
//  FlutterWindow.swift
//  flutter_multi_window
//
//  Created by Bin Yang on 2022/1/10.
//
import Cocoa
import FlutterMacOS
import Foundation

class BaseFlutterWindow: NSObject {
  private let window: NSWindow
  let windowChannel: WindowChannel

  init(window: NSWindow, channel: WindowChannel) {
    self.window = window
    self.windowChannel = channel
    super.init()
  }

  func show() {
    window.makeKeyAndOrderFront(nil)
    NSApp.activate(ignoringOtherApps: true)
  }

  func hide() {
    window.orderOut(nil)
  }

  func center() {
    window.center()
  }

  func setFrame(frame: NSRect) {
    window.setFrame(frame, display: false, animate: true)
  }

  func setTitle(title: String) {
    window.title = title
  }

  func resizable(resizable: Bool) {
    if (resizable) {
      window.styleMask.insert(.resizable)
    } else {
      window.styleMask.remove(.resizable)
    }
  }

  func close() {
    window.close()
  }

  func setFrameAutosaveName(name: String) {
    window.setFrameAutosaveName(name)
  }
}

class FlutterWindow: BaseFlutterWindow {
  let windowId: Int64

  let window: NSWindow

  weak var delegate: WindowManagerDelegate?

  init(id: Int64, arguments: String) {
    windowId = id
    window = NSWindow(
      contentRect: NSRect(x: 0, y: 0, width: 480, height: 270),
      styleMask: [.miniaturizable, .closable, .resizable, .titled, .fullSizeContentView],
      backing: .buffered, defer: false)
    let project = FlutterDartProject()
    project.dartEntrypointArguments = ["multi_window", "\(windowId)", arguments]
    let flutterViewController = FlutterViewController(project: project)
    window.contentViewController = flutterViewController

    let plugin = flutterViewController.registrar(forPlugin: "FlutterMultiWindowPlugin")
    FlutterMultiWindowPlugin.registerInternal(with: plugin)
    let windowChannel = WindowChannel.register(with: plugin, windowId: id)
    // Give app a chance to register plugin.

    FlutterMultiWindowPlugin.onWindowCreatedCallback?(flutterViewController)

    RegisterPluginsOrigin(registry: flutterViewController)

    super.init(window: window, channel: windowChannel)

    window.delegate = self
    window.isReleasedWhenClosed = false
    window.titleVisibility = .hidden
    window.titlebarAppearsTransparent = true
  }

  deinit {
    debugPrint("release window resource")
    window.delegate = nil
    if let flutterViewController = window.contentViewController as? FlutterViewController {
      flutterViewController.engine.shutDownEngine()
    }
    window.contentViewController = nil
    window.windowController = nil
  }
}

extension FlutterWindow: NSWindowDelegate {
  func windowWillClose(_ notification: Notification) {
    delegate?.onClose(windowId: windowId)
  }

  func windowShouldClose(_ sender: NSWindow) -> Bool {
    delegate?.onClose(windowId: windowId)
    return true
  }
}

func RegisterPluginsOrigin(registry: FlutterPluginRegistry) {
  SharedPreferencesPluginOriginal.register(with: registry.registrar(forPlugin: "SharedPreferencesPlugin"))
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
