plugins {
    kotlin("jvm") version "1.9.20"
    id("org.jetbrains.kotlin.android") version "2.0.0-Beta4" apply false
}

buildscript {
    repositories {
        google()
        jcenter()
        mavenCentral()
    }
    dependencies {
        classpath("com.android.tools.build:gradle:8.12.1")
        classpath("com.google.gms:google-services:4.4.2")
        classpath("com.google.firebase:firebase-crashlytics-gradle:2.9.9")
        classpath("org.jetbrains.kotlin:kotlin-gradle-plugin:2.0.0-Beta4")
        classpath("com.joom.paranoid:paranoid-gradle-plugin:0.3.14")
        classpath("com.squareup.okhttp3:okhttp-dnsoverhttps:5.1.0")
    }
}