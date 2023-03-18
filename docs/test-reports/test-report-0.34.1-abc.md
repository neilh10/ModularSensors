# Testing 
For the mayfly_0.34.1-abc_230314_1615_LT5_Mdbus_wireless.hex   
the target customer setup is
Mayfly 1.1
RS485 board & 12V boost  knh002rev7
Insitu ruggesd connector with rat tail, to Insitu Rugged Calbe,    
to Insitu LT500 water depth and temperature gauge.
with Adafruit 4.4Amp battery
and BSP212 2.5W battery, and step down converters Knh004rev3

This was run as a beta for two systems  over three months    
These where named tu_rcru_test06 and tu_rcru_test07    
https://monitormywatershed.org/sites/tu_rcru_test06/    
https://monitormywatershed.org/sites/tu_rcru_test07/    
Details of extensive reliable delivery, with final complete delivery of 6000 readings documented here    
https://github.com/ODM2/ODM2DataSharingPortal/issues/641    
This included a period of two weeks when LiIon voltage dropped below a 3.8V threshold,   
and readings where still collected and stored locally on the uSD.    
After the sunshine returned and the battery gained some charge exceeding 3.8V,
the system started attempting POSTs to MMW. However MMW had poor responses, and the retried POSTing readings was very slow to be accepted (give a 201) from MMW.    
However enventually MMW was rebooted and data started flowing.    
Eventually all data was sent to MMW, and then was downloaded and checked for completeness.    
The SequenceNumber or SampleNum where all continguous with no loss of readings.    