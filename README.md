# individual_project_iot

1) SETUP and RUN

First at all, install the espressif framework, using that tutorial at: "https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html#manual-installation".
Second, go inside the project folder, type the command "idf.py menuconfig", go to "Project Configuration Options" and modify ID WiFi, WiFi password, MQTT broker URI, MQTT client username and MQTT client password with yours.
Then through the command "idf.py -p path_to_the_esp flash monitor" you can build, flash and monitor the project in your esp device.

OSS:
If it is required to insert the target, it can be done through the command "idf.py set-target esp32S3", for a esp32 s3 device.

2) EXPLAINATION OF THE PROJECT

The project, first at all creates a queue for the synchronization between wifi-connection and mqtt connection, which requires the wifi connection set up.
Then starts the wifi connection and allocation of its structures, by pinning that task to the core 1 of the cpu (the one which is in idle status).
Then is created the calibration handle for the convertion of the raw values of the sampled signal in its corresponding milliVolts, considering an attenuation of 12 DB and a Bit Width of 12 bits.
An array of 1024 samples is populated by reading the ADC1 Channel 0 pin with a oversample frequence of SOC_ADC_SAMPLE_FREQ_THRES_HIGH, which in my case is 83333 Hz.
Such a sampled signal is normalized and put in the FFT function for getting the max frequency of it.
Then is doubled the value of the max frequency of the signal and starts the function which samples the signal in a time window TIME_WINDOW with a sampling frequence result of the previous doubling.

OSS:
if the value of the adaptive sampling frequency is grather than the bottlenck maximum frequency (it depends on the resources of the device and it's calculated at runtime), the signal is captured undersampling it and a warning is printed.

Then the average of the new sampling is compued.
This average is a ... average.
After that is start the mqtt module with the authentication params (OSS: the broker to use its HiveMQ).

After the mqtt client starts, the mean value is published under the topic "/mean" with QoS=1.

In the end, the calibration, mqtt client, wifi structures and the queue are deallocated.


3) PERFORMANCE ANALYSIS

For calculatiung the power consuming (in milliWatts) I have used the INA219 circuit and adapting the sampling frequency (doing the fft and sampling at 2*max frequency) and using a signal of 440 Hz (so sampling at 976 hz, cause of a certain degree of error of fft) i have calculated an average consume of 329.7 mW.
Calculating the power consuming avoiding the usage of fft and sampling at maximum sampling rate (27.78 khz in my case) i have calculated an average power consuming of 376.7 mW, so having a saving of 12.48% mW using the first solution.
(obviously the power consumption of the wifi and mqtt setups and usage are a common overhead for both the solutions).

The volume of data trasmitted by the system from the setup of the wifi point, to the publishing of the mqtt message is about 4.6 KiloBytes (we are using wifi technology).

For calculating the latency of the published data, I have to do a premise:
while I was synchronizing the esp32 device to an NTP server through "sntp_setservername(0, "pool.ntp.org")", I have noticed that the timestamp getted before publishing the mean message through mqtts has a timestamp previous in time wrt the timestamp provided by the broker and associated with such a message; so there is a de-synchronization of approximately 150 milliseconds between the time servers (the used by the esp and HiveMQs' one). After this premise, I have calculated an average of 1.2 seconds in the interval time between the generation of the mean data and the reception of that data by the broker.













