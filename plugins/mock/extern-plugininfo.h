/* This file is generated by the nymea build system. Any changes to this file will *
 * be lost. If you want to change this file, edit the plugin's json file.          */

#ifndef EXTERNPLUGININFO_H
#define EXTERNPLUGININFO_H

#include "typeutils.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(dcMock)

extern PluginId pluginId;
extern ParamTypeId mockPluginConfigParamIntParamTypeId;
extern ParamTypeId mockPluginConfigParamBoolParamTypeId;
extern VendorId nymeaVendorId;
extern ThingClassId mockThingClassId;
extern ParamTypeId mockThingHttpportParamTypeId;
extern ParamTypeId mockThingAsyncParamTypeId;
extern ParamTypeId mockThingBrokenParamTypeId;
extern ParamTypeId mockSettingsSetting1ParamTypeId;
extern ParamTypeId mockDiscoveryResultCountParamTypeId;
extern StateTypeId mockIntStateTypeId;
extern StateTypeId mockBoolStateTypeId;
extern StateTypeId mockDoubleStateTypeId;
extern StateTypeId mockBatteryLevelStateTypeId;
extern StateTypeId mockBatteryCriticalStateTypeId;
extern StateTypeId mockPowerStateTypeId;
extern EventTypeId mockIntEventTypeId;
extern ParamTypeId mockIntEventIntParamTypeId;
extern EventTypeId mockBoolEventTypeId;
extern ParamTypeId mockBoolEventBoolParamTypeId;
extern EventTypeId mockDoubleEventTypeId;
extern ParamTypeId mockDoubleEventDoubleParamTypeId;
extern EventTypeId mockBatteryLevelEventTypeId;
extern ParamTypeId mockBatteryLevelEventBatteryLevelParamTypeId;
extern EventTypeId mockBatteryCriticalEventTypeId;
extern ParamTypeId mockBatteryCriticalEventBatteryCriticalParamTypeId;
extern EventTypeId mockPowerEventTypeId;
extern ParamTypeId mockPowerEventPowerParamTypeId;
extern EventTypeId mockEvent1EventTypeId;
extern EventTypeId mockEvent2EventTypeId;
extern ParamTypeId mockEvent2EventIntParamParamTypeId;
extern ActionTypeId mockPowerActionTypeId;
extern ParamTypeId mockPowerActionPowerParamTypeId;
extern ActionTypeId mockWithParamsActionTypeId;
extern ParamTypeId mockWithParamsActionParam1ParamTypeId;
extern ParamTypeId mockWithParamsActionParam2ParamTypeId;
extern ActionTypeId mockWithoutParamsActionTypeId;
extern ActionTypeId mockAsyncActionTypeId;
extern ActionTypeId mockFailingActionTypeId;
extern ActionTypeId mockAsyncFailingActionTypeId;
extern ActionTypeId mockAddToFavoritesBrowserItemActionTypeId;
extern ActionTypeId mockRemoveFromFavoritesBrowserItemActionTypeId;
extern ThingClassId autoMockThingClassId;
extern ParamTypeId autoMockThingHttpportParamTypeId;
extern ParamTypeId autoMockThingAsyncParamTypeId;
extern ParamTypeId autoMockThingBrokenParamTypeId;
extern StateTypeId autoMockIntStateTypeId;
extern StateTypeId autoMockBoolValueStateTypeId;
extern EventTypeId autoMockIntEventTypeId;
extern ParamTypeId autoMockIntEventIntParamTypeId;
extern EventTypeId autoMockBoolValueEventTypeId;
extern ParamTypeId autoMockBoolValueEventBoolValueParamTypeId;
extern EventTypeId autoMockEvent1EventTypeId;
extern EventTypeId autoMockEvent2EventTypeId;
extern ParamTypeId autoMockEvent2EventIntParamParamTypeId;
extern ActionTypeId autoMockWithParamsActionTypeId;
extern ParamTypeId autoMockWithParamsActionMockActionParam1ParamTypeId;
extern ParamTypeId autoMockWithParamsActionMockActionParam2ParamTypeId;
extern ActionTypeId autoMockMockActionNoParmsActionTypeId;
extern ActionTypeId autoMockMockActionAsyncActionTypeId;
extern ActionTypeId autoMockMockActionBrokenActionTypeId;
extern ActionTypeId autoMockMockActionAsyncBrokenActionTypeId;
extern ThingClassId pushButtonMockThingClassId;
extern ParamTypeId pushButtonMockDiscoveryResultCountParamTypeId;
extern StateTypeId pushButtonMockColorStateTypeId;
extern StateTypeId pushButtonMockPercentageStateTypeId;
extern StateTypeId pushButtonMockAllowedValuesStateTypeId;
extern StateTypeId pushButtonMockDoubleStateTypeId;
extern StateTypeId pushButtonMockBoolStateTypeId;
extern EventTypeId pushButtonMockColorEventTypeId;
extern ParamTypeId pushButtonMockColorEventColorParamTypeId;
extern EventTypeId pushButtonMockPercentageEventTypeId;
extern ParamTypeId pushButtonMockPercentageEventPercentageParamTypeId;
extern EventTypeId pushButtonMockAllowedValuesEventTypeId;
extern ParamTypeId pushButtonMockAllowedValuesEventAllowedValuesParamTypeId;
extern EventTypeId pushButtonMockDoubleEventTypeId;
extern ParamTypeId pushButtonMockDoubleEventDoubleParamTypeId;
extern EventTypeId pushButtonMockBoolEventTypeId;
extern ParamTypeId pushButtonMockBoolEventBoolParamTypeId;
extern ActionTypeId pushButtonMockColorActionTypeId;
extern ParamTypeId pushButtonMockColorActionColorParamTypeId;
extern ActionTypeId pushButtonMockPercentageActionTypeId;
extern ParamTypeId pushButtonMockPercentageActionPercentageParamTypeId;
extern ActionTypeId pushButtonMockAllowedValuesActionTypeId;
extern ParamTypeId pushButtonMockAllowedValuesActionAllowedValuesParamTypeId;
extern ActionTypeId pushButtonMockDoubleActionTypeId;
extern ParamTypeId pushButtonMockDoubleActionDoubleParamTypeId;
extern ActionTypeId pushButtonMockBoolActionTypeId;
extern ParamTypeId pushButtonMockBoolActionBoolParamTypeId;
extern ActionTypeId pushButtonMockTimeoutActionTypeId;
extern ThingClassId displayPinMockThingClassId;
extern ParamTypeId displayPinMockThingPinParamTypeId;
extern ParamTypeId displayPinMockDiscoveryResultCountParamTypeId;
extern StateTypeId displayPinMockColorStateTypeId;
extern StateTypeId displayPinMockPercentageStateTypeId;
extern StateTypeId displayPinMockAllowedValuesStateTypeId;
extern StateTypeId displayPinMockDoubleStateTypeId;
extern StateTypeId displayPinMockBoolStateTypeId;
extern EventTypeId displayPinMockColorEventTypeId;
extern ParamTypeId displayPinMockColorEventColorParamTypeId;
extern EventTypeId displayPinMockPercentageEventTypeId;
extern ParamTypeId displayPinMockPercentageEventPercentageParamTypeId;
extern EventTypeId displayPinMockAllowedValuesEventTypeId;
extern ParamTypeId displayPinMockAllowedValuesEventAllowedValuesParamTypeId;
extern EventTypeId displayPinMockDoubleEventTypeId;
extern ParamTypeId displayPinMockDoubleEventDoubleParamTypeId;
extern EventTypeId displayPinMockBoolEventTypeId;
extern ParamTypeId displayPinMockBoolEventBoolParamTypeId;
extern ActionTypeId displayPinMockColorActionTypeId;
extern ParamTypeId displayPinMockColorActionColorParamTypeId;
extern ActionTypeId displayPinMockPercentageActionTypeId;
extern ParamTypeId displayPinMockPercentageActionPercentageParamTypeId;
extern ActionTypeId displayPinMockAllowedValuesActionTypeId;
extern ParamTypeId displayPinMockAllowedValuesActionAllowedValuesParamTypeId;
extern ActionTypeId displayPinMockDoubleActionTypeId;
extern ParamTypeId displayPinMockDoubleActionDoubleParamTypeId;
extern ActionTypeId displayPinMockBoolActionTypeId;
extern ParamTypeId displayPinMockBoolActionBoolParamTypeId;
extern ActionTypeId displayPinMockTimeoutActionTypeId;
extern ThingClassId parentMockThingClassId;
extern StateTypeId parentMockBoolValueStateTypeId;
extern EventTypeId parentMockBoolValueEventTypeId;
extern ParamTypeId parentMockBoolValueEventBoolValueParamTypeId;
extern ActionTypeId parentMockBoolValueActionTypeId;
extern ParamTypeId parentMockBoolValueActionBoolValueParamTypeId;
extern ThingClassId childMockThingClassId;
extern StateTypeId childMockBoolValueStateTypeId;
extern EventTypeId childMockBoolValueEventTypeId;
extern ParamTypeId childMockBoolValueEventBoolValueParamTypeId;
extern ActionTypeId childMockBoolValueActionTypeId;
extern ParamTypeId childMockBoolValueActionBoolValueParamTypeId;
extern ThingClassId inputTypeMockThingClassId;
extern ParamTypeId inputTypeMockThingTextLineParamTypeId;
extern ParamTypeId inputTypeMockThingTextAreaParamTypeId;
extern ParamTypeId inputTypeMockThingPasswordParamTypeId;
extern ParamTypeId inputTypeMockThingSearchParamTypeId;
extern ParamTypeId inputTypeMockThingMailParamTypeId;
extern ParamTypeId inputTypeMockThingIp4ParamTypeId;
extern ParamTypeId inputTypeMockThingIp6ParamTypeId;
extern ParamTypeId inputTypeMockThingUrlParamTypeId;
extern ParamTypeId inputTypeMockThingMacParamTypeId;
extern StateTypeId inputTypeMockBoolStateTypeId;
extern StateTypeId inputTypeMockWritableBoolStateTypeId;
extern StateTypeId inputTypeMockIntStateTypeId;
extern StateTypeId inputTypeMockWritableIntStateTypeId;
extern StateTypeId inputTypeMockWritableIntMinMaxStateTypeId;
extern StateTypeId inputTypeMockUintStateTypeId;
extern StateTypeId inputTypeMockWritableUIntStateTypeId;
extern StateTypeId inputTypeMockWritableUIntMinMaxStateTypeId;
extern StateTypeId inputTypeMockDoubleStateTypeId;
extern StateTypeId inputTypeMockWritableDoubleStateTypeId;
extern StateTypeId inputTypeMockWritableDoubleMinMaxStateTypeId;
extern StateTypeId inputTypeMockStringStateTypeId;
extern StateTypeId inputTypeMockWritableStringStateTypeId;
extern StateTypeId inputTypeMockWritableStringSelectionStateTypeId;
extern StateTypeId inputTypeMockColorStateTypeId;
extern StateTypeId inputTypeMockWritableColorStateTypeId;
extern StateTypeId inputTypeMockTimeStateTypeId;
extern StateTypeId inputTypeMockWritableTimeStateTypeId;
extern StateTypeId inputTypeMockTimestampIntStateTypeId;
extern StateTypeId inputTypeMockWritableTimestampIntStateTypeId;
extern StateTypeId inputTypeMockTimestampUIntStateTypeId;
extern StateTypeId inputTypeMockWritableTimestampUIntStateTypeId;
extern EventTypeId inputTypeMockBoolEventTypeId;
extern ParamTypeId inputTypeMockBoolEventBoolParamTypeId;
extern EventTypeId inputTypeMockWritableBoolEventTypeId;
extern ParamTypeId inputTypeMockWritableBoolEventWritableBoolParamTypeId;
extern EventTypeId inputTypeMockIntEventTypeId;
extern ParamTypeId inputTypeMockIntEventIntParamTypeId;
extern EventTypeId inputTypeMockWritableIntEventTypeId;
extern ParamTypeId inputTypeMockWritableIntEventWritableIntParamTypeId;
extern EventTypeId inputTypeMockWritableIntMinMaxEventTypeId;
extern ParamTypeId inputTypeMockWritableIntMinMaxEventWritableIntMinMaxParamTypeId;
extern EventTypeId inputTypeMockUintEventTypeId;
extern ParamTypeId inputTypeMockUintEventUintParamTypeId;
extern EventTypeId inputTypeMockWritableUIntEventTypeId;
extern ParamTypeId inputTypeMockWritableUIntEventWritableUIntParamTypeId;
extern EventTypeId inputTypeMockWritableUIntMinMaxEventTypeId;
extern ParamTypeId inputTypeMockWritableUIntMinMaxEventWritableUIntMinMaxParamTypeId;
extern EventTypeId inputTypeMockDoubleEventTypeId;
extern ParamTypeId inputTypeMockDoubleEventDoubleParamTypeId;
extern EventTypeId inputTypeMockWritableDoubleEventTypeId;
extern ParamTypeId inputTypeMockWritableDoubleEventWritableDoubleParamTypeId;
extern EventTypeId inputTypeMockWritableDoubleMinMaxEventTypeId;
extern ParamTypeId inputTypeMockWritableDoubleMinMaxEventWritableDoubleMinMaxParamTypeId;
extern EventTypeId inputTypeMockStringEventTypeId;
extern ParamTypeId inputTypeMockStringEventStringParamTypeId;
extern EventTypeId inputTypeMockWritableStringEventTypeId;
extern ParamTypeId inputTypeMockWritableStringEventWritableStringParamTypeId;
extern EventTypeId inputTypeMockWritableStringSelectionEventTypeId;
extern ParamTypeId inputTypeMockWritableStringSelectionEventWritableStringSelectionParamTypeId;
extern EventTypeId inputTypeMockColorEventTypeId;
extern ParamTypeId inputTypeMockColorEventColorParamTypeId;
extern EventTypeId inputTypeMockWritableColorEventTypeId;
extern ParamTypeId inputTypeMockWritableColorEventWritableColorParamTypeId;
extern EventTypeId inputTypeMockTimeEventTypeId;
extern ParamTypeId inputTypeMockTimeEventTimeParamTypeId;
extern EventTypeId inputTypeMockWritableTimeEventTypeId;
extern ParamTypeId inputTypeMockWritableTimeEventWritableTimeParamTypeId;
extern EventTypeId inputTypeMockTimestampIntEventTypeId;
extern ParamTypeId inputTypeMockTimestampIntEventTimestampIntParamTypeId;
extern EventTypeId inputTypeMockWritableTimestampIntEventTypeId;
extern ParamTypeId inputTypeMockWritableTimestampIntEventWritableTimestampIntParamTypeId;
extern EventTypeId inputTypeMockTimestampUIntEventTypeId;
extern ParamTypeId inputTypeMockTimestampUIntEventTimestampUIntParamTypeId;
extern EventTypeId inputTypeMockWritableTimestampUIntEventTypeId;
extern ParamTypeId inputTypeMockWritableTimestampUIntEventWritableTimestampUIntParamTypeId;
extern ActionTypeId inputTypeMockWritableBoolActionTypeId;
extern ParamTypeId inputTypeMockWritableBoolActionWritableBoolParamTypeId;
extern ActionTypeId inputTypeMockWritableIntActionTypeId;
extern ParamTypeId inputTypeMockWritableIntActionWritableIntParamTypeId;
extern ActionTypeId inputTypeMockWritableIntMinMaxActionTypeId;
extern ParamTypeId inputTypeMockWritableIntMinMaxActionWritableIntMinMaxParamTypeId;
extern ActionTypeId inputTypeMockWritableUIntActionTypeId;
extern ParamTypeId inputTypeMockWritableUIntActionWritableUIntParamTypeId;
extern ActionTypeId inputTypeMockWritableUIntMinMaxActionTypeId;
extern ParamTypeId inputTypeMockWritableUIntMinMaxActionWritableUIntMinMaxParamTypeId;
extern ActionTypeId inputTypeMockWritableDoubleActionTypeId;
extern ParamTypeId inputTypeMockWritableDoubleActionWritableDoubleParamTypeId;
extern ActionTypeId inputTypeMockWritableDoubleMinMaxActionTypeId;
extern ParamTypeId inputTypeMockWritableDoubleMinMaxActionWritableDoubleMinMaxParamTypeId;
extern ActionTypeId inputTypeMockWritableStringActionTypeId;
extern ParamTypeId inputTypeMockWritableStringActionWritableStringParamTypeId;
extern ActionTypeId inputTypeMockWritableStringSelectionActionTypeId;
extern ParamTypeId inputTypeMockWritableStringSelectionActionWritableStringSelectionParamTypeId;
extern ActionTypeId inputTypeMockWritableColorActionTypeId;
extern ParamTypeId inputTypeMockWritableColorActionWritableColorParamTypeId;
extern ActionTypeId inputTypeMockWritableTimeActionTypeId;
extern ParamTypeId inputTypeMockWritableTimeActionWritableTimeParamTypeId;
extern ActionTypeId inputTypeMockWritableTimestampIntActionTypeId;
extern ParamTypeId inputTypeMockWritableTimestampIntActionWritableTimestampIntParamTypeId;
extern ActionTypeId inputTypeMockWritableTimestampUIntActionTypeId;
extern ParamTypeId inputTypeMockWritableTimestampUIntActionWritableTimestampUIntParamTypeId;
extern ThingClassId oAuthGoogleMockThingClassId;
extern ThingClassId oAuthSonosMockThingClassId;
extern ThingClassId userAndPassMockThingClassId;
extern ThingClassId genericIoMockThingClassId;
extern StateTypeId genericIoMockDigitalInput1StateTypeId;
extern StateTypeId genericIoMockDigitalInput2StateTypeId;
extern StateTypeId genericIoMockDigitalOutput1StateTypeId;
extern StateTypeId genericIoMockDigitalOutput2StateTypeId;
extern StateTypeId genericIoMockAnalogInput1StateTypeId;
extern StateTypeId genericIoMockAnalogInput2StateTypeId;
extern StateTypeId genericIoMockAnalogOutput1StateTypeId;
extern StateTypeId genericIoMockAnalogOutput2StateTypeId;
extern EventTypeId genericIoMockDigitalInput1EventTypeId;
extern ParamTypeId genericIoMockDigitalInput1EventDigitalInput1ParamTypeId;
extern EventTypeId genericIoMockDigitalInput2EventTypeId;
extern ParamTypeId genericIoMockDigitalInput2EventDigitalInput2ParamTypeId;
extern EventTypeId genericIoMockDigitalOutput1EventTypeId;
extern ParamTypeId genericIoMockDigitalOutput1EventDigitalOutput1ParamTypeId;
extern EventTypeId genericIoMockDigitalOutput2EventTypeId;
extern ParamTypeId genericIoMockDigitalOutput2EventDigitalOutput2ParamTypeId;
extern EventTypeId genericIoMockAnalogInput1EventTypeId;
extern ParamTypeId genericIoMockAnalogInput1EventAnalogInput1ParamTypeId;
extern EventTypeId genericIoMockAnalogInput2EventTypeId;
extern ParamTypeId genericIoMockAnalogInput2EventAnalogInput2ParamTypeId;
extern EventTypeId genericIoMockAnalogOutput1EventTypeId;
extern ParamTypeId genericIoMockAnalogOutput1EventAnalogOutput1ParamTypeId;
extern EventTypeId genericIoMockAnalogOutput2EventTypeId;
extern ParamTypeId genericIoMockAnalogOutput2EventAnalogOutput2ParamTypeId;
extern ActionTypeId genericIoMockDigitalOutput1ActionTypeId;
extern ParamTypeId genericIoMockDigitalOutput1ActionDigitalOutput1ParamTypeId;
extern ActionTypeId genericIoMockDigitalOutput2ActionTypeId;
extern ParamTypeId genericIoMockDigitalOutput2ActionDigitalOutput2ParamTypeId;
extern ActionTypeId genericIoMockAnalogInput1ActionTypeId;
extern ParamTypeId genericIoMockAnalogInput1ActionAnalogInput1ParamTypeId;
extern ActionTypeId genericIoMockAnalogOutput1ActionTypeId;
extern ParamTypeId genericIoMockAnalogOutput1ActionAnalogOutput1ParamTypeId;
extern ActionTypeId genericIoMockAnalogOutput2ActionTypeId;
extern ParamTypeId genericIoMockAnalogOutput2ActionAnalogOutput2ParamTypeId;
extern ThingClassId virtualIoLightMockThingClassId;
extern StateTypeId virtualIoLightMockPowerStateTypeId;
extern EventTypeId virtualIoLightMockPowerEventTypeId;
extern ParamTypeId virtualIoLightMockPowerEventPowerParamTypeId;
extern ActionTypeId virtualIoLightMockPowerActionTypeId;
extern ParamTypeId virtualIoLightMockPowerActionPowerParamTypeId;
extern ThingClassId virtualIoTemperatureSensorMockThingClassId;
extern ParamTypeId virtualIoTemperatureSensorMockSettingsMinTempParamTypeId;
extern ParamTypeId virtualIoTemperatureSensorMockSettingsMaxTempParamTypeId;
extern StateTypeId virtualIoTemperatureSensorMockInputStateTypeId;
extern StateTypeId virtualIoTemperatureSensorMockTemperatureStateTypeId;
extern EventTypeId virtualIoTemperatureSensorMockInputEventTypeId;
extern ParamTypeId virtualIoTemperatureSensorMockInputEventInputParamTypeId;
extern EventTypeId virtualIoTemperatureSensorMockTemperatureEventTypeId;
extern ParamTypeId virtualIoTemperatureSensorMockTemperatureEventTemperatureParamTypeId;
extern ActionTypeId virtualIoTemperatureSensorMockInputActionTypeId;
extern ParamTypeId virtualIoTemperatureSensorMockInputActionInputParamTypeId;

#endif // EXTERNPLUGININFO_H
