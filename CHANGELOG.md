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