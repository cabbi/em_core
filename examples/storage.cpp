#include "em_defs.h"
#include "em_log.h"
#include "em_log_print.h"
#include "em_storage.h"

EmLogPrintTarget<USB_SERIAL_CLASS> printLog(Serial);

void nvsIterateKeys(void* userArgs, const char* nameSpace, const char* key) {
    logInfo<100>("NVS--->", "Key: %s.%s", nameSpace, key);
}

template<typename T>
void nvsTest(EmStorage& ts, const char* key, const T& value) {
    T v = value;
    ts.setValue(key, v);
    v = 0;
    ts.getValue(key, v);
    if (v != value) {
        logError<100>("TS---->", "'%s' FAILED!", key);
    } else {
        logInfo<100>("TS---->", "'%s' SUCCESS!", key);
    }
}    

void nvsTestString(EmStorage& ts, const char* key, const char* value) {
    ts.setString(key, value);
    EmStringM str;
    ts.getString(key, str);
    if (str != value) {
        logError<100>("TS---->", "'%s' FAILED!", key);
    } else {
        logInfo<100>("TS---->", "'%s' SUCCESS!", key);
    }
}    

EmStorage ts("test", EmLogLevel::debug);
EmStorageValue<double, ts> doubleStValue("doubleStValue", 100.543);
EmStorageTag<ts> intStTag("intStTag", EmTagValue(11), EmSyncFlags::canReadCanWrite);

void TEST_NVS() {
    int iv = 0; 
    double dv = 0;
    EmStringL sv;
    EmTagValue tv;


    if (!ts.begin("TEST_NVS", 4)) {
        logError("TS---->", "TEST_NVS init failed!");
        return;
    }

    ts.iterateKeys(nvsIterateKeys);

    if (doubleStValue.getValue(dv) == EmGetValueResult::failed) {
        logError("TS---->", "Get 'doubleStValue' FAILED!");
    } else {
        logInfo<100>("TS---->", "Get 'doubleStValue' = %f", dv);
    }

    if (intStTag.getValue(iv) == EmGetValueResult::failed) {
        logError("TS---->", "Get 'intStTag' FAILED!");
    } else {
        logInfo<100>("TS---->", "Get 'intStTag' = %d", intStTag.getValue().asInteger());
    }


    if (ts.hasKey("short_key")) {
        logInfo("TS---->", "short_key ALREADY exist!");
    } else {
        logInfo("TS---->", "short_key DOES NOT exist!");
    }
    
    ts.setValue("short_key", 123);
    if (ts.getValue("short_key", iv)) {
        logInfo<100>("TS---->", "short_key: %d", iv);
    } else {
        logError("TS---->", "short_key FAILED!");
    }

    if (ts.hasKey("key_requiring_HASH")) {
        logInfo("TS---->", "key_requiring_HASH ALREADY exist!");
    } else {
        logError("TS---->", "key_requiring_HASH DOES NOT exist!");
    }

    ts.setValue("key_requiring_HASH", 123);
    if (ts.getValue("key_requiring_HASH", iv)) {
        logInfo<100>("TS---->", "key_requiring_HASH: %d", iv);
    } else {
        logError("TS---->", "key_requiring_HASH FAILED!");
    }

    nvsTest<int8_t>(ts, "int8_t", -123);
    nvsTest<uint8_t>(ts, "uint8_t", 223);
    nvsTest<int16_t>(ts, "int16_t", -12300);
    nvsTest<uint16_t>(ts, "uint16_t", 52300);
    nvsTest<int32_t>(ts, "int32_t", -1230000);
    nvsTest<uint32_t>(ts, "uint32_t", 5230000);
    nvsTest<int64_t>(ts, "int64_t", -1230000000000000);
    nvsTest<uint64_t>(ts, "uint64_t", 5230000000000000);
    nvsTest<float>(ts, "float", -1.2);
    nvsTest<double>(ts, "double", -2.345);
    EmTagValue st("This is a string Tag");
    nvsTest<EmTagValue>(ts, "str_tag", st);
    EmTagValue ft((float)-3.5);
    nvsTest<EmTagValue>(ts, "float_tag", ft);
    const char* myStr = "Hi, how are you out there!? :)";
    nvsTestString(ts, "string", myStr);
    if (!ts.isSameValue("string", myStr)) {
        logError("TS---->", "String comparison FAILED!");
    }

    EmTagValue strTag;
    if (!ts.getValue("str_tag", strTag)) {
        logError("TS---->", "Get 'str_tag' FAILED!");
    } else {
        logInfo<100>("TS---->", "Get 'str_tag' = %s", strTag.asString());
    }

    EmTagValue floatTag;
    if (!ts.getValue("float_tag", floatTag)) {
        logError("TS---->", "Get 'float_tag' FAILED!");
    } else {
        logInfo<100>("TS---->", "Get 'float_tag' = %g", floatTag.asReal());
    }


    ts.iterateKeys(nvsIterateKeys);
}

void setup() {
    EmLog::init(printLog, EmLogLevel::debug);
    Serial.begin();
    tDelay(3000, true); // Wait for serial monitor
    logInfo("Setup", F("initializing..."));
    TEST_NVS();
}

void loop() {
    tDelay(1000, true);
}
