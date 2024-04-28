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

The agricultural sector is recognized as one of the most hazardous industries in the United States, with a notable fatality rate of 23.5 per 100,000 workers. Two primary challenges complicate safety measures in this sector:

1. Asset Management: The extensive use of equipment and vast expanses of farmland make it difficult to monitor and manage assets effectively. Loss and misplacement of equipment are common issues, often highlighted during inventory checks.

2. Connectivity and Safety: Unreliable Wi-Fi and cellular connections in rural areas severely impact the implementation of safety measures. This connectivity gap poses a significant risk to worker safety, potentially leaving individuals isolated and unable to call for help during emergencies.

### LoRaWAN as a Solution

Given the limitations of conventional wireless communications like cellular and WiFi—namely their high cost and inadequate coverage in rural settings—LoRaWAN presents a viable alternative. Its low-cost, long-range capabilities allow for communication up to tens of kilometers in open conditions, addressing critical issues within the agricultural industry. 

To confront the safety and security concerns in the agricultural sector, our group developed a LoRaWAN-based end node that works as a personal emergency button as well as an asset tracker. Containing a GPS module, the end node collects the real time GPS coordinates of the end node and sends it in uplink messages to the server, along with other end node data, including the emergency status of the device, the battery voltage, and unique message codes that can be sent by field workers. Additionally, a Python application was created that will allow farm managers to view the real time GPS locations of the end nodes on a geolocation map, as well as the rest of the end node data. To ensure quick responses to emergencies, we have configured the server to send email and text notifications to relevant parties whenever the emergency button on the end nodes has been pressed. 

<!-- System Architecture -->
## System Architecture

<p align="center">
	<img src="documentation_images/architecture.png" height="600">
</p>

Our system architecture is comprised of of five primary components: the end nodes, the gateway, TheThingsNetwork, ThingSpeak, and a Python GUI. Please note that we refer to TheThingsStack, our network server, as TheThingsNetwork (TTN) in our documentation.

1. The end nodes are waterproof devices that are based around the [HTCC-AB02S](https://heltec.org/project/htcc-ab02s/), a development board that supports LoRaWAN communication and contains an Air530Z GPS and OLED display, making it optimal for our application. Please refer to the [hardware](hardware/) and [heltec_software](heltec_software/) folders, which each contain their own README files, for more information about the design of the end node.

2. The gateway is an SX1303 Raspberry Pi LoRaWAN gateway that was built and configured on TTN by the Fall 2023 LoRaWAN team. Additional information about the gateway and the instructions that the previous team used to set it up can be found [here](https://www.waveshare.com/wiki/SX1302_LoRaWAN_Gateway_HAT).

3. TTN is our network server, where all of the uplink messages that are forwarded by the gateway are received, decoded, and forwarded to ThingSpeak. TTN is discussed in greater detail in the [TheThingsNetwork](#thethingsnetwork) section of this README.

4. ThingSpeak is our application server, where the data received from TTN is displayed and processed, allowing alerts to be triggered when an emergency signal is sent by an end node. Originally, we intended for ThingSpeak to act as the platform on which farm managers could view end node data and GPS locations, but due to limitations that are described in our report, we decided to develop a Python GUI to serve this purpose instead. The [ThingSpeak API](https://thingspeak.readthedocs.io/en/latest/api.html) allowed us to extract our end node data from ThingSpeak and use it in our GUI.

5. The Python GUI extracts data from ThingSpeak, generating a fully interactive map displaying various end node locations and paths, as well as presenting end node data such as emergency status, battery charge, messages, and time since the last uplink message was received, all within a single interface. Please refer to the [gui_software](gui_software/) folder, which contains its own README file, for more information about the Python GUI.


<!-- Operation -->
## TheThingsNetwork

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