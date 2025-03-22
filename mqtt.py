from paho.mqtt import client as mqtt_client

broker = '192.168.152.254'  
port = 1883
topic = "accx"

print("Testing MQTT connection...")  

def connect_mqtt():
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print(f"Failed to connect, return code {rc}")

    # Set Connecting Client ID
    client = mqtt_client.Client("PythonMQTT")  # Instantiate the client
    client.on_connect = on_connect
    client.connect(broker, port)
    return client


def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        print(f"Received {msg.payload.decode()} from {msg.topic} topic")

    client.subscribe(topic)  # Subscribe to the defined topic
    client.on_message = on_message


client = connect_mqtt()
subscribe(client)
client.loop_forever()
