package com.jakibaki.hid_mitmbridge;

import android.content.SharedPreferences;
import android.os.PowerManager;
import android.os.StrictMode;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import static java.lang.Math.abs;

public class MainActivity extends AppCompatActivity {

    volatile int pressed_keys = 0;

    static final int BUTTON_A = 1;
    static final int BUTTON_B = 1 << 1;
    static final int BUTTON_X = 1 << 2;
    static final int BUTTON_Y = 1 << 3;
    static final int BUTTON_L_DOWN = 1 << 4;
    static final int BUTTON_R_DOWN = 1 << 5;
    static final int BUTTON_L1 = 1 << 6;
    static final int BUTTON_R1 = 1 << 7;
    static final int BUTTON_L2 = 1 << 8;
    static final int BUTTON_R2 = 1 << 9;
    static final int BUTTON_PLUS = 1 << 10;
    static final int BUTTON_MINUS = 1 << 11;
    static final int BUTTON_DLEFT = 1 << 12;
    static final int BUTTON_DUP = 1 << 13;
    static final int BUTTON_DRIGHT = 1 << 14;
    static final int BUTTON_DDOWN = 1 << 15;

    volatile int joy_LX = 0;
    volatile int joy_LY = 0;

    volatile int joy_RX = 0;
    volatile int joy_RY = 0;


    public static final Object singleThreadLock = new Object();
    void set_key(int key, boolean state) {
        synchronized( singleThreadLock ) {
            if (state) {
                pressed_keys |= key;
            } else {
                pressed_keys &= ~key;
            }
        }
    }

