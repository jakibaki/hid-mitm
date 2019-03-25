package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"log"
	"net"
	"os"
	"time"

	"github.com/veandco/go-sdl2/sdl"
)

type inputPkg struct {
	magic uint16
	keys  uint64
	joyLX int32
	joyLY int32
	joyRX int32
	joyRY int32
}

const (
	keyA           = 1 << 0  ///< A
	keyB           = 1 << 1  ///< B
	keyX           = 1 << 2  ///< X
	keyY           = 1 << 3  ///< Y
	keyLStick      = 1 << 4  ///< Left Stick Button
	keyRStick      = 1 << 5  ///< Right Stick Button
	keyL           = 1 << 6  ///< L
	keyR           = 1 << 7  ///< R
	keyZL          = 1 << 8  ///< ZL
	keyZR          = 1 << 9  ///< ZR
	keyPlus        = 1 << 10 ///< Plus
	keyMinus       = 1 << 11 ///< Minus
	keyDLeft       = 1 << 12 ///< D-Pad Left
	keyDUp         = 1 << 13 ///< D-Pad Up
	keyDRight      = 1 << 14 ///< D-Pad Right
	keyDDown       = 1 << 15 ///< D-Pad Down
	keyLStickLeft  = 1 << 16 ///< Left Stick Left
	keyLStickUp    = 1 << 17 ///< Left Stick Up
	keyLStickRight = 1 << 18 ///< Left Stick Right
	keyLStickDown  = 1 << 19 ///< Left Stick Down
	keyRStickLeft  = 1 << 20 ///< Right Stick Left
	keyRStickUp    = 1 << 21 ///< Right Stick Up
	keyRStickRight = 1 << 22 ///< Right Stick Right
	keyRStickDown  = 1 << 23 ///< Right Stick Down
	keySLLeft      = 1 << 24 ///< SL on Left Joy-Con
	keySRLeft      = 1 << 25 ///< SR on Left Joy-Con
	keySLRight     = 1 << 26 ///< SL on Right Joy-Con
	keySRRight     = 1 << 27 ///< SR on Right Joy-Con
)

func showMsgbox() {
	buttons := []sdl.MessageBoxButtonData{
		{0, 0, "Ok"},
	}

	colorScheme := sdl.MessageBoxColorScheme{
		Colors: [5]sdl.MessageBoxColor{
			sdl.MessageBoxColor{255, 255, 255},
			sdl.MessageBoxColor{0, 0, 0},
			sdl.MessageBoxColor{200, 200, 200},
			sdl.MessageBoxColor{255, 255, 255},
			sdl.MessageBoxColor{255, 255, 255},
		},
	}

	messageboxdata := sdl.MessageBoxData{
		sdl.MESSAGEBOX_INFORMATION,
		nil,
		"",
		"On linux/mac run ./input_pc YOUR_SWITCH_IP\nIf on windows, use the start.bat.",
		buttons,
		&colorScheme,
	}

	sdl.ShowMessageBox(&messageboxdata)
}

func senderLoop(input chan inputPkg) {
	if len(os.Args) != 2 {
		showMsgbox()
		os.Exit(-1)
	}

	ip := os.Args[1]
	service := ip + ":8080"

	RemoteAddr, err := net.ResolveUDPAddr("udp", service)
	if err != nil {
		log.Fatal(err)
	}
	conn, err := net.DialUDP("udp", nil, RemoteAddr)
	if err != nil {
		log.Fatal(err)
	}

	pkg := inputPkg{magic: 0x3275}

	tick := time.Tick(time.Second / 80)
	for {
		select {
		case <-tick:
			var binBuf bytes.Buffer
			binary.Write(&binBuf, binary.LittleEndian, pkg.magic)
			binary.Write(&binBuf, binary.LittleEndian, pkg.keys)
			binary.Write(&binBuf, binary.LittleEndian, pkg.joyLX)
			binary.Write(&binBuf, binary.LittleEndian, pkg.joyLY)
			binary.Write(&binBuf, binary.LittleEndian, pkg.joyRX)
			binary.Write(&binBuf, binary.LittleEndian, pkg.joyRY)

			_, err = conn.Write(binBuf.Bytes())

		case pkg = <-input:
		}

	}
}

