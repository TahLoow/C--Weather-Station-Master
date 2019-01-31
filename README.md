# Weather-Station-Master
A Computer Networking semester group project. Header files (excluding project.h) have been removed for the de-"educational" purposes.

The compounded function of the programs is to create a system that:
  - Acquires the weather conditions from a David Weather Station (wgetter.c) by the minute
  - Puts latest weather conditions into shared memory (operations.c)
  - Archives hourly weather data into daily archive files
  - Allows network connections to the program from over the internet
  - Maintains a log of network
  
# Components
## engine.c
  Controls primary functions of the program. On a 60-second (recursive) interval, prompts operations.c to acquire, process, share, and archive weather conditions. Calls `connectNetwork()` from network.c once.
## operations.c 
  The middleman for weather acquisition, storing the weather into the proper struct, averaging, shared memory, and file archiving.
## network.c
  Handles network socket creation. Continuosly listens for network connections. Upon connection, processes a request of data, being current data or a given date's records of data.
## wgetter.c
  Acquires raw weather data from David Weather Station. Imports raw sensor_image into currentData struct, which is used throughout the program primarily via the operations.c interface.
## logger.c
  Logs events into the Logs folder using `logger(SourceString,FormatString,Args)`. Records the SourceString, followed by the Args formatted into the FormatString.