    boolean dpad_is_axis = true;

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent motionEvent) {
        joy_LX = (int) (motionEvent.getAxisValue(MotionEvent.AXIS_X) * 32767);
        if(abs(joy_LX) < 5000)
            joy_LX = 0;

        joy_LY = (int) (motionEvent.getAxisValue(MotionEvent.AXIS_Y) * -32767);
        if(abs(joy_LY) < 5000)
            joy_LY = 0;

        joy_RX = (int) (motionEvent.getAxisValue(MotionEvent.AXIS_Z) * 32767);
        if(abs(joy_RX) < 5000)
            joy_RX = 0;

        joy_RY = (int) (motionEvent.getAxisValue(MotionEvent.AXIS_RZ) * -32767);
        if(abs(joy_RY) < 5000)
            joy_RY = 0;


        set_key(BUTTON_L2, motionEvent.getAxisValue(MotionEvent.AXIS_BRAKE) > 0.05);

        set_key(BUTTON_R2, motionEvent.getAxisValue(MotionEvent.AXIS_GAS) > 0.05);

        if(motionEvent.getAxisValue(MotionEvent.AXIS_HAT_X) != 0 ||
                motionEvent.getAxisValue(MotionEvent.AXIS_HAT_Y) != 0) {
            dpad_is_axis = true;
        }

        if(dpad_is_axis) {
            set_key(BUTTON_DLEFT, motionEvent.getAxisValue(MotionEvent.AXIS_HAT_X) == -1);
            set_key(BUTTON_DRIGHT, motionEvent.getAxisValue(MotionEvent.AXIS_HAT_X) == 1);

            set_key(BUTTON_DUP, motionEvent.getAxisValue(MotionEvent.AXIS_HAT_Y) == -1);
            set_key(BUTTON_DDOWN, motionEvent.getAxisValue(MotionEvent.AXIS_HAT_Y) == 1);
        }

        return true;
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent keyEvent) {

        boolean keyIsDown = keyEvent.getAction() == KeyEvent.ACTION_DOWN;

        int key_code = keyEvent.getKeyCode();

        switch (key_code) {
            case KeyEvent.KEYCODE_BUTTON_A:
                set_key(BUTTON_B, keyIsDown);
                break;
            case KeyEvent.KEYCODE_BUTTON_B:
                set_key(BUTTON_A, keyIsDown);
                break;
            case KeyEvent.KEYCODE_BUTTON_X:
                set_key(BUTTON_Y, keyIsDown);
                break;
            case KeyEvent.KEYCODE_BUTTON_Y:
                set_key(BUTTON_X, keyIsDown);
                break;
            case KeyEvent.KEYCODE_BUTTON_L1:
                set_key(BUTTON_L1, keyIsDown);
                break;
            case KeyEvent.KEYCODE_BUTTON_R1:
                set_key(BUTTON_R1, keyIsDown);
                break;
            case KeyEvent.KEYCODE_BUTTON_L2:
                set_key(BUTTON_L2, keyIsDown);
                break;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                dpad_is_axis = false;
                set_key(BUTTON_DDOWN, keyIsDown);
                break;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                dpad_is_axis = false;
                set_key(BUTTON_DLEFT, keyIsDown);
                break;
            case KeyEvent.KEYCODE_DPAD_UP:
                dpad_is_axis = false;
                set_key(BUTTON_DUP, keyIsDown);
                break;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                dpad_is_axis = false;
                set_key(BUTTON_DRIGHT, keyIsDown);
                break;
            case KeyEvent.KEYCODE_BUTTON_THUMBL:
                set_key(BUTTON_L_DOWN, keyIsDown);
                break;
            case KeyEvent.KEYCODE_BUTTON_THUMBR:
                set_key(BUTTON_R_DOWN, keyIsDown);
                break;
            case KeyEvent.KEYCODE_BUTTON_SELECT:
            case KeyEvent.KEYCODE_BACK:
                set_key(BUTTON_MINUS, keyIsDown);
                break;
            case KeyEvent.KEYCODE_BUTTON_START:
                set_key(BUTTON_PLUS, keyIsDown);
                break;
            default:
                // Need to do this because android is weird af.
                // Without this line numbers on the keyboard-input no longer work for some reason...
                return super.dispatchKeyEvent(keyEvent);
        }
        return true;
    }

    byte[] build_pkg() {
        ByteBuffer buf = ByteBuffer.allocate(26);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        buf.putShort((short) 0x3275);
        synchronized( singleThreadLock ) {
            buf.putLong(pressed_keys);
            buf.putInt(joy_LX);
            buf.putInt(joy_LY);
            buf.putInt(joy_RX);
            buf.putInt(joy_RY);
        }
        return buf.array();
    }

    private DatagramSocket socket;
    private InetAddress address;

    class SenderThread extends Thread {
        public void run() {

            while(true) {
                byte[] msg;
                msg = build_pkg();


                DatagramPacket packet
                        = new DatagramPacket(msg, msg.length, address, 8080);

                try {
                    socket.send(packet);
                } catch (IOException e) {
                    e.printStackTrace();
                }

                try {
                    sleep(1000/60);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

        }
    }

    private PowerManager powerManager;
    private PowerManager.WakeLock wakeLock;
    private int field = 0x00000020;

    SenderThread senderThread = new SenderThread();

    Button connectButton;
    EditText ipBox;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // I guess it's quite obvious that I know nothing about android development.
        // But hey, it works!

        StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();
        StrictMode.setThreadPolicy(policy);

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        try {
            socket = new DatagramSocket();

        } catch (SocketException e) {
            e.printStackTrace();
        }

        ipBox = findViewById(R.id.ip_adress_box);
        final SharedPreferences prefs = getSharedPreferences("data", MODE_PRIVATE);
        String last_ip = prefs.getString("last_ip", "192.168.178.22");
        ipBox.setText(last_ip);

        try {
            address = InetAddress.getByName(last_ip);
        } catch (UnknownHostException e) {
            e.printStackTrace();
        }

        senderThread.start();

        // Make the phone turn off its screen when something is close to the proximity sensor so people can have their phone in their pocket
        try {
            // Yeah, this is hidden field.
            field = PowerManager.class.getClass().getField("PROXIMITY_SCREEN_OFF_WAKE_LOCK").getInt(null);
        } catch (Throwable ignored) {
        }

        powerManager = (PowerManager) getSystemService(POWER_SERVICE);
        wakeLock = powerManager.newWakeLock(field, getLocalClassName());
        wakeLock.acquire();

        connectButton = findViewById(R.id.set_ip_button);
        connectButton.setOnClickListener(new View.OnClickListener() {

            public void onClick(View v) {

                try {
                    address = InetAddress.getByName(String.valueOf(ipBox.getText()));
                    Toast.makeText(getApplicationContext(), "Sending input to " + ipBox.getText() + " now", Toast.LENGTH_SHORT).show();
                    SharedPreferences.Editor editor = prefs.edit();
                    editor.putString("last_ip", ipBox.getText().toString());
                    editor.commit();

                } catch (UnknownHostException e) {
                    Toast.makeText(getApplicationContext(), " is not a valid ip!", Toast.LENGTH_SHORT).show();
                }

            }
        });
    }
}
