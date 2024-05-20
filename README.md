# individual_project_iot

While I was synchronizing the esp32 device to an NTP server through "sntp_setservername(0, "pool.ntp.org")", I have noticed that the timestamp getted before publishing the mean message through mqtts has a timestamp previous in time wrt the timestamp provided by the broker and associated with such a message. So I cannot measure the latency cause a de-synchronization of few milliseconds the time servers (mine and HiveMQ).

For calculatiung the power consuming (in milliWatts) I have used the INA219 circuit and adapting the sampling frequency (doing the fft and sampling at 2*max frequency) and using a signal of 440 Hz (so sampling at 976 hz, cause of a certain degree of error of fft) i have calculated an average consume of 329.7 mW.
Calculating the power consuming avoiding the usage of fft and sampling at maximum sampling rate (27,78 khz in my case) i have calculated an average power consuming of 376.7 mW, so having a saving of 12.48% mW using the first solution.
