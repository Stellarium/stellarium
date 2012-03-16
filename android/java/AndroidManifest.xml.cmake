<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
      package="org.stellarium.stellarium"
      android:sharedUserId="org.stellarium.stellarium"
      android:versionCode="1"
      android:versionName="@ANDROID_VERSION_NAME@">
    <application android:name="org.kde.necessitas.origo.QtApplication" android:icon="@drawable/stellarium_icon" android:label="@string/app_name" android:debuggable="true">
        <activity android:name="org.kde.necessitas.origo.QtActivity" android:label="@string/app_name" android:configChanges="orientation|locale|fontScale|keyboard|keyboardHidden" android:screenOrientation="portrait" >
            <intent-filter>
                <action android:name="android.intent.action.MAIN"/>
                <category android:name="android.intent.category.LAUNCHER"/>
            </intent-filter>
            <meta-data android:name="android.app.qt_libs_resource_id" android:resource="@array/qt_libs"/>
            <meta-data android:name="android.app.bundled_libs_resource_id" android:resource="@array/bundled_libs"/>
            <meta-data android:name="android.app.lib_name" android:value="stellarium"/>
            <!--  Messages maps -->
            <meta-data android:name="android.app.ministro_not_found_msg" android:value="@string/ministro_not_found_msg"/>
            <meta-data android:name="android.app.ministro_needed_msg" android:value="@string/ministro_needed_msg"/>
            <meta-data android:name="android.app.fatal_error_msg" android:value="@string/fatal_error_msg"/>
            <!--  Messages maps -->
            <!-- Splash screen -->
            <meta-data android:name="android.app.splash_screen" android:resource="@layout/splash"/>
            <!-- Splash screen -->
        </activity>
    </application>
    <uses-sdk android:minSdkVersion="@ANDROID_API_LEVEL@"/>
    <supports-screens android:smallScreens="true" android:normalScreens="true" android:largeScreens="true" android:resizeable="true" android:anyDensity="true"/>
    @ANDROID_PERMISSIONS@
    <uses-permission android:name="android.permission.INTERNET"/>
    <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE"/>
	<uses-permission android:name="android.permission.DISABLE_KEYGUARD"/>	
</manifest> 