var winTitle = "HID-MITM"
var winWidth, winHeight int32 = 200, 200
var joysticks map[int]*sdl.Joystick

func run() int {
	pkg := inputPkg{magic: 0x3275}

	c := make(chan inputPkg)
	go senderLoop(c)

	joysticks = make(map[int]*sdl.Joystick)

	var window *sdl.Window
	var event sdl.Event
	var running bool
	var err error

	sdl.Init(sdl.INIT_EVERYTHING)
	defer sdl.Quit()

	window, err = sdl.CreateWindow(winTitle, sdl.WINDOWPOS_UNDEFINED, sdl.WINDOWPOS_UNDEFINED,
		winWidth, winHeight, sdl.WINDOW_SHOWN)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to create window: %s\n", err)
		return 1
	}
	defer window.Destroy()

	sdl.JoystickEventState(sdl.ENABLE)

	running = true
	for running {
		for event = sdl.WaitEvent(); event != nil; event = sdl.WaitEvent() {
			switch t := event.(type) {

			case *sdl.QuitEvent:
				running = false
				println("Quit")
				break

			case *sdl.JoyAxisEvent:
				switch t.Axis {
				case 0:
					pkg.joyLX = int32(t.Value)
				case 1:
					pkg.joyLY = -int32(t.Value)
				case 2:
					if t.Value > -32768 {
						pkg.keys |= keyZL
					} else {
						pkg.keys &^= keyZL
					}
				case 3:
					pkg.joyRX = int32(t.Value)
				case 4:
					pkg.joyRY = -int32(t.Value)
				case 5:
					if t.Value > -32768 {
						pkg.keys |= keyZR
					} else {
						pkg.keys &^= keyZR
					}
				default:
					fmt.Printf("[%d ms] JoyAxis\ttype:%d\twhich:%c\taxis:%d\tvalue:%d\n",
						t.Timestamp, t.Type, t.Which, t.Axis, t.Value)
				}

			case *sdl.JoyButtonEvent:
				var bit uint64

				switch t.Button {
				case 0:
					bit = keyB
				case 1:
					bit = keyA
				case 2:
					bit = keyY
				case 3:
					bit = keyX
				case 4:
					bit = keyL
				case 5:
					bit = keyR
				case 6:
					bit = keyMinus
				case 7:
					bit = keyPlus
				case 8:
					// Home button
				case 9:
					bit = keyLStick
				case 10:
					bit = keyRStick
				}

				if t.State == 1 {
					pkg.keys |= bit
				} else {
					pkg.keys &^= bit
				}

			case *sdl.JoyHatEvent:
				pkg.keys &^= (0xF << 12)

				switch {
				case t.Value&1 > 0:
					pkg.keys |= keyDUp
				case t.Value&2 > 0:
					pkg.keys |= keyDRight
				case t.Value&4 > 0:
					pkg.keys |= keyDDown
				case t.Value&8 > 0:
					pkg.keys |= keyDLeft
				}

			case *sdl.JoyDeviceAddedEvent:
				joysticks[int(t.Which)] = sdl.JoystickOpen(t.Which)
				if joysticks[int(t.Which)] != nil {
					fmt.Printf("Joystick %d connected\n", t.Which)
				}
			case *sdl.JoyDeviceRemovedEvent:
				if joystick := joysticks[int(t.Which)]; joystick != nil {
					joystick.Close()
				}
				fmt.Printf("Joystick %d disconnected\n", t.Which)

			}

			c <- pkg

		}

	}

	return 0
}

func main() {
	os.Exit(run())
}
