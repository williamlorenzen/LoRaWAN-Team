<!-- PROJECT LOGO -->
<br />
<p align="center">
  <a href="https://github.com/williamlorenzen/LoRaWAN-Team">
    <img src="documentation_images/IoT.jpg" alt="Logo" height="100">
  </a>

  <h2 align="center" style="font-size: 36px;">LoRaWAN Asset Tracker and Emergency Button</h2>

  <p align="center" style="font-size: 24px;">
    Spring 2024 LoRaWAN Team
    <br />
  </p>
</p>

<!-- Table of Contents -->

<!-- Background -->
## Background

### LoRaWAN Overview

<p align="center">
	<img src="documentation_images/LoRaWAN.jpg" height="250">
</p>

LoRaWAN (Long Range Wide Area Network) is a networking protocol specially designed for low power, wide area applications. Introduced in the mid-2010s, LoRaWAN is known for its lower power consumption, cost-effectiveness, and longer range compared to traditional WiFi and cellular communications. However, it features lower bandwidth, which restricts its use for real-time, high data transfer applications. This makes LoRaWAN particularly well-suited for agricultural applications, where its long range and low power consumption are ideal for monitoring and reporting across extensive areas without requiring frequent battery recharges/replacements. To learn more about LoRaWAN, refer to [TheThingsNetwork's documentation](https://www.thethingsnetwork.org/docs/lorawan/).

### Problem Overview

The agricultural sector is recognized as one of the most hazardous industries in the United States, with a notable fatality rate of [23.5 per 100,000 workers](https://www.bls.gov/news.release/cfoi.nr0.htm). Two primary challenges complicate safety measures in this sector:

1. Asset Management: The extensive use of equipment and vast expanses of farmland make it difficult to monitor and manage assets effectively. Loss and misplacement of equipment are common issues, often highlighted during inventory checks.

2. Connectivity and Safety: Unreliable Wi-Fi and cellular connections in rural areas severely impact the implementation of safety measures. This connectivity gap poses a significant risk to worker safety, potentially leaving individuals isolated and unable to call for help during emergencies.

### LoRaWAN as a Solution

Given the limitations of conventional wireless communications like cellular and WiFi—namely their high cost and inadequate coverage in rural settings—LoRaWAN presents a viable alternative. Its low-cost, long-range capabilities allow for communication up to tens of kilometers in open conditions, addressing critical issues within the agricultural industry. 

To confront the safety and security concerns in the agricultural sector, our group developed a LoRaWAN-based end node that works as a personal emergency button as well as an asset tracker. Containing a GPS module, the end node collects the real time GPS coordinates of the end node and sends it in uplink messages to the server, along with other end node data, including the emergency status of the device, the battery voltage, and unique message codes that can be sent by field workers. Additionally, a Python application was created that will allow farm managers to view the real time GPS locations of the end nodes on a geolocation map, as well as the rest of the end node data. To ensure quick responses to emergencies, we have configured the server to send email and text notifications to relevant parties whenever the emergency button on the end node has been pressed. 

<!-- System Architecture -->
## System Architecture

<p align="center">
	<img src="documentation_images/architecture.png" height="600">
</p>

Our system architecture is comprised of of five primary components: the end nodes, the gateway, TheThingsNetwork, ThingSpeak, and a Python GUI. Please note that we refer to TheThingsStack, our network server, as TheThingsNetwork (TTN) in our documentation.

1. The end nodes are waterproof devices that are based around the [HTCC-AB02S](https://heltec.org/project/htcc-ab02s/), a development board that supports LoRaWAN communication and contains an Air530Z GPS and OLED display, making it optimal for our application. Please refer to the [hardware](hardware/) and [heltec_software](heltec_software/) folders, which each contain their own README files, for more information about the design of the end node.

2. The gateway is an SX1303 Raspberry Pi LoRaWAN gateway that was built and configured on TTN by the Fall 2023 LoRaWAN team. Additional information about the gateway and the instructions that the previous team used to set it up can be found [here](https://www.waveshare.com/wiki/SX1302_LoRaWAN_Gateway_HAT). If you want to learn how to set up a new gateway and configure it on TTN, refer to the [TTN documentation](https://www.thethingsindustries.com/docs/gateways/models/).

3. TTN is our network server, where all of the uplink messages that are forwarded by the gateway are received, decoded, and forwarded to ThingSpeak. TTN is discussed in greater detail in the [TheThingsNetwork](#thethingsnetwork) section of this README.

4. ThingSpeak is our application server, where the data received from TTN is displayed and processed, allowing alerts to be triggered when an emergency signal is sent by an end node. Originally, we intended for ThingSpeak to act as the platform on which farm managers could view end node data and GPS locations, but due to limitations that are described in our report, we decided to develop a Python GUI to serve this purpose instead. The [ThingSpeak API](https://thingspeak.readthedocs.io/en/latest/api.html) allowed us to extract our end node data from ThingSpeak and use it in our GUI.

5. The Python GUI extracts data from ThingSpeak, generating a fully interactive map displaying various end node locations and paths, as well as presenting end node data such as emergency status, battery charge, messages, and time since the last uplink message was received, all within a single interface. Please refer to the [gui_software](gui_software/) folder, which contains its own README file, for more information about the Python GUI.

<!-- Operation -->
## TheThingsNetwork

TTN is the backbone of our system, with it receiving the uplink messages from our end nodes, decrypting and decoding the data, and forwarding the properly formatted data to ThingSpeak. TTN can also schedule downlink messages to particular end nodes, which we have implemented functionality for on our devices. 

On TTN, the gateway and end nodes must be configured appropriately in order to facilitate LoRaWAN communication. The LoRaWAN and Sentinel teams are using a shared TTN account for their implementations, on which the SX1303 gateway has already been configured. Future team members can reach out to [William](#authors) for the login information. As the gateway has already been configured on this shared account, configuration of the gateway on TTN will not be explained. TTN can be accessed [here](https://nam1.cloud.thethings.network/console/applications).

The important part of TTN for building upon our project or implementing other LoRaWAN based applications is the "Applications" tab. It is here that end devices are configured, which in our case is the HTCC-AB02S. We have designed our system so that each end node is configured in in its own unique application. This is because our TTN applications are configured to send decoded data to ThingSpeak, which then appears on ThingSpeak channels. The problem with these ThingSpeak channels is that there is a maximum of eight data fields per channel, meaning that a single ThingSpeak channel would not be able to handle the end node data of even two of our end nodes. Additionally, a single TTN application cannot be configured to send data from different end devices to separate ThingSpeak channels to circumvent the channel field limit. Therefore, we decided that our system would implement a dedicated TTN application for each of its end nodes, with each dedicated application sending end node data to a dedicated ThingSpeak channel. This way, each device was able to send eight fields of data to ThingSpeak, which was sufficient for our project and will allow future teams the bandwidth to send additional data from our end nodes, such as agricultural sensor data.

Establishing connection between TTN and ThingSpeak includes creating a webhook to a ThingSpeak channel from a TTN application. This process is explained in this [video](https://www.youtube.com/watch?v=b9Ga4nwnsTM).

The next step is to register an end device in an application. To do so: 

1. Click "Register end device" inside the relevant application.

2. Fill in the below information for the AB02S:

<p align="center">
	<img src="documentation_images/registration.png" width = "400">
</p>

	Depending on what brand or model that you use, the information may need to be different. But make 	sure you select your model with (Class A - OTAA), as is done in the image above. This is because 	end devices should be configured as Class A to reduce power draw, and OTAA communication should be 	used because it is more secure than ABP. More on [LoRaWAN classes]	(https://www.thethingsnetwork.org/docs/lorawan/classes/) and [communication types]	(https://www.thethingsnetwork.org/forum/t/what-is-the-difference-between-otaa-and-abp-	devices/2723).

3. Input all 0's for the JoinEUI

4. Have TTN Generate the DevEUI and AppKey for you.

5. Go to your end node's script and update the DevEUI and AppKey to reflect the TTN generated information. For our project, this means going to the [GPS_LoRa script](heltec_software/GPS_LoRa/GPS_LoRa.ino) and updating the devEUI and appKEY arrays.

6. Click "Register end device." Your end device is now configured on TTN.

7. Click on the registered device and select "Payload Formatters."

8. For "Formatter type," select "Custom Javascript formatter."

9. Input your custom payload formatter, which decodes the payload that has been received from the end device. This should be tailored to the way in which you prepare your payload on your end node device. For our project, our TTN payload formatter was created according to how we filled the "appData" buffer in our GPS_LoRa.ino script. See the [heltec_software README](heltec_software/README.md) and [GPS_LoRA.ino script](heltec_software/GPS_LoRa/GPS_LoRa.ino) for more information. Our TTN payload formatter is below:

```
function Decoder(bytes) {
  // Decode an uplink message from a buffer
  var decoded = {};

  // Decode longitude
  var lng_frac_part = (bytes[0] | (bytes[1] << 8) | bytes[2] << 16);
  var lng_int_part = (bytes[3]);
  if (bytes[4] === 0xFF ){ //field 1
    decoded.lng = -1 * (lng_int_part + lng_frac_part / 1e6);
  } else {
    decoded.lng = (lng_int_part + lng_frac_part / 1e6);
  }
  
  // Decode latitude
  var lat_frac_part = (bytes[5] | (bytes[6] << 8) | bytes[7] << 16);
  var lat_int_part = (bytes[8]);
  if (bytes[9] === 0xFF ){ // field 2
    decoded.lat = -1 * (lat_int_part + lat_frac_part / 1e6);
  } else {
    decoded.lat = (lat_int_part + lat_frac_part / 1e6);
  }
  // button state
  if (bytes[10] === 0x00){
    decoded.button = 0; //not pressed
  } else if (bytes[10] == 0xFF){
    decoded.button = 1; //pressed
  }
  
  var bat_vol = (bytes[11] | bytes[12] << 8);
  decoded.bat = (bat_vol / 1e3);
  
  var message_code = bytes[13];
  decoded.message = message_code;

  //return decoded;
  return {
  field1: decoded.lng,
  field2: decoded.lat,
  field3: decoded.button,
  field4: decoded.bat,
  field5: decoded.message
  };
}
```

The combination of a ThingSpeak webhook and the returning of fields in the payload formatter ensures that data is sent to the ThingSpeak channel that is specified in the TTN "Integrations" tab. In our payload formatter, the longitude of the end node is sent to a ThingSpeak channel's field 1, the latitude is sent to field 2, the emergency status is sent to field 3, the battery voltage (V) is sent to field 4, and the message code is sent to field 5. These fields then show up on the ThingSpeak channel, as seen in the figures below.

<p align="center">
	<img src="documentation_images/payload.png" width = "250">
</p>

<p align="center">
	<img src="documentation_images/thingspeak.png" width = "250">
</p>

With that, an end node can be been configured on TTN. When a new end node is being added to the system, a new application will have to be created, the end device registered, the relevant Javascript formatter code pasted into into payload formatter, a new ThingSpeak channel created, and the webhook to the new channel established.

<!-- Gateway -->
## Gateway

The SX1303 Raspberry Pi gateway has already been fully setup and initialized on TTN. To connect the end nodes to TTN, the gateway needs to be connected to the server as well, which is done by powering up the gateway and running its packet forwarder. 

There are two options for connecting the gateway to the server:
1. SSHing in
2. Starting the packet forwarder directly on the Raspberry Pi

Option 1:
1. Connect the gateway and your computer to the same WiFi network. You cannot use eduroam on the gateway, so you will either need to use your hotspot, or connect to the wifi that Dr.Eisenstadt has setup in NEB212. If you are using your hotspot, the gateway will not automatically join the first time, so you will need to directly startup the Raspberry Pi and connect to your WiFi hotspot manually, like in Option 2, so that the gateway knows your hotspot. Upon future bootups, the gateway will automatically connect to your hotspot.
2. Once the gateway and your computer are on the same Wifi, open up "command prompt" on your computer.
3. Paste:
	```
	ssh lora23@raspberrypi
	```
4. When prompted for the password, enter:
	```
	IoT4Ag
	```
5. When you have connected to the gateway, paste the following lines separately:
	```
	cd ~
	```
	```
	cd ~/sx1302_hal/
	```
	```
	cd packet_forwarder
	```
	```
	sudo ./lora_pkt_fwd -c test_conf
	```

Option 2:
1. Connect an HDMI connector to the gateway and a monitor. Connect a mouse and keybord to the gateway.
2. Manually connect to your hotspot or other WiFi (not eduroam) on the Raspberry Pi.
3. Open up command prompt on the Raspberry Pi.
4. When you have connected to the WiFi, paste the following lines separately:
	```
	cd ~
	```
	```
	cd ~/sx1302_hal/
	```
	```
	cd packet_forwarder
	```
	```
	sudo ./lora_pkt_fwd -c test_conf
	```

The packet forwarder will now be running. You should see an error relating to the GPS on the gateway being constantly output to the command prompt. This is expected. When you look at the "Gateways" tab on TTN while signed into the shared account, you should see that the "waveshare-sx1303-eedesign2" gateway has its status listed as "Connected." 

The gateway needs to be connected to WiFi at all times to communicate with the server and receive information from the end nodes.

<!-- Operation -->
## Operation




<!-- Authors -->
## Authors

| Name               | Email                                                   | Phone       |
|--------------------|---------------------------------------------------------|-------------|
| William Lorenzen   | [liamv1200@gmail.com](mailto:liamv1200@gmail.com)       | 813-777-3454|
| Maria Barrera      | [maria.barrera@ufl.edu](mailto:maria.barrera@ufl.edu)   | 786-319-6846|
| Justin Nagovskiy   | [jnagovskiy@ufl.edu](mailto:jnagovskiy@ufl.edu)         | 954-258-6993|

Please reach out to William if you have any questions.

[Project Repository on GitHub](https://github.com/williamlorenzen/LoRaWAN-Team)