#ifndef TELEGRAM_MESSAGES_H
#define TELEGRAM_MESSAGES_H

// System Status Messages
#define SYSTEM_ONLINE_MSG_AR "🟢 نظام مكافحة سرقة عداد المياه\n\nمرحباً شقة رقم %d!\nالنظام متصل ويعمل الآن.\n\nلتغيير إعدادات الواي فاي، يجب عليك أولاً الاتصال بشبكة الواي فاي الخاصة بالجهاز ثم زيارة:\nhttp://%s.local"
#define SYSTEM_ONLINE_MSG_EN "\n\n🟢 Water Meter Anti-Theft System\n\nHello Apartment %d!\nThe system is now online and working.\n\nTo configure WiFi settings, you must first connect to the device's WiFi network and then visit:\nhttp://%s.local"

// Theft Detection Messages
// For the apartment being stolen from
#define THEFT_ALERT_OWNER_AR "🚨 تنبيه سرقة محتملة!\n\nتم اكتشاف اهتزاز في عداد المياه الخاص بشقتك.\nالرجاء التحقق من العداد فوراً!"
#define THEFT_ALERT_OWNER_EN "\n\n🚨 Potential Theft Alert!\n\nVibration detected in your water meter.\nPlease check your meter immediately!"

// For apartments in the same box
#define THEFT_ALERT_SAME_BOX_AR "⚠️ تنبيه أمني!\n\nتم اكتشاف اهتزاز محتمل في عداد المياه للشقة رقم %d.\nهذا العداد موجود في نفس الصندوق مع عدادك.\nالرجاء الحذر والمراقبة!"
#define THEFT_ALERT_SAME_BOX_EN "\n\n⚠️ Security Alert!\n\nPotential vibration detected in Apartment %d's water meter.\nThis meter is in the same box as yours.\nPlease be vigilant!"

// For apartments in the adjacent box
#define THEFT_ALERT_ADJACENT_BOX_AR "⚠️ تنبيه أمني!\n\nتم اكتشاف اهتزاز محتمل في عداد المياه للشقة رقم %d.\nهذا العداد موجود في الصندوق المجاور لصندوق عدادك.\nالرجاء الحذر والمراقبة!"
#define THEFT_ALERT_ADJACENT_BOX_EN "\n\n⚠️ Security Alert!\n\nPotential vibration detected in Apartment %d's water meter.\nThis meter is in the box adjacent to yours.\nPlease be vigilant!"

// For apartments on the other side
#define THEFT_ALERT_OTHER_SIDE_AR "ℹ️ إشعار أمني!\n\nتم اكتشاف اهتزاز محتمل في عداد المياه للشقة رقم %d.\nهذا العداد موجود في الجهة الأخرى من المبنى."
#define THEFT_ALERT_OTHER_SIDE_EN "\n\nℹ️ Security Notice!\n\nPotential vibration detected in Apartment %d's water meter.\nThis meter is on the other side of the building."

// Sensor Wire Cut Detection Messages
#define SENSOR_WIRE_CUT_ALERT_AR "🚨 تنبيه خطر - قطع سلك مستشعر!\n\nتم اكتشاف قطع في أحد أسلاك المستشعرات في صندوق العدادات الخاص بكم.\nلن يتم إخطارك بقطع الأسلاك مرة أخرى حتى يتم إصلاح المشكلة.\nالرجاء الاتصال بالصيانة."
#define SENSOR_WIRE_CUT_ALERT_EN "\n\n🚨 Danger Alert - Sensor Wire Cut!\n\nA cut has been detected in one of the sensor cables in your meters box.\nYou won't be notified of wire cuts again until this is fixed.\nPlease contact maintenance."

// Wire Cut between Distribution Box and Control unit Detection Messages
#define DIST_CTRL_WIRE_CUT_ALERT_AR "🔥 تنبيه خطر - قطع سلك رئيسي!\n\nتم اكتشاف قطع في الأسلاك بين لوحة التوزيع ووحدة التحكم في جهتك من المبنى.\nلن يتم إخطارك بقطع الأسلاك مرة أخرى حتى يتم إصلاح المشكلة.\nالرجاء الاتصال بالصيانة فورًا."
#define DIST_CTRL_WIRE_CUT_ALERT_EN "\n\n🔥 Danger Alert - Main Wire Cut!\n\nA wire cut has been detected between the distribution box and the control unit in your side of the building.\nYou won't be notified of wire cuts again until this is fixed.\nPlease contact maintenance immediately."

// Startup Wire Cut Detection Messages For Sensors
#define STARTUP_SENSOR_WIRE_CUT_AR "🛠️ تنبيه هام!\n\nتمت إعادة تشغيل النظام وتم اكتشاف قطع في أحد أسلاك المستشعرات في صندوق العدادات الخاص بكم.\nلن يتمكن النظام من اكتشاف قطع الأسلاك في صندوق العدادات الخاص بكم حتى يتم إصلاح المشكلة.\nالرجاء الاتصال بالصيانة في أقرب وقت ممكن لإصلاح المشكلة وإعادة تفعيل نظام كشف قطع الأسلاك."
#define STARTUP_SENSOR_WIRE_CUT_EN "\n\n🛠️ Important Alert!\n\nThe system has rebooted and a cut was detected in one of the sensor wires in your meters box.\nThe system won't be able to detect wire cuts in your meter box until this issue is fixed.\nPlease contact maintenance as soon as possible to fix the issue and reactivate the wire cut detection system."

// Startup Wire Cut between Distribution Box and Control unit Detection Messages
#define STARTUP_DIST_CTRL_WIRE_CUT_AR "🛠️ تنبيه هام!\n\nتمت إعادة تشغيل النظام وتم اكتشاف قطع في الأسلاك بين لوحة التوزيع ووحدة التحكم في جهتك من المبنى.\nلن يتمكن النظام من اكتشاف قطع الأسلاك في جهتك حتى يتم إصلاح المشكلة.\nالرجاء الاتصال بالصيانة في أقرب وقت ممكن لإصلاح المشكلة وإعادة تفعيل نظام كشف قطع الأسلاك."
#define STARTUP_DIST_CTRL_WIRE_CUT_EN "\n\n🛠️ Important Alert!\n\nSystem has rebooted and a wire cut was detected between the distribution box and the control unit in your side of the building.\nThe system won't be able to detect wire cuts in your side until this issue is fixed.\nPlease contact maintenance as soon as possible to fix the issue and reactivate the wire cut detection system."

// Service Subscription Messages
#define SERVICE_ENABLED_AR "🎉 تهانينا!\n\nتم تفعيل خدمة مكافحة سرقة عداد المياه لشقتك رقم %d.\nسيتم إخطارك بأي نشاط مشبوه يتعلق بعدادك."
#define SERVICE_ENABLED_EN "\n\n🎉 Congratulations!\n\nWater Meter Anti-Theft service has been activated for your Apartment %d.\nYou will be notified of any suspicious activity related to your meter."

// Service Disabled Message
#define SERVICE_DISABLED_AR "⚠️ تنبيه!\n\nتم إيقاف خدمة مكافحة سرقة عداد المياه لشقتك رقم %d."
#define SERVICE_DISABLED_EN "\n\n⚠️ Alert!\n\nWater Meter Anti-Theft service has been deactivated for your Apartment %d."


#endif // TELEGRAM_MESSAGES_H