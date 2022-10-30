# FLORAbot
Open-Source platform to monitor plant health and optimize on agriculture through data and augmented speech.

![florabot logo](https://user-images.githubusercontent.com/58347016/190914597-c5c6265c-ab0e-44f4-a64f-7828ac7b5182.png)

**THE CHALLENGES PLANTATION OWNERS FACE TODAY-**
- Many trips have to be taken in order to manually check the soil-moisture on a regular basis. - It can be difficult to know the exact amount of water to give plants, thus causing stress for the crops by over or under-watering.
- Over-watering plants and crops could lead to higher water costs than what is really needed.
- It is sometimes difficult to know the optimal time to plant without data.
- Manually measuring key data points about crops is often difficult, time-consuming, and more likely to be inaccurate.

![UIflow](https://user-images.githubusercontent.com/58347016/198899923-8cd2c9a6-37b9-408d-8123-a1026fe6f3a6.jpg)
FLORAbot allows plantation owners to instantly know several agricultural data points about their crop.
-	Soil Moisture (VWC)
-	UVA/B irradiance
-	Ambient Climate Parameters including IAQ.
thus, enabling better water conservation, less likely to over or under-water crops and save time and resources.

**WHAT IF PLANTS COULD TALK?**
The answer is that we can know what they want, which will help us to respond to their needs accurately and precisely. For example, if the temperature rises, the moisture in the soil is decreased; as a result, plants need more water. If they are not watered at the right time, they will gradually wilt and eventually die.
Hence for PoC development, the solution utilizes WizFi360-EVB-Pico board that will be interfaced to capacitive moisture sensor, UV irradiance and Sparkfun Environment Sensor [CCS811+BME280] for real-time monitoring of plantation. Data sampling and processing will be done on RP2040 chipset and communication will be done through WizFi360-PA chipset. The entire device is fashioned around to be powered through battery or USB and enclosed in all-weather custom enclosure. Device communicates sensor data over MQTT to Home-Assistant/ HASS Server hosted natively on Raspberry PI 4.
The end user can set the schedule, perform automations and create scenes with beautiful dashboards via HASS App.
Through Advanced Mode (more likely to be used by home-growners and hobbyist gardeners), the FLORAbot allows user to interact directly with his/her plant through Alexa/Google Home Service, for this functionality the device utilizes Google Speech Engine Service to converse in a more human-friendly manner such as local weather updates, water requirement, daily-greets and n-things that can be done by user.

Demo: https://youtu.be/0GKHVBIlNC8
