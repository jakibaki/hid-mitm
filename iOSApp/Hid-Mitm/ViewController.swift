//
//  ViewController.swift
//  Hid-Mitm
//
//  Created by Jakob Dietrich on 27.03.19.
//  Copyright © 2019 Jakob Dietrich. All rights reserved.
//
// Credit to https://github.com/jazzplato/MFi-Controller-Swift for an example on how to mfi

import UIKit
import GameController
import Foundation
import SwiftSocket

class ViewController: UIViewController, UITextFieldDelegate {
    var mainController: GCController?
    weak var timer: Timer?
    var client: UDPClient?
    var i: Int = 0
    var fd: Int32 = -1
    @IBOutlet weak var ipBox: UITextField!
    
    func toByteArray<T>(_ value: T) -> [UInt8] {
        var value = value
        return withUnsafeBytes(of: &value) { Array($0) }
    }
    
    
    override func viewDidLoad() {
        super.viewDidLoad()
        self.ipBox.delegate = self
        
        NotificationCenter.default.addObserver(self,
                                               selector: #selector(self.controllerWasConnected),
                                               name: NSNotification.Name.GCControllerDidConnect,
                                               object: nil)
        NotificationCenter.default.addObserver(self,
                                               selector: #selector(self.controllerWasDisconnected),
                                               name: NSNotification.Name.GCControllerDidDisconnect,
                                               object: nil)
        // Do any additional setup after loading the view.
        

    }
    
    @objc func timerFired(timer: Timer) {
        
        guard let gamepad: GCExtendedGamepad = self.mainController?.extendedGamepad else {
            return
        }

        var pkg: [UInt8] = []
        
        
        let magic: UInt16 = 0x3275
        pkg.append(contentsOf: self.toByteArray(magic))
        
        var keys: UInt64 = 0
        
        if gamepad.buttonB.isPressed {
            keys |= 1
        }
        
        if gamepad.buttonA.isPressed {
            keys |= 1 << 1
        }
        
        if gamepad.buttonY.isPressed {
            keys |= 1 << 2
        }
        
        if gamepad.buttonX.isPressed {
            keys |= 1 << 3
        }
        
        
        if #available(iOS 12.1, *) {
            if let pressed = gamepad.leftThumbstickButton?.isPressed {
                if pressed {
                    keys |= 1 << 4
                }
            }
            
            if let pressed = gamepad.rightThumbstickButton?.isPressed {
                if pressed {
                    keys |= 1 << 5
                }
            }
        }
        
        
        if gamepad.leftShoulder.isPressed {
            keys |= 1 << 6
        }
        
        if gamepad.rightShoulder.isPressed {
            keys |= 1 << 7
        }
        
        if gamepad.leftTrigger.isPressed {
            keys |= 1 << 8
        }
        
        if gamepad.rightTrigger.isPressed {
            keys |= 1 << 9
        }
        
        if gamepad.dpad.left.isPressed {
            keys |= 1 << 12
        }
        if gamepad.dpad.up.isPressed {
            keys |= 1 << 13
        }
        if gamepad.dpad.right.isPressed {
            keys |= 1 << 14
        }
        if gamepad.dpad.down.isPressed {
            keys |= 1 << 15
        }
        
        
        
        
        // nControl triggers this for plus and minus so we might as well use it.
        if gamepad.buttonX.isPressed && gamepad.leftShoulder.isPressed && gamepad.leftTrigger.isPressed && gamepad.rightShoulder.isPressed && gamepad.rightTrigger.isPressed {
            keys |= 1 << 10
            keys &= ~(1 << 3)
            keys &= ~(1 << 6)
            keys &= ~(1 << 7)
            keys &= ~(1 << 8)
            keys &= ~(1 << 9)
        }
        
        if gamepad.dpad.right.isPressed && gamepad.leftShoulder.isPressed && gamepad.leftTrigger.isPressed && gamepad.rightShoulder.isPressed && gamepad.rightTrigger.isPressed {
            keys |= 1 << 11
            keys &= ~(1 << 3)
            keys &= ~(1 << 6)
            keys &= ~(1 << 7)
            keys &= ~(1 << 8)
            keys &= ~(1 << 9)
        }
        
        pkg.append(contentsOf: self.toByteArray(keys))
        
        let joyLeftX: Int32 = Int32(32767 * gamepad.leftThumbstick.xAxis.value)
        pkg.append(contentsOf: self.toByteArray(joyLeftX))
        let joyLeftY: Int32 = Int32(32767 * gamepad.leftThumbstick.yAxis.value)
        pkg.append(contentsOf: self.toByteArray(joyLeftY))
        
        let joyRightX: Int32 = Int32(32767 * gamepad.rightThumbstick.xAxis.value)
        pkg.append(contentsOf: self.toByteArray(joyRightX))
        let joyRightY: Int32 = Int32(32767 * gamepad.rightThumbstick.yAxis.value)
        pkg.append(contentsOf: self.toByteArray(joyRightY))
        
        self.client?.send(data: pkg)
        //self.udpSend(fd: self.fd, dataToSend: pkg, address: self.addr, port: 8080)
    }
    
    func connectToIp(ip: String) {
        timer?.invalidate()
        client?.close()
        client = UDPClient(address: ip, port: 8080)
        
        // ios 9 compat
        timer = Timer.scheduledTimer(timeInterval: 1/80, target: self, selector: #selector(timerFired), userInfo: ip, repeats: true)
    }
    
    func textFieldShouldReturn(_ textField: UITextField) -> Bool {
        textField.resignFirstResponder()
        connectToIp(ip: textField.text!)
        return true
    }

    @IBAction func buttonPressed(_ sender: UIButton) {
        connectToIp(ip: self.ipBox.text!)
        self.view.endEditing(true)
    }
    
    @IBAction func disconnectButtonPressed(_ sender: Any) {
        self.ipBox.text = ""
        self.view.endEditing(true)
        timer?.invalidate()
    }
    
    @objc private func controllerWasConnected(_ notification: Notification) {
        let controller: GCController = notification.object as! GCController
        let status = "MFi Controller: \(controller.vendorName) is connected"
        print(status)
        
        mainController = controller
    }
    
    @objc private func controllerWasDisconnected(_ notification: Notification) {
        let controller: GCController = notification.object as! GCController
        let status = "MFi Controller: \(controller.vendorName) is disconnected"
        print(status)
        
        mainController = nil
    }
    
}

