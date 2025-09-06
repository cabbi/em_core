# 1.0.0
# 1.0.2
# 1.0.3
# 1.0.4
- first releases with several adjustments and fixes

# 1.0.5
- templated 'iMolt', 'iDiv' and 'iRound' to have both float and double operations

# 1.0.6
- Added abstract 'EmUpdatable' class for all objects that have an 'Update' method
- Added 'EmUpdater' and 'EmAppUpdaterInterface' classes used to update all 'EmUpdatable' objects in a single call
- Added 'SoftwareSerial' as dependency to avoid compilation errors.

# 1.0.7
- Added 'EmIterator' class to support array and EmList iteration  
- Moved 'EmLogSwSerial' within #ifdef block for ESP32 compatibility. Define 'EM_SOFTWARE_SERIAL' to use 'EmLogSwSerial'

# 1.0.8
- Fixed 'EmList' compilation issues

# 2.0.0
- New naming to align with more c++ style convention
- 'EmSyncValue' is now also 'EmUpdatable'
- Removed the 'EmLogSerialBase' in favour of a simpler 'EmLogPrintTarget'
- Added 'EmOptional' class as lightweight alternative to std::optional in AVR development
- Added "EmDuration" for a more clear time duration definition 
- Added 'EmTime" time handling classes (ESP only)
- Added 'EmStore' and 'EmStoreValue' classes for persistent storage in NVS (ESP only)

# 2.0.1
- 'EmTime' added logLeve in constructor
- 'EmStore' added logLeve in constructor
- 'EmStore' added new methods: 'getStringLength', 'hasValue', 'hasBytes' and 'hasString'