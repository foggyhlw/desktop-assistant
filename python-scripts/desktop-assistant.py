import paho.mqtt.client as mqtt
import keyboard
import os
import threading

level_brightness = {
    "1": 20,
    "2": 40,
    "3": 60,
    "4": 100,
}

DETECT_TOPIC = '/foggyhlw/boss-detec'
BRIGHTNESS_TOPIC_AUTO = '/desktop-assistant/monitor/brightness_auto'
BRIGHTNESS_TOPIC_MANUAL = '/desktop-assistant/monitor/brightness_manual'
arm_flag = True

brightness_last = 1
brightness_set = 1
def arm_toggle():
    global arm_flag 
    arm_flag = not arm_flag
    print('chang arm status to:', arm_flag)

def on_connect(client, userdata, flags, rc):
    client.subscribe(DETECT_TOPIC)
    client.subscribe(BRIGHTNESS_TOPIC_AUTO)
    client.subscribe(BRIGHTNESS_TOPIC_MANUAL)

    print("BOSS探测已就绪！")

def on_message(client, userdata, msg):
    global arm_flag
    global brightness_last
    global timer
    print(msg.payload.decode('UTF-8'))
    if (msg.topic == DETECT_TOPIC and msg.payload.decode('UTF-8') == 'danger'):
        if(arm_flag):
            print("boss detected!")
            keyboard.press('f12')
            arm_flag = False
    if (msg.topic == BRIGHTNESS_TOPIC_AUTO ):
        brightness_now = level_brightness[str(int(msg.payload))]
        if (brightness_now != brightness_last):
            brightness_last = brightness_now
            timer = threading.Timer(3, set_brightness, [brightness_now])
            timer.start()
        else:
            brightness_last = brightness_now
    if (msg.topic == BRIGHTNESS_TOPIC_MANUAL ):
        brightness_set = level_brightness[str(int(msg.payload))]
        os.popen('monitorian /set {0}'.format(int(brightness_set)))

def set_brightness(brightness_set):
    global brightness_last
    global timer
    if(brightness_set == brightness_last):
        os.popen('monitorian /set {0}'.format(int(brightness_set)))
    else:
        timer.cancel()
        
keyboard.add_hotkey('alt + a', arm_toggle)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.username_pw_set(username = 'foggy',password = '1989228')
client.connect("192.168.0.100", 1883, 60)
client.loop_forever()
